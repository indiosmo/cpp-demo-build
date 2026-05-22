#include "order_client/json_encoder.hpp"

#include "lab/json.hpp"
#include "lab/variant.hpp"

#include <cstdlib>

namespace order_client {

std::string json_encoder::encode(const order_entry::new_order_single& order) const
{
  lab::json::value payload{
    {"message_type", "new_order_single"},
    {"client_id", order.client_id},
    {"cl_ord_id", order.cl_ord_id},
    {"security_id", order.security_id},
    {"symbol", order.symbol},
    {"security_exchange", order.security_exchange},
    {"side", encode_side(order.side)},
    {"ord_type", encode_ord_type(order.ord_type)},
    {"time_in_force", encode_time_in_force(order.time_in_force)},
    {"order_qty", order.order_qty},
    {"price", order.price},
  };
  return lab::json::dump(payload);
}

std::string json_encoder::encode(const order_entry::replace_order& replace) const
{
  lab::json::value payload{
    {"message_type", "replace_order"},
    {"client_id", replace.client_id},
    {"cl_ord_id", replace.cl_ord_id},
    {"orig_cl_ord_id", replace.orig_cl_ord_id},
    {"security_id", replace.security_id},
    {"symbol", replace.symbol},
    {"security_exchange", replace.security_exchange},
    {"side", encode_side(replace.side)},
    {"ord_type", encode_ord_type(replace.ord_type)},
    {"time_in_force", encode_time_in_force(replace.time_in_force)},
    {"order_qty", replace.order_qty},
    {"price", replace.price},
  };
  return lab::json::dump(payload);
}

std::string json_encoder::encode(const order_entry::cancel_order& cancel) const
{
  lab::json::value payload{
    {"message_type", "cancel_order"},
    {"client_id", cancel.client_id},
    {"cl_ord_id", cancel.cl_ord_id},
    {"orig_cl_ord_id", cancel.orig_cl_ord_id},
  };
  return lab::json::dump(payload);
}

std::string json_encoder::encode(const order_entry::flush&) const
{
  return lab::json::dump(lab::json::value{{"message_type", "flush"}});
}

std::string json_encoder::encode(const order_entry::request& request) const
{
  return lab::match(request, [this](const auto& command) { return encode(command); });
}

const char* json_encoder::encode_side(order_entry::types::side side)
{
  switch (side) {
    case order_entry::types::side::buy:
      return "buy";
    case order_entry::types::side::sell:
      return "sell";
  }

  std::terminate();
}

const char* json_encoder::encode_ord_type(order_entry::types::ord_type ord_type)
{
  switch (ord_type) {
    case order_entry::types::ord_type::market:
      return "market";
    case order_entry::types::ord_type::limit:
      return "limit";
  }

  std::terminate();
}

const char* json_encoder::encode_time_in_force(order_entry::types::time_in_force time_in_force)
{
  switch (time_in_force) {
    case order_entry::types::time_in_force::day:
      return "day";
    case order_entry::types::time_in_force::ioc:
      return "ioc";
  }

  std::terminate();
}

} // namespace order_client
