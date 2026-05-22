#ifndef MATCHING_ENGINE_FACTORIES_HPP
#define MATCHING_ENGINE_FACTORIES_HPP

#include "matching_engine/order_state.hpp"
#include "order_entry/messages.hpp"

/* Construction helpers for the matching engine, shared between engine and tests. */

namespace matching_engine {

/*
 * Builds the internal order state used as the match() taker and, if a
 * residual remains, as the resting-node payload.
 */
inline order_state make_order_state(const order_entry::new_order_single& incoming)
{
  return order_state{
    .client_id = incoming.client_id,
    .cl_ord_id = incoming.cl_ord_id,
    .security_id = incoming.security_id,
    .symbol = incoming.symbol,
    .security_exchange = incoming.security_exchange,
    .side = incoming.side,
    .ord_type = incoming.ord_type,
    .time_in_force = incoming.time_in_force,
    .price = incoming.price,
    .order_qty = incoming.order_qty,
    .leaves_qty = incoming.order_qty,
  };
}

} // namespace matching_engine

#endif /* MATCHING_ENGINE_FACTORIES_HPP */
