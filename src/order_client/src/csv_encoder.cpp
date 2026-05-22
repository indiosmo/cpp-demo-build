#include "order_client/csv_encoder.hpp"

#include "lab/fmt.hpp"
#include "lab/variant.hpp"

#include <cstdlib>

namespace order_client {

std::string csv_encoder::encode(const order_routing::new_order& order) const
{
  return fmt::format(
    "N,{},{},{},{},{},{}",
    order.user,
    order.instrument,
    order.limit_price,
    order.order_quantity,
    encode_side(order.order_side),
    order.order_id);
}

std::string csv_encoder::encode(const order_routing::cancel_order& cancel) const
{
  return fmt::format("C,{},{}", cancel.user, cancel.order_id);
}

std::string csv_encoder::encode(const order_routing::flush&) const
{
  return "F";
}

std::string csv_encoder::encode(const order_routing::request& request) const
{
  return lab::match(request, [this](const auto& command) { return encode(command); });
}

char csv_encoder::encode_side(order_routing::types::side side)
{
  switch (side) {
    case order_routing::types::side::buy:
      return 'B';
    case order_routing::types::side::sell:
      return 'S';
  }

  std::terminate();
}

} // namespace order_client
