#ifndef MATCHING_ENGINE_V2_ORDER_BOOK_HPP
#define MATCHING_ENGINE_V2_ORDER_BOOK_HPP

#include "boost/intrusive/list.hpp"
#include "boost/intrusive/list_hook.hpp"
#include "boost/pool/pool.hpp"
#include "matching_engine/order_state.hpp"
#include "matching_engine/types.hpp"
#include "order_routing/types.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>

/*
 * Per-symbol order book, v2: intrusive-list-with-pool layout from
 * ADR 0004. Price-keyed map per side (desc bids, asc asks); each level
 * an intrusive list of pool-owned nodes in arrival order.
 *
 * Pool is held by reference and outlives the book (engine-owned in
 * production). Place placement-news from the pool; cancel runs the
 * destructor and returns the slot. Destructor walks every resting
 * node to keep intrusive invariants intact before the pool reclaims.
 *
 * Non-copyable, non-movable: intrusive hooks point back into pool
 * storage, so any copy would alias node ownership.
 */

namespace matching_engine::v2 {

struct order_node : boost::intrusive::list_base_hook<>
{
  order_state data;
};

using level_list = boost::intrusive::list<order_node>;

class order_book
{
public:
  using bids_map = std::map<order_routing::types::price, level_list, std::greater<>>;
  using asks_map = std::map<order_routing::types::price, level_list, std::less<>>;

  /* Precondition: node_pool outlives this book. */
  explicit order_book(boost::pool<>& node_pool);
  ~order_book();

  order_book(const order_book&) = delete;
  order_book& operator=(const order_book&) = delete;
  order_book(order_book&&) = delete;
  order_book& operator=(order_book&&) = delete;

  /* Returned handle stays valid for the resting order's lifetime. */
  order_node* place(const order_state& resting);

  void cancel(order_node* handle);

  [[nodiscard]] std::optional<order_routing::types::price> best_bid() const;
  [[nodiscard]] std::optional<order_routing::types::price> best_ask() const;
  [[nodiscard]] std::optional<order_routing::types::quantity> total_at_best_bid() const;
  [[nodiscard]] std::optional<order_routing::types::quantity> total_at_best_ask() const;

  /* Precondition: corresponding side non-empty (KRAKEN_ASSERTed). */
  [[nodiscard]] const order_state& top_bid_front() const;
  [[nodiscard]] const order_state& top_ask_front() const;

  /*
   * Reduces the top-of-side front by `fill`. Returns the consumed
   * order's key on full fill, nullopt otherwise.
   * Precondition: fill <= front.remaining_quantity.
   */
  std::optional<types::order_key> fill_top_bid_front(order_routing::types::quantity fill);
  std::optional<types::order_key> fill_top_ask_front(order_routing::types::quantity fill);

  [[nodiscard]] const bids_map& bids() const
  {
    return bids_;
  }

  [[nodiscard]] const asks_map& asks() const
  {
    return asks_;
  }

private:
  /* Throws std::bad_alloc when the pool cannot grow. */
  order_node* allocate_node(const order_state& data);

  /* Precondition: node is no longer linked into any list. */
  void release_node(order_node* node);

  void release_level(level_list& level);

  /*
   * Raw-memory pool rather than a typed object pool: the typed variant
   * keeps its free list address-sorted (O(N) per allocation), while the
   * raw-memory one is O(1). The trade-off is that this book runs
   * placement-new and the destructor itself on every place/cancel.
   */
  boost::pool<>& pool_;
  bids_map bids_;
  asks_map asks_;
};

} // namespace matching_engine::v2

#endif /* MATCHING_ENGINE_V2_ORDER_BOOK_HPP */
