#ifndef ORDER_ROUTING_MESSAGES_HPP
#define ORDER_ROUTING_MESSAGES_HPP

#include "order_routing/types.hpp"

#include <string>
#include <variant>

/*
 * Typed requests produced by the decoder from CSV wire bytes, mirroring
 * the protocol in EXERCISE.md:
 *   N, userId, symbol, price, quantity, side, userOrderId  -> new_order
 *   C, userId, userOrderId                                 -> cancel_order
 *   F                                                      -> flush
 *
 * Market orders are signalled by limit_price == 0 (IOC contract); the
 * matching engine branches on the value rather than carrying a tag.
 */

namespace order_routing {

struct new_order
{
  types::user_id user;
  types::user_order_id order_id;
  types::symbol instrument;
  types::side order_side;
  types::price limit_price;
  types::quantity order_quantity;
};

struct cancel_order
{
  types::user_id user;
  types::user_order_id order_id;
};

struct flush
{
};

using request = std::variant<new_order, cancel_order, flush>;

struct rejection
{
  std::string raw_payload;
  std::string reason;
};

} // namespace order_routing

#endif /* ORDER_ROUTING_MESSAGES_HPP */
