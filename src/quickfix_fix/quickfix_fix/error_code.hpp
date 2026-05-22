#ifndef QUICKFIX_FIX_ERROR_CODE_HPP
#define QUICKFIX_FIX_ERROR_CODE_HPP

#include "lab/error_macros.hpp"

#include <cstdint>

namespace quickfix_fix {

enum class error_code : std::int32_t
{
  not_connected = 207001,
  malformed_message = 207002,
  missing_msg_type = 207003,
};

constexpr const char* to_string(error_code ec)
{
  switch (ec) {
    case error_code::not_connected:
      return "not connected";
    case error_code::malformed_message:
      return "malformed message";
    case error_code::missing_msg_type:
      return "missing message type";
  }

  return "unknown error";
}

} // namespace quickfix_fix

LAB_DEFINE_ERROR_CATEGORY(quickfix_fix)

#endif /* QUICKFIX_FIX_ERROR_CODE_HPP */
