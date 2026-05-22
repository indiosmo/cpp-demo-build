#ifndef QUICKFIX_FIX_ERRORS_HPP
#define QUICKFIX_FIX_ERRORS_HPP

#include "lab/fmt.hpp"
#include "quickfix_fix/error_code.hpp"

#include <string>
#include <system_error>

namespace quickfix_fix::errors {

struct not_connected
{
  std::error_code error_code() const
  {
    return quickfix_fix::error_code::not_connected;
  }

  std::string what() const
  {
    return "FIX session has no peer";
  }
};

struct malformed_message
{
  std::string text;

  std::error_code error_code() const
  {
    return quickfix_fix::error_code::malformed_message;
  }

  std::string what() const
  {
    return text;
  }
};

struct missing_msg_type
{
  std::error_code error_code() const
  {
    return quickfix_fix::error_code::missing_msg_type;
  }

  std::string what() const
  {
    return "tag 35 is required";
  }
};

} // namespace quickfix_fix::errors

#endif /* QUICKFIX_FIX_ERRORS_HPP */
