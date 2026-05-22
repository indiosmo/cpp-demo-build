#include "catch2/catch_test_macros.hpp"
#include "factories.hpp"
#include "market_data/messages.hpp"
#include "market_data/types.hpp"
#include "matching_engine/engine.hpp"
#include "matching_engine/engine_config.hpp"

#include <cstddef>
#include <variant>
#include <vector>

/*
 * Component test for the production matching engine exposed through
 * matching_engine::engine. Protocol-level scenarios exercise the runtime
 * boundary through typed codecs; this file keeps one in-process scenario for
 * fast iteration through the
 * synchronous callback channel.
 *
 * The scenario uses a few resting orders, an aggressing sell that
 * crosses the only resting buy, the resulting top-of-book elimination, and a
 * fresh resting buy after the elimination. The expected emissions come from
 * a cancelled best bid and a fresh resting buy. The contract that produces
 * the events is documented in
 * docs/engine-specs.md.
 */

namespace {

namespace md = market_data;
namespace me = matching_engine;
namespace rt = order_entry;
namespace ft = matching_engine::testing;

using ft::aapl;
using ft::alice;
using ft::bob;

me::engine_config default_config()
{
  return me::engine_config{
    .valid_symbols{aapl},
    .expected_resting_orders = 1024,
    .node_pool_chunk_size = 32,
  };
}

struct recorder
{
  me::engine engine;
  std::vector<md::message> events;
  std::vector<rt::event> order_entry_events;

  recorder()
    : engine{default_config()}
  {
    engine.on_market_data = [this](const md::message& ev) { events.push_back(ev); };
    engine.on_order_entry = [this](const rt::event& ev) { order_entry_events.push_back(ev); };
  }

  void send_order(const ft::order_params& params)
  {
    engine.send(ft::make_new_order_single(params));
  }
};

template <typename T>
std::vector<T> collect(const std::vector<md::message>& events)
{
  std::vector<T> hits;
  for (const auto& ev : events) {
    if (const auto* hit = std::get_if<T>(&ev)) {
      hits.push_back(*hit);
    }
  }
  return hits;
}

template <typename T>
std::vector<T> collect_order_entry(const std::vector<rt::event>& events)
{
  std::vector<T> hits;
  for (const auto& ev : events) {
    if (const auto* hit = std::get_if<T>(&ev)) {
      hits.push_back(*hit);
    }
  }
  return hits;
}

} // namespace

TEST_CASE("matching_engine - cross, eliminate, fresh rest", "[matching_engine][engine]")
{
  recorder r;

  // Seed a bid, an ask, and a lower bid:
  //   alice 1 buys 100 @ 10  -> new best bid
  //   alice 2 sells 100 @ 12 -> new best ask
  //   bob   102 sells 100 @ 11 -> tightens the ask side
  r.send_order({.cl_ord_id{1}, .side = rt::types::side::buy, .price{10}});
  r.send_order({.cl_ord_id{2}, .side = rt::types::side::sell, .price{12}});
  r.send_order({.client_id = bob, .cl_ord_id{102}, .side = rt::types::side::sell, .price{11}});

  // Aggressing sell at 10 -- crosses alice 1 fully, empties the bid side.
  r.send_order({.client_id = bob, .cl_ord_id{103}, .side = rt::types::side::sell, .price{10}});

  // Fresh resting buy at the new best.
  r.send_order({.cl_ord_id{3}, .side = rt::types::side::buy, .price{10}});

  // One trade only: bob 103 lifts alice 1 at 10 for 100.
  const auto trades = collect<md::trade>(r.events);
  REQUIRE(trades.size() == 1);
  CHECK(trades[0].buyer == md::types::order_id{1});
  CHECK(trades[0].seller == md::types::order_id{103});
  CHECK(trades[0].price == md::types::price{10});
  CHECK(trades[0].quantity == md::types::quantity{100});

  // Five book-update emissions in order:
  //   B 10/100  (alice 1 rests at the new best bid)
  //   S 12/100  (alice 2 rests at the new best ask)
  //   S 11/100  (bob 102 tightens the ask)
  //   B -/-     (cross consumes alice 1; bid side eliminated)
  //   B 10/100  (alice 3 rests at the new best bid)
  const auto book_updates = collect<md::mbo_book_update>(r.events);
  REQUIRE(book_updates.size() == 5);

  CHECK(book_updates[0].side == md::types::side::buy);
  CHECK(book_updates[0].price == md::types::price{10});
  CHECK(book_updates[0].quantity == md::types::quantity{100});

  CHECK(book_updates[1].side == md::types::side::sell);
  CHECK(book_updates[1].price == md::types::price{12});
  CHECK(book_updates[1].quantity == md::types::quantity{100});

  CHECK(book_updates[2].side == md::types::side::sell);
  CHECK(book_updates[2].price == md::types::price{11});
  CHECK(book_updates[2].quantity == md::types::quantity{100});

  CHECK(book_updates[3].side == md::types::side::buy);
  CHECK_FALSE(book_updates[3].price.has_value());
  CHECK_FALSE(book_updates[3].quantity.has_value());

  CHECK(book_updates[4].side == md::types::side::buy);
  CHECK(book_updates[4].price == md::types::price{10});
  CHECK(book_updates[4].quantity == md::types::quantity{100});

  const auto reports = collect_order_entry<rt::execution_report>(r.order_entry_events);
  CHECK(reports.size() == 6);
  CHECK(reports[0].exec_type == rt::types::exec_type::new_order);
  CHECK(reports[5].exec_type == rt::types::exec_type::new_order);
}
