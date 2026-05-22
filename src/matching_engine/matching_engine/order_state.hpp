#ifndef MATCHING_ENGINE_ORDER_STATE_HPP
#define MATCHING_ENGINE_ORDER_STATE_HPP

#include "order_entry/types.hpp"

/*
 * Internal resting order. Carries the symbol so the engine can
 * recover the owning book from a (client_id, cl_ord_id) lookup without
 * scanning every book.
 */

namespace matching_engine {

struct order_state
{
  order_entry::types::client_id client_id;
  order_entry::types::cl_ord_id cl_ord_id;
  order_entry::types::security_id security_id;
  order_entry::types::symbol symbol;
  order_entry::types::security_exchange security_exchange;
  order_entry::types::side side;
  order_entry::types::ord_type ord_type;
  order_entry::types::time_in_force time_in_force;
  order_entry::types::price price;
  order_entry::types::quantity order_qty;
  order_entry::types::quantity leaves_qty;
};

} // namespace matching_engine

#endif /* MATCHING_ENGINE_ORDER_STATE_HPP */
