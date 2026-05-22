#ifndef MATCHING_ENGINE_V3_ORDER_BOOK_HPP
#define MATCHING_ENGINE_V3_ORDER_BOOK_HPP

#include "boost/container/flat_map.hpp"
#include "boost/intrusive/list.hpp"
#include "matching_engine/v3/order_node.hpp"
#include "order_entry/types.hpp"

#include "lab/assert.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <type_traits>

/*
 * Per-symbol order book, v3: structural layer over engine-owned
 * order_nodes, linked into per-price-level intrusive lists. Defaulted
 * destructor; teardown safety argument in order_node.hpp.
 *
 * Non-copyable, non-movable: intrusive hooks point back into pool
 * storage, so any copy would alias node ownership.
 *
 * Templated on the side-map type. Production uses flat_order_book over
 * std_order_book; std_order_book stays in-tree as a benchmark reference.
 */

namespace matching_engine::v3 {

using orders_list = boost::intrusive::list<order_node>;

/*
 * Per-price-level storage: the FIFO of resting orders plus a running
 * total of their remaining quantity. The total is maintained at every
 * mutation point so total_at_best() is a single read instead of a walk
 * of the level (it is called on every mbo_book_update emission).
 */
struct price_level
{
  orders_list orders;
  order_entry::types::quantity total_remaining{0};
};

/*
 * Value-typed snapshot of a side's top level. Returned by
 * best_level(side) so callers get price and aggregate quantity in one
 * fetch without dipping into the per-level storage.
 */
struct level_snapshot
{
  order_entry::types::price price;
  order_entry::types::quantity total_quantity;
};

namespace detail {

template <template <class...> class SideMap>
class basic_order_book
{
public:
  using bids_map = SideMap<order_entry::types::price, price_level, std::greater<>>;
  using asks_map = SideMap<order_entry::types::price, price_level, std::less<>>;

  // The two sides differ only in comparator; lock that in so they cannot
  // drift on either key type or per-level storage.
  static_assert(std::is_same_v<typename bids_map::key_type, order_entry::types::price>);
  static_assert(std::is_same_v<typename asks_map::key_type, order_entry::types::price>);
  static_assert(std::is_same_v<typename bids_map::mapped_type, price_level>);
  static_assert(std::is_same_v<typename asks_map::mapped_type, price_level>);

  basic_order_book() = default;
  ~basic_order_book() = default;

  basic_order_book(const basic_order_book&) = delete;
  basic_order_book& operator=(const basic_order_book&) = delete;
  basic_order_book(basic_order_book&&) = delete;
  basic_order_book& operator=(basic_order_book&&) = delete;

  // flat_map may move its values when it grows, so the per-level entry must
  // be move-constructible.
  static_assert(std::is_move_constructible_v<price_level>);

  /*
   * Node address stays stable for the resting order's lifetime; the engine's
   * identity index holds raw order_node* and relies on this.
   */
  void place(order_node* node)
  {
    LAB_ASSERT(node != nullptr);

    switch (node->data.side) {
      case order_entry::types::side::buy:
        place_into(bids_, node);
        break;

      case order_entry::types::side::sell:
        place_into(asks_, node);
        break;
    }
  }

  void cancel(order_node* node)
  {
    LAB_ASSERT(node != nullptr);

    switch (node->data.side) {
      case order_entry::types::side::buy:
        cancel_from(bids_, node);
        break;

      case order_entry::types::side::sell:
        cancel_from(asks_, node);
        break;
    }
  }

  [[nodiscard]] std::optional<order_entry::types::price> best_bid() const
  {
    return best_price(bids_);
  }

  [[nodiscard]] std::optional<order_entry::types::price> best_ask() const
  {
    return best_price(asks_);
  }

  [[nodiscard]] std::optional<order_entry::types::quantity> total_at_best_bid() const
  {
    return total_at_best(bids_);
  }

  [[nodiscard]] std::optional<order_entry::types::quantity> total_at_best_ask() const
  {
    return total_at_best(asks_);
  }

  /*
   * Side-keyed accessors. The bid and ask maps differ in comparator
   * type, so a single side_map(side) returning a common reference is
   * not expressible; the two-arm ternary keeps the dispatch local.
   */
  [[nodiscard]] std::optional<order_entry::types::price> best(order_entry::types::side s) const
  {
    return s == order_entry::types::side::buy ? best_bid() : best_ask();
  }

  [[nodiscard]] std::optional<level_snapshot> best_level(order_entry::types::side s) const
  {
    return s == order_entry::types::side::buy ? best_level_of(bids_) : best_level_of(asks_);
  }

  // clang-format off
  [[nodiscard]] auto& bids() noexcept { return bids_; }
  [[nodiscard]] const auto& bids() const noexcept { return bids_; }

  [[nodiscard]] auto& asks() noexcept { return asks_; }
  [[nodiscard]] const auto& asks() const noexcept { return asks_; }

  // clang-format on

private:
  /* Side helpers are templated because bids_ and asks_ differ only in comparator. */
  template <typename Side>
  static void place_into(Side& side_map, order_node* node)
  {
    // append to the price level's FIFO (creating the level on first sight)
    // and bump its running total.
    auto& level = side_map[node->data.price];
    level.orders.push_back(*node);
    level.total_remaining += node->data.leaves_qty;
  }

  template <typename Side>
  static void cancel_from(Side& side_map, order_node* node)
  {
    const auto level_it = side_map.find(node->data.price);
    LAB_ASSERT(level_it != side_map.end());

    // Single-order cancel after the engine has located one resting node in
    // its identity index; not invoked while match() iterates the side map,
    // so erasing the just-emptied level here is iterator-safe.
    auto& level = level_it->second;
    level.total_remaining -= node->data.leaves_qty;
    level.orders.erase(level.orders.iterator_to(*node));
    if (level.orders.empty()) {
      side_map.erase(level_it);
    }
  }

  template <typename Side>
  static std::optional<order_entry::types::price> best_price(const Side& side_map)
  {
    if (!side_map.empty()) {
      return side_map.begin()->first;
    } else {
      return std::nullopt;
    }
  }

  template <typename Side>
  static std::optional<order_entry::types::quantity> total_at_best(const Side& side_map)
  {
    if (!side_map.empty()) {
      return side_map.begin()->second.total_remaining;
    } else {
      return std::nullopt;
    }
  }

  template <typename Side>
  static std::optional<level_snapshot> best_level_of(const Side& side_map)
  {
    if (side_map.empty()) {
      return std::nullopt;
    }
    const auto& entry = *side_map.begin();
    return level_snapshot{.price = entry.first, .total_quantity = entry.second.total_remaining};
  }

  bids_map bids_;
  asks_map asks_;
};

} // namespace detail

using std_order_book = detail::basic_order_book<std::map>;
using flat_order_book = detail::basic_order_book<boost::container::flat_map>;

} // namespace matching_engine::v3

#endif /* MATCHING_ENGINE_V3_ORDER_BOOK_HPP */
