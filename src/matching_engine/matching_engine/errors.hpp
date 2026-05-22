#ifndef MATCHING_ENGINE_ERRORS_HPP
#define MATCHING_ENGINE_ERRORS_HPP

#include "matching_engine/error_code.hpp"
#include "order_routing/types.hpp"

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
  order_routing::types::user_id user;
  order_routing::types::user_order_id order_id;

  std::error_code error_code() const
  {
    return matching_engine::error_code::duplicate_order;
  }

  std::string what() const
  {
    return fmt::format("user={} order_id={}", user, order_id);
  }
};

struct unknown_symbol
{
  order_routing::types::symbol instrument;

  std::error_code error_code() const
  {
    return matching_engine::error_code::unknown_symbol;
  }

  std::string what() const
  {
    return fmt::format("instrument={}", instrument);
  }
};

} // namespace matching_engine::errors

#endif /* MATCHING_ENGINE_ERRORS_HPP */
