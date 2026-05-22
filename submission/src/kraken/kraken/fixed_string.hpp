#ifndef KRAKEN_FIXED_STRING_HPP
#define KRAKEN_FIXED_STRING_HPP

#include "boost/container_hash/hash.hpp"

#include "kraken/assert.hpp"
#include "kraken/error.hpp"
#include "kraken/errors.hpp"
#include "kraken/fmt.hpp"
#include "kraken/result.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace kraken {

enum class fixed_string_truncation_policy : std::uint8_t
{
  strict,
  auto_truncate,
};

/*
 * Bounds-checked fixed-size string with no allocations. Stores the length in
 * buffer_[0] and a trailing null, so c_str() and a length-prefixed view are
 * O(1). Truncation behaviour is selected by the Policy template parameter.
 */
template <std::uint8_t N, fixed_string_truncation_policy Policy = fixed_string_truncation_policy::strict>
class fixed_string
{
public:
  static constexpr uint8_t max_size = N;

  // Length is stored in buffer_[0] as a signed char.
  static_assert(N <= std::numeric_limits<signed char>::max());

  fixed_string() = default;
  fixed_string(const fixed_string& other) = default;
  fixed_string(fixed_string&& other) = default;
  fixed_string& operator=(const fixed_string& other) = default;
  fixed_string& operator=(fixed_string&& other) = default;

  template <uint8_t M, fixed_string_truncation_policy OtherPolicy>
    requires(M <= N)
  constexpr fixed_string(const fixed_string<M, OtherPolicy>& other)
    : truncated_(other.truncated_)
  {
    std::copy(other.buffer_.begin(), other.buffer_.end(), buffer_.begin());
  }

  template <uint8_t M, fixed_string_truncation_policy OtherPolicy>
    requires(M <= N)
  constexpr fixed_string(fixed_string<M, OtherPolicy>&& other)
    : truncated_(other.truncated_)
  {
    std::move(other.buffer_.begin(), other.buffer_.end(), buffer_.begin());
  }

  template <uint8_t M, fixed_string_truncation_policy OtherPolicy>
    requires(M <= N)
  constexpr fixed_string& operator=(const fixed_string<M, OtherPolicy>& other)
  {
    std::copy(other.buffer_.begin(), other.buffer_.end(), buffer_.begin());
    std::fill(buffer_.begin() + other.buffer_.size(), buffer_.end(), 0);
    truncated_ = other.truncated_;
    return *this;
  }

  template <uint8_t M, fixed_string_truncation_policy OtherPolicy>
    requires(M <= N)
  constexpr fixed_string& operator=(fixed_string<M, OtherPolicy>&& other)
  {
    std::move(other.buffer_.begin(), other.buffer_.end(), buffer_.begin());
    std::fill(buffer_.begin() + other.buffer_.size(), buffer_.end(), 0);
    truncated_ = other.truncated_;
    return *this;
  }

  /* Direct construction from a string literal that fits; assumes a trailing null. */
  template <std::size_t M>
    requires(M - 1 <= N)
  constexpr fixed_string(const char (&str)[M])
  {
    std::copy(str, str + (M - 1), buffer_.begin() + 1);
    buffer_[0] = static_cast<char>(M - 1);
  }

  constexpr fixed_string(std::string_view sv)
    requires(Policy == fixed_string_truncation_policy::auto_truncate)
    : fixed_string(from_string_view(sv))
  {
  }

  constexpr fixed_string(const char* data, std::size_t size)
    requires(Policy == fixed_string_truncation_policy::auto_truncate)
    : fixed_string(std::string_view{data, size})
  {
    // precondition: data is non-null.
    KRAKEN_ASSERT(data != nullptr);
  }

  [[nodiscard]] static fixed_string truncate_from(std::string_view sv)
    requires(Policy == fixed_string_truncation_policy::strict)
  {
    return from_string_view(sv);
  }

  static kraken::result<fixed_string> from(std::string_view sv)
    requires(Policy == fixed_string_truncation_policy::strict)
  {
    if (sv.size() > N) {
      return kraken::make_leaf_error(kraken::error_code::out_of_bounds, "max size exceeded");
    }

    return from_string_view(sv);
  }

  [[nodiscard]] static kraken::result<fixed_string> from(const char* data, std::size_t size)
    requires(Policy == fixed_string_truncation_policy::strict)
  {
    KRAKEN_ASSERT(data != nullptr);
    return from(std::string_view{data, size});
  }

  [[nodiscard]] static kraken::result<fixed_string> from(std::integral auto number)
  {
    fixed_string fs;

    auto result = fmt::format_to_n(fs.buffer_.begin() + 1, N, "{}", number);

    if (result.size > N) {
      return kraken::make_leaf_error(
        kraken::error_code::out_of_bounds, fmt::format("number {} exceeds max string size {}", number, N));
    }

    fs.buffer_[0] = static_cast<char>(result.size);

    return fs;
  }

  [[nodiscard]] fixed_string remove_suffix(std::string_view chars) const
  {
    auto sv = this->to_string_view();

    if (sv.ends_with(chars)) {
      sv.remove_suffix(chars.size());
    }

    fixed_string fs;
    fs.buffer_[0] = static_cast<char>(sv.size());
    std::copy(sv.begin(), sv.end(), fs.buffer_.begin() + 1);
    fs.truncated_ = truncated_;
    return fs;
  }

  [[nodiscard]] constexpr const char* c_str() const
  {
    return &buffer_[1];
  }

  [[nodiscard]] constexpr const char* length_prefixed_string() const
  {
    return &buffer_[0];
  }

  [[nodiscard]] std::string to_string() const
  {
    return std::string(c_str(), size());
  }

  [[nodiscard]] constexpr std::string_view to_string_view() const
  {
    return std::string_view(c_str(), size());
  }

  constexpr operator std::string_view() const
  {
    return this->to_string_view();
  }

  template <typename T>
  [[nodiscard]] constexpr bool contains(T&& t) const
  {
    return this->to_string_view().find(std::forward<T>(t)) != std::string_view::npos;
  }

  template <typename T>
  [[nodiscard]] constexpr bool starts_with(T&& t) const
  {
    return this->to_string_view().starts_with(std::forward<T>(t));
  }

  template <typename T>
  [[nodiscard]] constexpr bool ends_with(T&& t) const
  {
    return this->to_string_view().ends_with(std::forward<T>(t));
  }

  [[nodiscard]] constexpr uint8_t size() const
  {
// False-positive -Wnull-dereference: buffer_ is an inline std::array.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    return static_cast<uint8_t>(buffer_[0]);
#pragma GCC diagnostic pop
  }

  [[nodiscard]] constexpr bool empty() const
  {
    return size() == 0;
  }

  [[nodiscard]] constexpr bool truncated() const
  {
    return truncated_;
  }

  [[nodiscard]] constexpr bool operator==(std::string_view rhs) const
  {
    return this->to_string_view() == rhs;
  }

  /* Explicit; gcc reports ambiguity against the auto-generated form. */
  [[nodiscard]] constexpr bool operator!=(std::string_view rhs) const
  {
    return this->to_string_view() != rhs;
  }

  [[nodiscard]] constexpr bool operator==(const char* rhs) const
  {
    KRAKEN_ASSERT(rhs != nullptr);
    return this->to_string_view() == std::string_view(rhs);
  }

  [[nodiscard]] constexpr bool operator!=(const char* rhs) const
  {
    return this->to_string_view() != std::string_view(rhs);
  }

private:
  template <uint8_t M, fixed_string_truncation_policy OtherPolicy>
  friend class fixed_string;

  template <uint8_t M, fixed_string_truncation_policy OtherPolicy>
  friend std::size_t hash_value(const fixed_string<M, OtherPolicy>& fs) noexcept;

  friend struct std::hash<fixed_string>;

  [[nodiscard]] static constexpr fixed_string from_string_view(std::string_view sv)
  {
    auto size = std::min(sv.size(), static_cast<std::size_t>(max_size));
    fixed_string fs;
    fs.buffer_[0] = static_cast<char>(size);
    fs.truncated_ = sv.size() > max_size;
    std::copy(sv.begin(), sv.begin() + size, fs.buffer_.begin() + 1);
    return fs;
  }

  /* buffer_[0] is the length (uint8_t); buffer_[N+1] is the null terminator. */
  std::array<char, N + 2> buffer_ = {};
  bool truncated_ = false;
};

template <std::uint8_t N>
using truncating_fixed_string = fixed_string<N, fixed_string_truncation_policy::auto_truncate>;

template <typename T>
struct is_fixed_string : std::false_type
{
};

template <std::uint8_t N, fixed_string_truncation_policy Policy>
struct is_fixed_string<fixed_string<N, Policy>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_fixed_string_v = is_fixed_string<std::remove_cvref_t<T>>::value;

template <typename T>
concept FixedString = is_fixed_string_v<T>;

static_assert(is_fixed_string_v<fixed_string<8>>);
static_assert(FixedString<fixed_string<8>>);
static_assert(!FixedString<std::array<int, 8>>);

template <uint8_t N, fixed_string_truncation_policy Policy>
std::ostream& operator<<(std::ostream& os, const fixed_string<N, Policy>& fs)
{
  return os << fs.to_string_view();
}

template <uint8_t N, fixed_string_truncation_policy Policy>
std::size_t hash_value(const fixed_string<N, Policy>& fs) noexcept
{
  return boost::hash_range(fs.buffer_.begin(), fs.buffer_.end());
}

} // namespace kraken

namespace std {
template <uint8_t N, kraken::fixed_string_truncation_policy Policy>
struct hash<kraken::fixed_string<N, Policy>>
{
  [[nodiscard]] std::size_t operator()(const kraken::fixed_string<N, Policy>& fs) const noexcept
  {
    return boost::hash_range(fs.buffer_.begin(), fs.buffer_.end());
  }
};
} // namespace std

template <uint8_t N, kraken::fixed_string_truncation_policy Policy>
struct fmt::formatter<kraken::fixed_string<N, Policy>>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const kraken::fixed_string<N, Policy>& str, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "{0}", str.to_string_view());
  }
};

#endif /* KRAKEN_FIXED_STRING_HPP */
