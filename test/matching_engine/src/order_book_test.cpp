#include "catch2/catch_test_macros.hpp"
#include "factories.hpp"
#include "matching_engine/order_book.hpp"
#include "matching_engine/order_state.hpp"
#include "order_entry/types.hpp"

#include <memory>
#include <vector>

/*
 * Component tests for the production order-book primitive exposed through
 * matching_engine::order_book.
 *
 * The production book links caller-owned order_node objects. The local
 * fixture owns those nodes and calls the real place(order_node*) /
 * cancel(order_node*) API directly.
 */

namespace {

namespace me = matching_engine;
namespace rt = order_entry;
namespace ft = matching_engine::testing;

using ft::alice;
using ft::bob;
using ft::carol;
using ft::make_order_state;

struct book_fixture
{
  std::vector<std::unique_ptr<me::order_node>> nodes;
  me::order_book book;

  me::order_node* place(const me::order_state& resting)
  {
    auto node = std::make_unique<me::order_node>();
    node->data = resting;
    auto* handle = node.get();
    nodes.push_back(std::move(node));
    book.place(handle);
    return handle;
  }

  void cancel(me::order_node* handle)
  {
    book.cancel(handle);
  }

  [[nodiscard]] std::vector<me::order_state> top_bid_level() const
  {
    std::vector<me::order_state> out;
    for (const auto& node : book.bids().begin()->second.orders) {
      out.push_back(node.data);
    }
    return out;
  }
};

} // namespace

TEST_CASE("order_book - empty book", "[matching_engine][order_book]")
{
  const book_fixture h;

  CHECK_FALSE(h.book.best_bid().has_value());
  CHECK_FALSE(h.book.best_ask().has_value());
  CHECK_FALSE(h.book.total_at_best_bid().has_value());
  CHECK_FALSE(h.book.total_at_best_ask().has_value());
}

TEST_CASE("order_book - place buy", "[matching_engine][order_book][place]")
{
  book_fixture h;

  h.place(make_order_state({.side = rt::types::side::buy, .price{10}, .quantity{100}}));

  CHECK(h.book.best_bid() == rt::types::price{10});
  CHECK(h.book.total_at_best_bid() == rt::types::quantity{100});
  CHECK_FALSE(h.book.best_ask().has_value());
  CHECK_FALSE(h.book.total_at_best_ask().has_value());
}

TEST_CASE("order_book - place sell", "[matching_engine][order_book][place]")
{
  book_fixture h;

  h.place(make_order_state({.side = rt::types::side::sell, .price{12}, .quantity{50}}));

  CHECK(h.book.best_ask() == rt::types::price{12});
  CHECK(h.book.total_at_best_ask() == rt::types::quantity{50});
  CHECK_FALSE(h.book.best_bid().has_value());
  CHECK_FALSE(h.book.total_at_best_bid().has_value());
}

TEST_CASE("order_book - best_bid", "[matching_engine][order_book][best]")
{
  book_fixture h;

  h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .price{9}, .quantity{100}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .price{11}, .quantity{40}}));
  h.place(make_order_state({.client_id = carol, .cl_ord_id{3}, .price{10}, .quantity{75}}));

  CHECK(h.book.best_bid() == rt::types::price{11});
  CHECK(h.book.total_at_best_bid() == rt::types::quantity{40});
}

TEST_CASE("order_book - best_ask", "[matching_engine][order_book][best]")
{
  book_fixture h;

  h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .side = rt::types::side::sell, .price{12}, .quantity{100}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .side = rt::types::side::sell, .price{11}, .quantity{60}}));
  h.place(make_order_state({.client_id = carol, .cl_ord_id{3}, .side = rt::types::side::sell, .price{13}, .quantity{25}}));

  CHECK(h.book.best_ask() == rt::types::price{11});
  CHECK(h.book.total_at_best_ask() == rt::types::quantity{60});
}

TEST_CASE("order_book - total_at_best", "[matching_engine][order_book][best]")
{
  book_fixture h;

  h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .price{10}, .quantity{100}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .price{10}, .quantity{40}}));
  h.place(make_order_state({.client_id = carol, .cl_ord_id{3}, .price{9}, .quantity{999}}));

  CHECK(h.book.best_bid() == rt::types::price{10});
  CHECK(h.book.total_at_best_bid() == rt::types::quantity{140});
}

TEST_CASE("order_book - cancel shrinks level", "[matching_engine][order_book][cancel]")
{
  book_fixture h;

  auto* alice_handle = h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .price{10}, .quantity{100}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .price{10}, .quantity{40}}));

  h.cancel(alice_handle);

  CHECK(h.book.best_bid() == rt::types::price{10});
  CHECK(h.book.total_at_best_bid() == rt::types::quantity{40});
}

TEST_CASE("order_book - cancel erases empty level", "[matching_engine][order_book][cancel]")
{
  book_fixture h;

  auto* alice_handle = h.place(
    make_order_state({.client_id = alice, .cl_ord_id{1}, .side = rt::types::side::sell, .price{12}, .quantity{30}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .side = rt::types::side::sell, .price{14}, .quantity{50}}));

  h.cancel(alice_handle);

  CHECK(h.book.best_ask() == rt::types::price{14});
  CHECK(h.book.total_at_best_ask() == rt::types::quantity{50});
}

TEST_CASE("order_book - cancel either side", "[matching_engine][order_book][cancel]")
{
  book_fixture h;

  auto* alice_handle = h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .price{10}, .quantity{100}}));
  auto* bob_handle =
    h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .side = rt::types::side::sell, .price{12}, .quantity{50}}));

  h.cancel(bob_handle);
  CHECK_FALSE(h.book.best_ask().has_value());

  h.cancel(alice_handle);
  CHECK_FALSE(h.book.best_bid().has_value());
}

TEST_CASE("order_book - cancel preserves FIFO", "[matching_engine][order_book][cancel]")
{
  book_fixture h;

  h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .price{10}, .quantity{100}}));
  auto* bob_handle = h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .price{10}, .quantity{40}}));
  h.place(make_order_state({.client_id = carol, .cl_ord_id{3}, .price{10}, .quantity{25}}));

  h.cancel(bob_handle);

  const auto level = h.top_bid_level();
  REQUIRE(level.size() == 2);
  CHECK(level[0].client_id == alice);
  CHECK(level[0].cl_ord_id == rt::types::cl_ord_id{1});
  CHECK(level[1].client_id == carol);
  CHECK(level[1].cl_ord_id == rt::types::cl_ord_id{3});
}

// Confirms the side-keyed best(side) and best_level(side) accessors dispatch
// to the per-side accessors and surface the same values, including the empty
// case on each side.
TEST_CASE("order_book - side-keyed best and best_level", "[matching_engine][order_book][best]")
{
  book_fixture h;

  CHECK_FALSE(h.book.best(rt::types::side::buy).has_value());
  CHECK_FALSE(h.book.best(rt::types::side::sell).has_value());
  CHECK_FALSE(h.book.best_level(rt::types::side::buy).has_value());
  CHECK_FALSE(h.book.best_level(rt::types::side::sell).has_value());

  h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .side = rt::types::side::buy, .price{10}, .quantity{100}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .side = rt::types::side::buy, .price{10}, .quantity{40}}));
  h.place(make_order_state({.client_id = carol, .cl_ord_id{3}, .side = rt::types::side::sell, .price{12}, .quantity{30}}));

  CHECK(h.book.best(rt::types::side::buy) == rt::types::price{10});
  CHECK(h.book.best(rt::types::side::sell) == rt::types::price{12});

  const auto best_bid_level = h.book.best_level(rt::types::side::buy);
  REQUIRE(best_bid_level.has_value());
  CHECK(best_bid_level->price == rt::types::price{10});
  CHECK(best_bid_level->total_quantity == rt::types::quantity{140});

  const auto best_ask_level = h.book.best_level(rt::types::side::sell);
  REQUIRE(best_ask_level.has_value());
  CHECK(best_ask_level->price == rt::types::price{12});
  CHECK(best_ask_level->total_quantity == rt::types::quantity{30});
}

// Covers running totals at non-top levels: the other test cases only assert
// total_at_best_*, which exercises the head of the side map. This walks both
// full side maps to confirm every level's total_remaining tracks placements,
// shrinking cancels, and the erase-on-empty path.
TEST_CASE("order_book - per-level totals across mutations", "[matching_engine][order_book][totals]")
{
  book_fixture h;

  // bids: two prices, multiple orders at the top, one cancel that shrinks
  // the top level without erasing it.
  h.place(make_order_state({.client_id = alice, .cl_ord_id{1}, .side = rt::types::side::buy, .price{10}, .quantity{100}}));
  auto* bob_bid =
    h.place(make_order_state({.client_id = bob, .cl_ord_id{2}, .side = rt::types::side::buy, .price{10}, .quantity{40}}));
  h.place(make_order_state({.client_id = carol, .cl_ord_id{3}, .side = rt::types::side::buy, .price{9}, .quantity{25}}));
  h.cancel(bob_bid);

  // asks: two prices, one cancel that empties the second level entirely.
  h.place(make_order_state({.client_id = alice, .cl_ord_id{4}, .side = rt::types::side::sell, .price{12}, .quantity{60}}));
  auto* carol_ask = h.place(
    make_order_state({.client_id = carol, .cl_ord_id{5}, .side = rt::types::side::sell, .price{13}, .quantity{30}}));
  h.place(make_order_state({.client_id = bob, .cl_ord_id{6}, .side = rt::types::side::sell, .price{13}, .quantity{20}}));
  h.cancel(carol_ask);

  const auto expected_bids = std::vector<std::pair<rt::types::price, rt::types::quantity>>{
    {rt::types::price{10}, rt::types::quantity{100}},
    {rt::types::price{9}, rt::types::quantity{25}},
  };
  const auto expected_asks = std::vector<std::pair<rt::types::price, rt::types::quantity>>{
    {rt::types::price{12}, rt::types::quantity{60}},
    {rt::types::price{13}, rt::types::quantity{20}},
  };

  auto actual_bids = std::vector<std::pair<rt::types::price, rt::types::quantity>>{};
  for (const auto& [price, level] : h.book.bids()) {
    actual_bids.emplace_back(price, level.total_remaining);
  }
  auto actual_asks = std::vector<std::pair<rt::types::price, rt::types::quantity>>{};
  for (const auto& [price, level] : h.book.asks()) {
    actual_asks.emplace_back(price, level.total_remaining);
  }

  CHECK(actual_bids == expected_bids);
  CHECK(actual_asks == expected_asks);
}
