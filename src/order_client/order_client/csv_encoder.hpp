#ifndef ORDER_CLIENT_CSV_ENCODER_HPP
#define ORDER_CLIENT_CSV_ENCODER_HPP

#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include <string>

namespace order_client {

class csv_encoder
{
public:
  [[nodiscard]] std::string encode(const order_routing::new_order& order) const;
  [[nodiscard]] std::string encode(const order_routing::cancel_order& cancel) const;
  [[nodiscard]] std::string encode(const order_routing::flush& flush_request) const;
  [[nodiscard]] std::string encode(const order_routing::request& request) const;

private:
  [[nodiscard]] static char encode_side(order_routing::types::side side);
};

} // namespace order_client

#endif /* ORDER_CLIENT_CSV_ENCODER_HPP */
