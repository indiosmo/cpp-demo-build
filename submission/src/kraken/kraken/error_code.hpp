#ifndef KRAKEN_ERROR_CODE_HPP
#define KRAKEN_ERROR_CODE_HPP

#include "kraken/error_macros.hpp"

#include <system_error>

namespace kraken {

enum class error_code
{
  generic_error = 101001,
  invalid_argument = 101004,
  out_of_bounds = 101007,
  already_in_progress = 101008,
  not_implemented = 101009,
  configuration_error = 101011,
};

constexpr const char* to_string(error_code ec)
{
  switch (ec) {
    case error_code::generic_error:
      return "generic error";

    case error_code::invalid_argument:
      return "invalid argument";

    case error_code::out_of_bounds:
      return "out of bounds";

    case error_code::already_in_progress:
      return "already in progress";

    case error_code::not_implemented:
      return "not implemented";

    case error_code::configuration_error:
      return "configuration error";
  }

  return "unknown error";
}

} // namespace kraken

KRAKEN_DEFINE_ERROR_CATEGORY(kraken)

#endif /* KRAKEN_ERROR_CODE_HPP */
