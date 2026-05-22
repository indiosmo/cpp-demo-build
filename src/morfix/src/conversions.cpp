#include "morfix/conversions.hpp"

#include "lab/variant.hpp"

namespace morfix {

new_order_single to_fix(const mor::new_order_single& request)
{
  return new_order_single{
    .account = request.client_id,
    .cl_ord_id = request.cl_ord_id,
    .security_id = request.security_id,
    .symbol = request.symbol,
    .security_exchange = request.security_exchange,
    .side = request.side,
    .ord_type = request.ord_type,
    .time_in_force = request.time_in_force,
    .order_qty = request.order_qty,
    .price = request.price,
  };
}

order_cancel_replace_request to_fix(const mor::replace_request& request)
{
  return order_cancel_replace_request{
    .account = request.client_id,
    .cl_ord_id = request.cl_ord_id,
    .orig_cl_ord_id = request.orig_cl_ord_id,
    .security_id = request.security_id,
    .symbol = request.symbol,
    .security_exchange = request.security_exchange,
    .side = request.side,
    .ord_type = request.ord_type,
    .time_in_force = request.time_in_force,
    .order_qty = request.order_qty,
    .price = request.price,
  };
}

order_cancel_request to_fix(const mor::cancel_request& request)
{
  return order_cancel_request{
    .account = request.client_id,
    .cl_ord_id = request.cl_ord_id,
    .orig_cl_ord_id = request.orig_cl_ord_id,
  };
}

std::optional<request> to_fix(const mor::request& request)
{
  return lab::match(
    request,
    [](const mor::new_order_single& routed_request) -> std::optional<morfix::request> { return to_fix(routed_request); },
    [](const mor::replace_request& routed_request) -> std::optional<morfix::request> { return to_fix(routed_request); },
    [](const mor::cancel_request& routed_request) -> std::optional<morfix::request> { return to_fix(routed_request); },
    [](const mor::flush_request&) -> std::optional<morfix::request> { return std::nullopt; });
}

execution_report to_fix(const mor::execution_report& event)
{
  return execution_report{
    .account = event.client_id,
    .cl_ord_id = event.cl_ord_id,
    .orig_cl_ord_id = event.orig_cl_ord_id,
    .order_id = event.order_id,
    .exec_id = event.exec_id,
    .security_id = event.security_id,
    .symbol = event.symbol,
    .security_exchange = event.security_exchange,
    .side = event.side,
    .ord_type = event.ord_type,
    .time_in_force = event.time_in_force,
    .exec_type = event.exec_type,
    .ord_status = event.ord_status,
    .order_qty = event.order_qty,
    .cum_qty = event.cum_qty,
    .leaves_qty = event.leaves_qty,
    .last_qty = event.last_qty,
    .last_px = event.last_px,
    .avg_px = event.avg_px,
    .transact_time = event.transact_time,
    .reject_reason = event.reject_reason,
    .text = event.text,
  };
}

order_cancel_reject to_fix(const mor::cancel_reject& event)
{
  return order_cancel_reject{
    .account = event.client_id,
    .cl_ord_id = event.cl_ord_id,
    .orig_cl_ord_id = event.orig_cl_ord_id,
    .reject_reason = event.reject_reason,
    .text = event.text,
    .transact_time = event.transact_time,
  };
}

std::optional<event> to_fix(const mor::event& event)
{
  return lab::match(
    event,
    [](const mor::execution_report& routed_event) -> std::optional<morfix::event> { return to_fix(routed_event); },
    [](const mor::cancel_reject& routed_event) -> std::optional<morfix::event> { return to_fix(routed_event); },
    [](const mor::parser_reject&) -> std::optional<morfix::event> { return std::nullopt; });
}

mor::new_order_single to_mor(const new_order_single& request)
{
  return mor::new_order_single{
    .client_id = request.account,
    .cl_ord_id = request.cl_ord_id,
    .security_id = request.security_id,
    .symbol = request.symbol,
    .security_exchange = request.security_exchange,
    .side = request.side,
    .ord_type = request.ord_type,
    .time_in_force = request.time_in_force,
    .order_qty = request.order_qty,
    .price = request.price,
  };
}

mor::replace_request to_mor(const order_cancel_replace_request& request)
{
  return mor::replace_request{
    .client_id = request.account,
    .cl_ord_id = request.cl_ord_id,
    .orig_cl_ord_id = request.orig_cl_ord_id,
    .security_id = request.security_id,
    .symbol = request.symbol,
    .security_exchange = request.security_exchange,
    .side = request.side,
    .ord_type = request.ord_type,
    .time_in_force = request.time_in_force,
    .order_qty = request.order_qty,
    .price = request.price,
  };
}

mor::cancel_request to_mor(const order_cancel_request& request)
{
  return mor::cancel_request{
    .client_id = request.account,
    .cl_ord_id = request.cl_ord_id,
    .orig_cl_ord_id = request.orig_cl_ord_id,
  };
}

mor::request to_mor(const request& request)
{
  return lab::match(request, [](const auto& fix_request) -> mor::request { return to_mor(fix_request); });
}

mor::execution_report to_mor(const execution_report& event)
{
  return mor::execution_report{
    .client_id = event.account,
    .cl_ord_id = event.cl_ord_id,
    .orig_cl_ord_id = event.orig_cl_ord_id,
    .order_id = event.order_id,
    .exec_id = event.exec_id,
    .security_id = event.security_id,
    .symbol = event.symbol,
    .security_exchange = event.security_exchange,
    .side = event.side,
    .ord_type = event.ord_type,
    .time_in_force = event.time_in_force,
    .exec_type = event.exec_type,
    .ord_status = event.ord_status,
    .order_qty = event.order_qty,
    .cum_qty = event.cum_qty,
    .leaves_qty = event.leaves_qty,
    .last_qty = event.last_qty,
    .last_px = event.last_px,
    .avg_px = event.avg_px,
    .transact_time = event.transact_time,
    .reject_reason = event.reject_reason,
    .text = event.text,
  };
}

mor::cancel_reject to_mor(const order_cancel_reject& event)
{
  return mor::cancel_reject{
    .client_id = event.account,
    .cl_ord_id = event.cl_ord_id,
    .orig_cl_ord_id = event.orig_cl_ord_id,
    .reject_reason = event.reject_reason,
    .text = event.text,
    .transact_time = event.transact_time,
  };
}

mor::event to_mor(const event& event)
{
  return lab::match(event, [](const auto& fix_event) -> mor::event { return to_mor(fix_event); });
}

} // namespace morfix
