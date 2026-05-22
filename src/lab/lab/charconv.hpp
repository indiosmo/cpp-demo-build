#ifndef LAB_CHARCONV_HPP
#define LAB_CHARCONV_HPP

#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/fixed_string.hpp"
#include "lab/fmt.hpp"
#include "lab/result.hpp"
#include "lab/strong_type.hpp"

#include <charconv>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace lab {

namespace charconv {

enum class match_type
{
  exact,
  partial,
};

} // namespace charconv

template <std::integral T, charconv::match_type MatchType = charconv::match_type::exact>
lab::result<T> from_chars(std::string_view view)
{
  T value{};
  const auto* first = view.data();
  const auto* last = view.data() + view.size();

  const auto [ptr, ec] = std::from_chars(first, last, value);

  if (ec != std::errc{}) {
    return lab::make_leaf_error(
      lab::error_code::invalid_argument, fmt::format("can't convert string \"{}\" to integer", view));
  }

  if constexpr (MatchType == charconv::match_type::exact) {
    if (ptr != last) {
      return lab::make_leaf_error(
        lab::error_code::invalid_argument,
        fmt::format(
          "string \"{}\" to integer with exact match has unmatched characters \"{}\"", view, std::string_view{ptr, last}));
    }
  }

  return value;
}

template <typename T, charconv::match_type MatchType = charconv::match_type::exact>
  requires lab::is_strong_type_v<T> && std::integral<typename T::UnderlyingType>
lab::result<T> from_chars(std::string_view view)
{
  BOOST_LEAF_ASSIGN(auto value, (lab::from_chars<typename T::UnderlyingType, MatchType>(view)));
  return T{value};
}

template <typename T, charconv::match_type MatchType = charconv::match_type::exact>
lab::result<T> from_chars(const lab::FixedString auto& fs)
{
  return lab::from_chars<T, MatchType>(fs.to_string_view());
}

template <typename T, charconv::match_type MatchType = charconv::match_type::exact, typename U>
  requires lab::is_strong_type_v<std::remove_cvref_t<U>> && requires (const U& value) { std::string_view{value.get()}; }
lab::result<T> from_chars(const U& value)
{
  return lab::from_chars<T, MatchType>(std::string_view{value.get()});
}

} // namespace lab

#endif /* LAB_CHARCONV_HPP */
