#ifndef MATCHING_ENGINE_V2_ENGINE_HPP
#define MATCHING_ENGINE_V2_ENGINE_HPP

#include "boost/pool/pool.hpp"
#include "market_data/messages.hpp"
#include "matching_engine/engine_config.hpp"
#include "matching_engine/order_state.hpp"
#include "matching_engine/types.hpp"
#include "matching_engine/v2/order_book.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include "lab/inplace_function.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "boost/unordered/unordered_flat_map.hpp"
#include "boost/unordered/unordered_node_map.hpp"
#pragma GCC diagnostic pop

/*
 * Matching engine, v2: intrusive-list price levels over an
 * engine-owned shared boost::pool; the identity index stores handles
 * returned by place(). Baseline for the v3 comparison.
 */

namespace matching_engine::v2 {

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

  void send(const order_entry::request&);

private:
  void handle(const order_entry::new_order_single&);
  void handle(const order_entry::cancel_order&);
  void handle(const order_entry::flush&);

  void match_buy(order_book&, const order_entry::new_order_single&, order_entry::types::quantity& remaining);
  void match_sell(order_book&, const order_entry::new_order_single&, order_entry::types::quantity& remaining);
  void emit_top_of_book(const order_book&, order_entry::types::side);

  boost::pool<> node_pool_;
  boost::unordered_node_map<order_entry::types::symbol, order_book> books_;
  boost::unordered_flat_map<types::order_key, order_node*> resting_index_;
};

} // namespace matching_engine::v2

#endif /* MATCHING_ENGINE_V2_ENGINE_HPP */
