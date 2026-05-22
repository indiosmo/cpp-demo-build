#ifndef MATCHING_ENGINE_ORDER_STATE_HPP
#define MATCHING_ENGINE_ORDER_STATE_HPP

#include "order_routing/types.hpp"

/*
 * Internal resting order. Carries the instrument so the engine can
 * recover the owning book from a (user, user_order_id) lookup without
 * scanning every book.
 */

namespace matching_engine {

struct order_state
{
  order_routing::types::user_id user;
  order_routing::types::user_order_id order_id;
  order_routing::types::symbol instrument;
  order_routing::types::side order_side;
  order_routing::types::price limit_price;
  order_routing::types::quantity remaining_quantity;
};

} // namespace matching_engine

#endif /* MATCHING_ENGINE_ORDER_STATE_HPP */
