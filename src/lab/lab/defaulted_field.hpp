#ifndef LAB_DEFAULTED_FIELD_HPP
#define LAB_DEFAULTED_FIELD_HPP

#include "lab/fmt.hpp"

#include <compare>
#include <concepts>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace lab {

/*
 * Transparent wrapper that carries a compile-time default for config fields.
 * The holder type keeps defaults available for values that are awkward as
 * non-type template parameters, such as fixed_string and other class types.
 */
template <typename T, typename DefaultHolder>
struct defaulted_field
{
  using value_type = T;

  static constexpr T default_value = DefaultHolder::value;

  T value = default_value;

  constexpr defaulted_field() = default;

  constexpr defaulted_field(T initial_value)
    : value{std::move(initial_value)}
  {
  }

  template <typename U>
    requires std::is_constructible_v<T, std::initializer_list<U>>
  constexpr defaulted_field(std::initializer_list<U> initial_values)
    : value{initial_values}
  {
  }

  constexpr operator T&() noexcept
  {
    return value;
  }

  constexpr operator const T&() const noexcept
  {
    return value;
  }

  friend constexpr bool operator==(const defaulted_field& lhs, const defaulted_field& rhs)
    requires requires { lhs.value == rhs.value; }
  {
    return lhs.value == rhs.value;
  }

  friend constexpr auto operator<=>(const defaulted_field& lhs, const defaulted_field& rhs)
    requires requires { lhs.value <=> rhs.value; }
  {
    return lhs.value <=> rhs.value;
  }

  template <typename U>
  friend constexpr bool operator==(const defaulted_field& lhs, const U& rhs)
    requires(!std::same_as<std::decay_t<U>, defaulted_field>) && std::constructible_from<T, const U&>
  {
    return lhs.value == rhs;
  }

  template <typename U>
  friend constexpr auto operator<=>(const defaulted_field& lhs, const U& rhs)
    requires(!std::same_as<std::decay_t<U>, defaulted_field>) && std::constructible_from<T, const U&>
  {
    return lhs.value <=> rhs;
  }

  constexpr auto begin()
    requires requires (T& v) { v.begin(); }
  {
    return value.begin();
  }

  constexpr auto begin() const
    requires requires (const T& v) { v.begin(); }
  {
    return value.begin();
  }

  constexpr auto end()
    requires requires (T& v) { v.end(); }
  {
    return value.end();
  }

  constexpr auto end() const
    requires requires (const T& v) { v.end(); }
  {
    return value.end();
  }
};

template <typename T>
struct is_defaulted_field : std::false_type
{
};

template <typename T, typename DefaultHolder>
struct is_defaulted_field<defaulted_field<T, DefaultHolder>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_defaulted_field_v = is_defaulted_field<std::decay_t<T>>::value;

template <typename T>
concept DefaultedField = is_defaulted_field_v<T>;

} // namespace lab

template <typename T, typename DefaultHolder>
struct fmt::formatter<lab::defaulted_field<T, DefaultHolder>>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const lab::defaulted_field<T, DefaultHolder>& field, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "{0}", field.value);
  }
};

#define LAB_DEFAULTED_FIELD(type, name, ...)  \
  struct name##_default_holder                \
  {                                           \
    static constexpr type value{__VA_ARGS__}; \
  };                                          \
  ::lab::defaulted_field<type, name##_default_holder> name {}

#endif // LAB_DEFAULTED_FIELD_HPP
