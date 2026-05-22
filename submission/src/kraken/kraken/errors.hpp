#ifndef KRAKEN_ERRORS_HPP
#define KRAKEN_ERRORS_HPP

#include "kraken/error_code.hpp"
#include "kraken/fmt.hpp"

#include <optional>
#include <string>
#include <system_error>

namespace kraken::errors {

/*
 * Catch-all error payload pairing an error_code with optional free-form text.
 * Domain code should prefer a named structured error when callers might want
 * to match on it.
 */
struct generic_error
{
  std::error_code ec = kraken::error_code::generic_error;
  std::optional<std::string> text{};

  std::error_code error_code() const
  {
    return ec;
  }

  std::string what() const
  {
    if (text.has_value()) {
      return fmt::format("{} | {}", ec.message(), text.value());
    } else {
      return fmt::format("{}", ec.message());
    }
  }
};

} // namespace kraken::errors

#endif /* KRAKEN_ERRORS_HPP */
