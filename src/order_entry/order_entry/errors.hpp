#ifndef ORDER_ENTRY_ERRORS_HPP
#define ORDER_ENTRY_ERRORS_HPP

#include "order_entry/error_code.hpp"
#include "order_entry/types.hpp"

#include "lab/fmt.hpp"

#include <string>
#include <system_error>

/*
 * Structured error payloads for order_entry request failures. Each
 * struct satisfies lab::ErrorData so it travels through boost::leaf;
 * session.cpp dispatches on the concrete type via lab::match_error<T>.
 */

namespace order_entry::errors {

struct invalid_field
{
  std::string field_name;
  std::string field_value;

  std::error_code error_code() const
  {
    return order_entry::error_code::invalid_field;
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
    return order_entry::error_code::missing_field;
  }

  std::string what() const
  {
    return fmt::format("missing required field: {}", field_name);
  }
};

struct unknown_order
{
  order_entry::types::client_id client_id;
  order_entry::types::orig_cl_ord_id orig_cl_ord_id;

  std::error_code error_code() const
  {
    return order_entry::error_code::unknown_order;
  }

  std::string what() const
  {
    return fmt::format("client_id={} orig_cl_ord_id={}", client_id, orig_cl_ord_id);
  }
};

struct parser_error
{
  std::string reason;

  std::error_code error_code() const
  {
    return order_entry::error_code::parser_error;
  }

  std::string what() const
  {
    return fmt::format("parser error: {}", reason);
  }
};

} // namespace order_entry::errors

#endif /* ORDER_ENTRY_ERRORS_HPP */
