#ifndef MORFIX_MESSAGES_HPP
#define MORFIX_MESSAGES_HPP

#include "mor/messages.hpp"

#include <optional>
#include <string>
#include <variant>

namespace morfix {

namespace types = mor::types;

enum class msg_type
{
  new_order_single,
  order_cancel_replace_request,
  order_cancel_request,
  execution_report,
  order_cancel_reject,
};

struct new_order_single
{
  types::client_id account;
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

struct order_cancel_replace_request
{
  types::client_id account;
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

struct order_cancel_request
{
  types::client_id account;
  types::cl_ord_id cl_ord_id;
  types::orig_cl_ord_id orig_cl_ord_id;
};

using request = std::variant<new_order_single, order_cancel_replace_request, order_cancel_request>;

struct execution_report
{
  types::client_id account;
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

struct order_cancel_reject
{
  types::client_id account;
  types::cl_ord_id cl_ord_id;
  types::orig_cl_ord_id orig_cl_ord_id;
  types::reject_reason reject_reason;
  std::string text;
  types::timestamp transact_time;
};

using event = std::variant<execution_report, order_cancel_reject>;

} // namespace morfix

#endif /* MORFIX_MESSAGES_HPP */
