#ifndef KRAKEN_CHARCONV_HPP
#define KRAKEN_CHARCONV_HPP

#include "kraken/error.hpp"
#include "kraken/error_code.hpp"
#include "kraken/fixed_string.hpp"
#include "kraken/fmt.hpp"
#include "kraken/result.hpp"
#include "kraken/strong_type.hpp"

#include <charconv>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace kraken {

namespace charconv {

enum class match_type
{
  exact,
  partial,
};

} // namespace charconv

template <std::integral T, charconv::match_type MatchType = charconv::match_type::exact>
kraken::result<T> from_chars(std::string_view view)
{
  T value{};
  const auto* first = view.data();
  const auto* last = view.data() + view.size();

  const auto [ptr, ec] = std::from_chars(first, last, value);

  if (ec != std::errc{}) {
    return kraken::make_leaf_error(
      kraken::error_code::invalid_argument, fmt::format("can't convert string \"{}\" to integer", view));
  }

  if constexpr (MatchType == charconv::match_type::exact) {
    if (ptr != last) {
      return kraken::make_leaf_error(
        kraken::error_code::invalid_argument,
        fmt::format(
          "string \"{}\" to integer with exact match has unmatched characters \"{}\"", view, std::string_view{ptr, last}));
    }
  }

  return value;
}

template <typename T, charconv::match_type MatchType = charconv::match_type::exact>
  requires kraken::is_strong_type_v<T> && std::integral<typename T::UnderlyingType>
kraken::result<T> from_chars(std::string_view view)
{
  BOOST_LEAF_ASSIGN(auto value, (kraken::from_chars<typename T::UnderlyingType, MatchType>(view)));
  return T{value};
}

template <typename T, charconv::match_type MatchType = charconv::match_type::exact>
kraken::result<T> from_chars(const kraken::FixedString auto& fs)
{
  return kraken::from_chars<T, MatchType>(fs.to_string_view());
}

template <typename T, charconv::match_type MatchType = charconv::match_type::exact, typename U>
  requires kraken::is_strong_type_v<std::remove_cvref_t<U>> && requires (const U& value) { std::string_view{value.get()}; }
kraken::result<T> from_chars(const U& value)
{
  return kraken::from_chars<T, MatchType>(std::string_view{value.get()});
}

} // namespace kraken

#endif /* KRAKEN_CHARCONV_HPP */
