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
 * matching_engine::engine. The black-box CSV harness under test/ exercises
 * every protocol-level scenario byte-for-byte against the recorded outputs;
 * this file keeps one in-process scenario for fast iteration through the
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
namespace rt = order_routing;
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

  recorder()
    : engine{default_config()}
  {
    engine.on_event = [this](const md::message& ev) { events.push_back(ev); };
  }

  void send_order(const ft::order_params& params)
  {
    engine.send(ft::make_new_order(params));
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

} // namespace

TEST_CASE("matching_engine - cross, eliminate, fresh rest", "[matching_engine][engine]")
{
  recorder r;

  // Seed a bid, an ask, and a lower bid:
  //   alice 1 buys 100 @ 10  -> new best bid
  //   alice 2 sells 100 @ 12 -> new best ask
  //   bob   102 sells 100 @ 11 -> tightens the ask side
  r.send_order({.order_id{1}, .order_side = rt::types::side::buy, .limit_price{10}});
  r.send_order({.order_id{2}, .order_side = rt::types::side::sell, .limit_price{12}});
  r.send_order({.user = bob, .order_id{102}, .order_side = rt::types::side::sell, .limit_price{11}});

  // Aggressing sell at 10 -- crosses alice 1 fully, empties the bid side.
  r.send_order({.user = bob, .order_id{103}, .order_side = rt::types::side::sell, .limit_price{10}});

  // Fresh resting buy at the new best.
  r.send_order({.order_id{3}, .order_side = rt::types::side::buy, .limit_price{10}});

  // One trade only: bob 103 lifts alice 1 at 10 for 100.
  const auto trades = collect<md::trade>(r.events);
  REQUIRE(trades.size() == 1);
  CHECK(trades[0].buy_user == alice);
  CHECK(trades[0].buy_order == rt::types::user_order_id{1});
  CHECK(trades[0].sell_user == bob);
  CHECK(trades[0].sell_order == rt::types::user_order_id{103});
  CHECK(trades[0].trade_price == rt::types::price{10});
  CHECK(trades[0].trade_quantity == rt::types::quantity{100});

  // Five top-of-book emissions in order:
  //   B 10/100  (alice 1 rests at the new best bid)
  //   S 12/100  (alice 2 rests at the new best ask)
  //   S 11/100  (bob 102 tightens the ask)
  //   B -/-     (cross consumes alice 1; bid side eliminated)
  //   B 10/100  (alice 3 rests at the new best bid)
  const auto tops = collect<md::top_of_book>(r.events);
  REQUIRE(tops.size() == 5);

  CHECK(tops[0].book_side == md::types::side::buy);
  CHECK(tops[0].top_price == rt::types::price{10});
  CHECK(tops[0].top_quantity == rt::types::quantity{100});

  CHECK(tops[1].book_side == md::types::side::sell);
  CHECK(tops[1].top_price == rt::types::price{12});
  CHECK(tops[1].top_quantity == rt::types::quantity{100});

  CHECK(tops[2].book_side == md::types::side::sell);
  CHECK(tops[2].top_price == rt::types::price{11});
  CHECK(tops[2].top_quantity == rt::types::quantity{100});

  CHECK(tops[3].book_side == md::types::side::buy);
  CHECK_FALSE(tops[3].top_price.has_value());
  CHECK_FALSE(tops[3].top_quantity.has_value());

  CHECK(tops[4].book_side == md::types::side::buy);
  CHECK(tops[4].top_price == rt::types::price{10});
  CHECK(tops[4].top_quantity == rt::types::quantity{100});

  // Acks: one per new_order request that the engine accepted.
  CHECK(collect<md::order_ack>(r.events).size() == 5);
}
