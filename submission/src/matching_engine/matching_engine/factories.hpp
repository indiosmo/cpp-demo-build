#ifndef MATCHING_ENGINE_FACTORIES_HPP
#define MATCHING_ENGINE_FACTORIES_HPP

#include "matching_engine/order_state.hpp"
#include "order_routing/messages.hpp"

/* Construction helpers for the matching engine, shared between engine and tests. */

namespace matching_engine {

/*
 * Builds the internal order state used as the match() taker and, if a
 * residual remains, as the resting-node payload.
 */
inline order_state make_order_state(const order_routing::new_order& incoming)
{
  return order_state{
    .user = incoming.user,
    .order_id = incoming.order_id,
    .instrument = incoming.instrument,
    .order_side = incoming.order_side,
    .limit_price = incoming.limit_price,
    .remaining_quantity = incoming.order_quantity,
  };
}

} // namespace matching_engine

#endif /* MATCHING_ENGINE_FACTORIES_HPP */
