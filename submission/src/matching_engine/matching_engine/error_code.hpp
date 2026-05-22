#ifndef MATCHING_ENGINE_ERROR_CODE_HPP
#define MATCHING_ENGINE_ERROR_CODE_HPP

#include "kraken/error_macros.hpp"

#include <cstdint>

/*
 * Engine failure taxonomy for the matching_engine domain. Numeric range
 * 202xxx identifies codes from this domain without the category name
 * (kraken:: utilities use 101xxx, order_routing uses 201xxx).
 */

namespace matching_engine {

enum class error_code : std::int32_t
{
  duplicate_order = 202001,
  unknown_symbol = 202002,
};

constexpr const char* to_string(error_code ec)
{
  switch (ec) {
    case error_code::duplicate_order:
      return "duplicate order";

    case error_code::unknown_symbol:
      return "unknown symbol";
  }

  return "unknown error";
}

} // namespace matching_engine

KRAKEN_DEFINE_ERROR_CATEGORY(matching_engine)

#endif /* MATCHING_ENGINE_ERROR_CODE_HPP */
