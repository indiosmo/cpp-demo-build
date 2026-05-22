#ifndef MATCHING_ENGINE_V3_ENGINE_HPP
#define MATCHING_ENGINE_V3_ENGINE_HPP

#include "boost/pool/pool.hpp"
#include "market_data/messages.hpp"
#include "matching_engine/engine_config.hpp"
#include "matching_engine/order_state.hpp"
#include "matching_engine/types.hpp"
#include "matching_engine/v3/matching.hpp"
#include "matching_engine/v3/order_book.hpp"
#include "matching_engine/v3/order_node.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include "lab/inplace_function.hpp"
#include "lab/result.hpp"

// GCC -O2/-O3 raises a false-positive -Wnull-dereference inside
// Boost.Unordered's iterator inlining; suppress just for these headers.
// see https://github.com/boostorg/unordered/issues/331
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "boost/unordered/unordered_flat_map.hpp"
#include "boost/unordered/unordered_node_map.hpp"
#pragma GCC diagnostic pop

/*
 * Matching engine. Owns the per-symbol order books, the cross-symbol
 * (user, user_order_id) identity index, and the boost::pool backing
 * every resting order_node. Pure synchronous code over value types;
 * the runtime shell drives send() on one thread and marshals on_event
 * onto the output loop.
 *
 * send() dispatches through lab::match to the typed handle()
 * overloads. on_event must be wired before send().
 *
 * The matching algorithm itself lives in matching_engine/v3/matching.hpp
 * as a side-agnostic function template.
 *
 * Matching logic runs at the engine level and not the symbols level
 * to allow eventual cross-symbol trading support (e.g. implied futures).
 */

namespace matching_engine::v3 {

class engine
{
public:
  lab::inplace_function<void(const market_data::message&)> on_event;

  explicit engine(engine_config configuration);
  engine(const engine&) = delete;
  engine& operator=(const engine&) = delete;
  engine(engine&&) = delete;
  engine& operator=(engine&&) = delete;
  ~engine() = default;

  void send(const order_routing::request&);

private:
  void handle(const order_routing::new_order&);
  void handle(const order_routing::cancel_order&);
  void handle(const order_routing::flush&);

  /*
   * Leaf-shaped body for handle(new_order); the outer handle() owns the
   * error-handling frame and maps structured errors to log lines.
   */
  lab::result<void> handle_new_order_impl(const order_routing::new_order&);

  lab::result<void> check_duplicate(const types::order_key& key) const;
  lab::result<flat_order_book*> find_book(order_routing::types::symbol instrument);

  /*
   * Cross `taker` against the opposite side of `book`, emitting trades and
   * releasing fully-filled makers through the engine's on_event/release path.
   */
  execution_summary match_orders(flat_order_book& book, order_state& taker);

  void on_trade(const market_data::trade&);
  void on_release(order_node* consumed);

  void emit_top_of_book(const flat_order_book&, order_routing::types::side);

  /* Throws std::bad_alloc if the pool cannot grow. */
  order_node* allocate_node(const order_state& data);

  /* Warm the pool so startup pays for the expected resting-order capacity. */
  void reserve_node_pool(std::size_t expected_resting_orders);

  /* Precondition: node is unlinked from every intrusive list. */
  void release_node(order_node* node);

  /*
   * Shared pool over per-book pools: slots freed by a shallow symbol
   * are reusable by a deep one, so resident memory tracks the total
   * resting count rather than the sum of per-symbol caps.
   *
   * Declared before books_ so the books tear down first and the pool
   * outlives every node it handed out (see order_node.hpp).
   */
  boost::pool<> node_pool_;

  /*
   * Node map (not flat): the matching loop holds a reference to a book
   * across on_event calls, so references must survive rehashes.
   */
  boost::unordered_node_map<order_routing::types::symbol, flat_order_book> books_;

  /*
   * Cross-symbol identity index. Flat (open-addressed) over node-based:
   * the stored order_node* is already address-stable through the pool,
   * so the flat layout's denser buckets win on cache.
   */
  boost::unordered_flat_map<types::order_key, order_node*> resting_index_;
};

} // namespace matching_engine::v3

#endif /* MATCHING_ENGINE_V3_ENGINE_HPP */
