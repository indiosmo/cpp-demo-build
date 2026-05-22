#ifndef ORDER_CLIENT_JSON_ENCODER_HPP
#define ORDER_CLIENT_JSON_ENCODER_HPP

#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include <string>

namespace order_client {

class json_encoder
{
public:
  [[nodiscard]] std::string encode(const order_entry::new_order_single& order) const;
  [[nodiscard]] std::string encode(const order_entry::replace_order& replace) const;
  [[nodiscard]] std::string encode(const order_entry::cancel_order& cancel) const;
  [[nodiscard]] std::string encode(const order_entry::flush& flush_request) const;
  [[nodiscard]] std::string encode(const order_entry::request& request) const;

private:
  [[nodiscard]] static const char* encode_side(order_entry::types::side side);
  [[nodiscard]] static const char* encode_ord_type(order_entry::types::ord_type ord_type);
  [[nodiscard]] static const char* encode_time_in_force(order_entry::types::time_in_force time_in_force);
};

} // namespace order_client

#endif /* ORDER_CLIENT_JSON_ENCODER_HPP */
