#ifndef MATCHING_ENGINE_V1_ENGINE_HPP
#define MATCHING_ENGINE_V1_ENGINE_HPP

#include "market_data/messages.hpp"
#include "matching_engine/engine_config.hpp"
#include "matching_engine/order_state.hpp"
#include "matching_engine/types.hpp"
#include "matching_engine/v1/order_book.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include "kraken/inplace_function.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "boost/unordered/unordered_flat_map.hpp"
#include "boost/unordered/unordered_node_map.hpp"
#pragma GCC diagnostic pop

/*
 * Matching engine, v1: vector-backed price levels with an in-place
 * matching loop. Baseline for the v2/v3 comparison.
 */

namespace matching_engine::v1 {

class engine
{
public:
  kraken::inplace_function<void(const market_data::message&)> on_event;

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

  void match_buy(order_book&, order_state& taker);
  void match_sell(order_book&, order_state& taker);
  void emit_top_of_book(const order_book&, order_routing::types::side);

  boost::unordered_node_map<order_routing::types::symbol, order_book> books_;
  boost::unordered_flat_map<types::order_key, order_routing::types::symbol> resting_index_;
};

} // namespace matching_engine::v1

#endif /* MATCHING_ENGINE_V1_ENGINE_HPP */
