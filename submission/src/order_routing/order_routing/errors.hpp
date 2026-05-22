#ifndef ORDER_ROUTING_ERRORS_HPP
#define ORDER_ROUTING_ERRORS_HPP

#include "order_routing/error_code.hpp"
#include "order_routing/types.hpp"

#include "kraken/fmt.hpp"

#include <string>
#include <system_error>

/*
 * Structured error payloads for order_routing request failures. Each
 * struct satisfies kraken::ErrorData so it travels through boost::leaf;
 * session.cpp dispatches on the concrete type via kraken::match_error<T>.
 */

namespace order_routing::errors {

struct invalid_field
{
  std::string field_name;
  std::string field_value;

  std::error_code error_code() const
  {
    return order_routing::error_code::invalid_field;
  }

  std::string what() const
  {
    return fmt::format("invalid field {}=\"{}\"", field_name, field_value);
  }
};

struct missing_field
{
  std::string field_name;

  std::error_code error_code() const
  {
    return order_routing::error_code::missing_field;
  }

  std::string what() const
  {
    return fmt::format("missing required field: {}", field_name);
  }
};

struct unknown_order
{
  order_routing::types::user_id user;
  order_routing::types::user_order_id order_id;

  std::error_code error_code() const
  {
    return order_routing::error_code::unknown_order;
  }

  std::string what() const
  {
    return fmt::format("user={} user_order_id={}", user, order_id);
  }
};

struct parser_error
{
  std::string reason;

  std::error_code error_code() const
  {
    return order_routing::error_code::parser_error;
  }

  std::string what() const
  {
    return fmt::format("parser error: {}", reason);
  }
};

} // namespace order_routing::errors

#endif /* ORDER_ROUTING_ERRORS_HPP */
