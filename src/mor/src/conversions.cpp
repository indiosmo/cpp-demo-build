#include "mor/conversions.hpp"

namespace mor {

new_order_single to_mor(const order_entry::new_order_single& request)
{
  return new_order_single{
    .client_id = request.client_id,
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

replace_request to_mor(const order_entry::replace_order& request)
{
  return replace_request{
    .client_id = request.client_id,
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

cancel_request to_mor(const order_entry::cancel_order& request)
{
  return cancel_request{
    .client_id = request.client_id,
    .cl_ord_id = request.cl_ord_id,
    .orig_cl_ord_id = request.orig_cl_ord_id,
  };
}

flush_request to_mor(const order_entry::flush&)
{
  return flush_request{};
}

request to_mor(const order_entry::request& request)
{
  return lab::match(request, [](const auto& routed_request) -> mor::request { return to_mor(routed_request); });
}

execution_report to_mor(const order_entry::execution_report& event)
{
  return execution_report{
    .client_id = event.client_id,
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

cancel_reject to_mor(const order_entry::cancel_reject& event)
{
  return cancel_reject{
    .client_id = event.client_id,
    .cl_ord_id = event.cl_ord_id,
    .orig_cl_ord_id = event.orig_cl_ord_id,
    .reject_reason = event.reject_reason,
    .text = event.text,
    .transact_time = event.transact_time,
  };
}

parser_reject to_mor(const order_entry::rejection& rejection)
{
  return parser_reject{.raw_payload = rejection.raw_payload, .reason = rejection.reason};
}

event to_mor(const order_entry::event& event)
{
  return lab::match(event, [](const auto& routed_event) -> mor::event { return to_mor(routed_event); });
}

order_entry::new_order_single to_order_entry(const new_order_single& request)
{
  return order_entry::new_order_single{
    .client_id = request.client_id,
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

order_entry::replace_order to_order_entry(const replace_request& request)
{
  return order_entry::replace_order{
    .client_id = request.client_id,
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

order_entry::cancel_order to_order_entry(const cancel_request& request)
{
  return order_entry::cancel_order{
    .client_id = request.client_id,
    .cl_ord_id = request.cl_ord_id,
    .orig_cl_ord_id = request.orig_cl_ord_id,
  };
}

order_entry::flush to_order_entry(const flush_request&)
{
  return order_entry::flush{};
}

order_entry::request to_order_entry(const request& request)
{
  return lab::match(request, [](const auto& routed_request) -> order_entry::request { return to_order_entry(routed_request); });
}

order_entry::execution_report to_order_entry(const execution_report& event)
{
  return order_entry::execution_report{
    .client_id = event.client_id,
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

order_entry::cancel_reject to_order_entry(const cancel_reject& event)
{
  return order_entry::cancel_reject{
    .client_id = event.client_id,
    .cl_ord_id = event.cl_ord_id,
    .orig_cl_ord_id = event.orig_cl_ord_id,
    .reject_reason = event.reject_reason,
    .text = event.text,
    .transact_time = event.transact_time,
  };
}

order_entry::rejection to_order_entry(const parser_reject& rejection)
{
  return order_entry::rejection{.raw_payload = rejection.raw_payload, .reason = rejection.reason};
}

} // namespace mor
