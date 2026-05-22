#ifndef MORFIX_QUICKFIX_ERROR_CODE_HPP
#define MORFIX_QUICKFIX_ERROR_CODE_HPP

#include "lab/error_macros.hpp"

#include <cstdint>

namespace morfix_quickfix {

enum class error_code : std::int32_t
{
  unsupported_message = 206002,
};

constexpr const char* to_string(error_code ec)
{
  switch (ec) {
    case error_code::unsupported_message:
      return "unsupported message";
  }

  return "unknown error";
}

} // namespace morfix_quickfix

LAB_DEFINE_ERROR_CATEGORY(morfix_quickfix)

#endif /* MORFIX_QUICKFIX_ERROR_CODE_HPP */
