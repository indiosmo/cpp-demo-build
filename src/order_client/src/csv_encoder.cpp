#include "order_client/csv_encoder.hpp"

#include "lab/fmt.hpp"
#include "lab/variant.hpp"

#include <cstdlib>

namespace order_client {

std::string csv_encoder::encode(const order_entry::new_order_single& order) const
{
  return fmt::format(
    "N,{},{},{},{},{},{}",
    order.client_id,
    order.symbol,
    order.price,
    order.order_qty,
    encode_side(order.side),
    order.cl_ord_id);
}

std::string csv_encoder::encode(const order_entry::replace_order& replace) const
{
  return fmt::format(
    "R,{},{},{},{},{},{},{}",
    replace.client_id,
    replace.symbol,
    replace.price,
    replace.order_qty,
    encode_side(replace.side),
    replace.cl_ord_id,
    replace.orig_cl_ord_id);
}

std::string csv_encoder::encode(const order_entry::cancel_order& cancel) const
{
  return fmt::format("C,{},{}", cancel.client_id, cancel.orig_cl_ord_id);
}

std::string csv_encoder::encode(const order_entry::flush&) const
{
  return "F";
}

std::string csv_encoder::encode(const order_entry::request& request) const
{
  return lab::match(request, [this](const auto& command) { return encode(command); });
}

char csv_encoder::encode_side(order_entry::types::side side)
{
  switch (side) {
    case order_entry::types::side::buy:
      return 'B';
    case order_entry::types::side::sell:
      return 'S';
  }

  std::terminate();
}

} // namespace order_client
