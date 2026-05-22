#ifndef QUICKFIX_FIX_CODEC_HPP
#define QUICKFIX_FIX_CODEC_HPP

#include "lab/result.hpp"
#include "quickfix_fix/message.hpp"

#include <string>
#include <string_view>

namespace quickfix_fix {

inline constexpr char soh = '\x01';
inline constexpr char printable_delimiter = '|';

[[nodiscard]] std::string encode(const message& message, char delimiter = soh);
[[nodiscard]] lab::result<message> decode(std::string_view payload, char delimiter = soh);

} // namespace quickfix_fix

#endif /* QUICKFIX_FIX_CODEC_HPP */
