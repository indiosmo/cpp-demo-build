#ifndef ORDER_ROUTING_ERROR_CODE_HPP
#define ORDER_ROUTING_ERROR_CODE_HPP

#include "kraken/error_macros.hpp"

#include <cstdint>

/*
 * Request failure taxonomy for the order_routing domain. Numeric range
 * 201xxx identifies codes from this domain without the category name
 * (kraken:: utilities use 101xxx).
 */

namespace order_routing {

enum class error_code : std::int32_t
{
  invalid_field = 201001,
  missing_field = 201002,
  unknown_order = 201003,
  parser_error = 201004,
};

constexpr const char* to_string(error_code ec)
{
  switch (ec) {
    case error_code::invalid_field:
      return "invalid field";

    case error_code::missing_field:
      return "missing field";

    case error_code::unknown_order:
      return "unknown order";

    case error_code::parser_error:
      return "parser error";
  }

  return "unknown error";
}

} // namespace order_routing

KRAKEN_DEFINE_ERROR_CATEGORY(order_routing)

#endif /* ORDER_ROUTING_ERROR_CODE_HPP */
