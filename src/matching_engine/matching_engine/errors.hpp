#ifndef MATCHING_ENGINE_ERRORS_HPP
#define MATCHING_ENGINE_ERRORS_HPP

#include "matching_engine/error_code.hpp"
#include "order_entry/types.hpp"

#include "lab/fmt.hpp"

#include <string>
#include <system_error>

/*
 * Structured error payloads for matching_engine request handling
 * failures. Each struct satisfies lab::ErrorData so it travels
 * through boost::leaf; the request handler dispatches on the concrete
 * type via lab::match_error<T>.
 */

namespace matching_engine::errors {

struct duplicate_order
{
  order_entry::types::client_id client_id;
  order_entry::types::cl_ord_id cl_ord_id;

  std::error_code error_code() const
  {
    return matching_engine::error_code::duplicate_order;
  }

  std::string what() const
  {
    return fmt::format("client_id={} cl_ord_id={}", client_id, cl_ord_id);
  }
};

struct unknown_symbol
{
  order_entry::types::symbol symbol;

  std::error_code error_code() const
  {
    return matching_engine::error_code::unknown_symbol;
  }

  std::string what() const
  {
    return fmt::format("symbol={}", symbol);
  }
};

} // namespace matching_engine::errors

#endif /* MATCHING_ENGINE_ERRORS_HPP */
