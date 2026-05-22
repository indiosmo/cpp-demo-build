#ifndef LAB_ERRORS_HPP
#define LAB_ERRORS_HPP

#include "lab/error_code.hpp"
#include "lab/fmt.hpp"

#include <optional>
#include <string>
#include <system_error>

namespace lab::errors {

/*
 * Catch-all error payload pairing an error_code with optional free-form text.
 * Domain code should prefer a named structured error when callers might want
 * to match on it.
 */
struct generic_error
{
  std::error_code ec = lab::error_code::generic_error;
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

} // namespace lab::errors

#endif /* LAB_ERRORS_HPP */
