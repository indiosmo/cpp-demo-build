#ifndef ORDER_ENTRY_MESSAGES_HPP
#define ORDER_ENTRY_MESSAGES_HPP

#include "order_entry/types.hpp"

#include <optional>
#include <string>
#include <variant>

/*
 * Typed order-entry messages. The JSON decoder maps the demo wire protocol
 * into these values; the matching engine consumes this order-entry surface
 * rather than sharing market-data message types.
 */

namespace order_entry {

struct new_order_single
{
  types::client_id client_id;
  types::cl_ord_id cl_ord_id;
  types::security_id security_id;
  types::symbol symbol;
  types::security_exchange security_exchange;
  types::side side;
  types::ord_type ord_type;
  types::time_in_force time_in_force;
  types::quantity order_qty;
  types::price price;
};

struct replace_order
{
  types::client_id client_id;
  types::cl_ord_id cl_ord_id;
  types::orig_cl_ord_id orig_cl_ord_id;
  types::security_id security_id;
  types::symbol symbol;
  types::security_exchange security_exchange;
  types::side side;
  types::ord_type ord_type;
  types::time_in_force time_in_force;
  types::quantity order_qty;
  types::price price;
};

struct cancel_order
{
  types::client_id client_id;
  types::cl_ord_id cl_ord_id;
  types::orig_cl_ord_id orig_cl_ord_id;
};

struct flush
{
};

using request = std::variant<new_order_single, replace_order, cancel_order, flush>;

struct execution_report
{
  types::client_id client_id;
  types::cl_ord_id cl_ord_id;
  std::optional<types::orig_cl_ord_id> orig_cl_ord_id;
  types::order_id order_id;
  types::exec_id exec_id;
  types::security_id security_id;
  types::symbol symbol;
  types::security_exchange security_exchange;
  types::side side;
  types::ord_type ord_type;
  types::time_in_force time_in_force;
  types::exec_type exec_type;
  types::ord_status ord_status;
  types::quantity order_qty;
  types::quantity cum_qty;
  types::quantity leaves_qty;
  std::optional<types::quantity> last_qty;
  std::optional<types::price> last_px;
  std::optional<types::price> avg_px;
  types::timestamp transact_time;
  std::optional<types::reject_reason> reject_reason;
  std::string text;
};

struct cancel_reject
{
  types::client_id client_id;
  types::cl_ord_id cl_ord_id;
  types::orig_cl_ord_id orig_cl_ord_id;
  types::reject_reason reject_reason;
  std::string text;
  types::timestamp transact_time;
};

using event = std::variant<execution_report, cancel_reject>;

struct rejection
{
  std::string raw_payload;
  std::string reason;
};

} // namespace order_entry

#endif /* ORDER_ENTRY_MESSAGES_HPP */
