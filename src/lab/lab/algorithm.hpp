#ifndef LAB_ALGORITHM_HPP
#define LAB_ALGORITHM_HPP

#include "lab/assert.hpp"

#include <array>
#include <boost/algorithm/string/split.hpp>
#include <cstddef>
#include <string_view>
#include <vector>

/*
 * string_view algorithms shared across the lab vocabulary. Trimming and
 * fixed-arity field splitting are repeated often enough that an unambiguous
 * spelling earns its place here.
 */

namespace lab {

inline constexpr std::string_view ascii_whitespace = " \t\r\n";

constexpr std::string_view ltrim(std::string_view view, std::string_view chars = ascii_whitespace) noexcept
{
  const auto first = view.find_first_not_of(chars);
  if (first == std::string_view::npos) {
    return {};
  }
  return view.substr(first);
}

constexpr std::string_view rtrim(std::string_view view, std::string_view chars = ascii_whitespace) noexcept
{
  const auto last = view.find_last_not_of(chars);
  if (last == std::string_view::npos) {
    return {};
  }
  return view.substr(0, last + 1);
}

constexpr std::string_view trim(std::string_view view, std::string_view chars = ascii_whitespace) noexcept
{
  return rtrim(ltrim(view, chars), chars);
}

/*
 * Splits a fixed-shape delimited body into exactly N trimmed fields. Arity is
 * part of the type so call sites encode it at compile time; a mismatch fires
 * LAB_ASSERT. Returned views slice into body (which must outlive them)
 * with ascii whitespace stripped on the outside; inner whitespace is kept.
 *
 * IMPROVEMENT: switch to a stack-resident alternative if profiling shows
 * the boost::algorithm::split vector alloc on a hot path.
 */
template <std::size_t N>
std::array<std::string_view, N> split_fields(std::string_view body, char delimiter = ',')
{
  std::vector<std::string_view> parts;
  boost::split(parts, body, [delimiter](char c) { return c == delimiter; });
  LAB_ASSERT(parts.size() == N);

  std::array<std::string_view, N> fields{};
  for (std::size_t i = 0; i < N; ++i) {
    fields[i] = lab::trim(parts[i]);
  }
  return fields;
}

} // namespace lab

#endif /* LAB_ALGORITHM_HPP */
