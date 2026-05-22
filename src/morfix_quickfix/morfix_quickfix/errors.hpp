#ifndef MORFIX_QUICKFIX_ERRORS_HPP
#define MORFIX_QUICKFIX_ERRORS_HPP

#include "lab/fmt.hpp"
#include "morfix_quickfix/error_code.hpp"

#include <string>
#include <system_error>

namespace morfix_quickfix::errors {

struct not_implemented
{
  std::string operation;

  std::error_code error_code() const
  {
    return morfix_quickfix::error_code::not_implemented;
  }

  std::string what() const
  {
    return fmt::format("{} is not implemented", operation);
  }
};

struct unsupported_message
{
  std::string message_type;

  std::error_code error_code() const
  {
    return morfix_quickfix::error_code::unsupported_message;
  }

  std::string what() const
  {
    return fmt::format("unsupported FIX message type {}", message_type);
  }
};

} // namespace morfix_quickfix::errors

#endif /* MORFIX_QUICKFIX_ERRORS_HPP */
