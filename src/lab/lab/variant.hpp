#ifndef LAB_VARIANT_HPP
#define LAB_VARIANT_HPP

#include "lab/overload.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace lab {

/* Type-trait helpers used by match. */
template <typename T, template <typename> class NestedType>
struct nested_type
{
  using type = typename NestedType<T>::type;
};

template <typename T, template <typename> class NestedType>
using nested_type_t = typename nested_type<T, NestedType>::type;

template <typename Callable, typename Tuple, std::size_t... I>
constexpr bool is_invocable_with_any_of(std::index_sequence<I...>)
{
  return (
    ... || (std::is_same_v<std::tuple_element_t<I, Tuple>, std::monostate> ||
            std::is_invocable_v<Callable, std::tuple_element_t<I, Tuple>>));
}

template <typename Callable, typename Tuple>
constexpr bool is_invocable_with_any_of()
{
  return is_invocable_with_any_of<Callable, Tuple>(std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

template <typename T, typename Tuple, std::size_t... Is>
constexpr std::size_t find_index_in_tuple(std::index_sequence<Is...>)
{
  return ((std::is_same<T, std::tuple_element_t<Is, Tuple>>::value ? Is : 0) + ...);
}

template <typename T, typename... Ts>
constexpr std::size_t get_variant_index()
{
  return find_index_in_tuple<T, std::tuple<Ts...>>(std::index_sequence_for<Ts...>{});
}

template <typename Var, typename T>
struct is_variant_member;

template <typename T, typename... Types>
struct is_variant_member<std::variant<Types...>, T> : std::disjunction<std::is_same<T, Types>...>
{
};

template <typename T, typename... Types>
constexpr bool is_variant_member_v = is_variant_member<T, Types...>::value;

static_assert(is_variant_member_v<std::variant<double, int>, int>);
static_assert(!is_variant_member_v<std::variant<double, int>, std::string>);

template <typename Var>
struct variant_types;

template <typename... Types>
struct variant_types<std::variant<Types...>>
{
  using types = std::tuple<Types...>;
};

template <typename Tuple, typename... NewTypes>
struct join_variant;

template <typename... OldTypes, typename... NewTypes>
struct join_variant<std::tuple<OldTypes...>, NewTypes...>
{
  using type = std::variant<OldTypes..., NewTypes...>;
};

template <typename Variant, typename... NewTypes>
using extend_variant = typename join_variant<typename variant_types<Variant>::types, NewTypes...>::type;

template <typename Var1, typename Var2>
struct concat_variants;

template <typename... T1s, typename... T2s>
struct concat_variants<std::variant<T1s...>, std::variant<T2s...>>
{
  using type = std::variant<T1s..., T2s...>;
};

template <typename Var1, typename Var2>
using concat_variants_t = typename concat_variants<Var1, Var2>::type;

/*
 * Recursive index-dispatch over a variant. Faster than std::visit for small
 * variants because the compiler inlines each branch directly; benchmarked at
 * https://stackoverflow.com/q/57726401.
 */
template <std::size_t N, typename R, typename Variant, typename Visitor>
[[nodiscard]] constexpr R match(Variant&& var, Visitor&& vis)
{
  if constexpr (N == 0) {
    if (N == var.index()) {
      // Guard the std::get so the compiler can drop the empty-variant
      // throwing path entirely.
      return std::forward<Visitor>(vis)(std::get<N>(std::forward<Variant>(var)));
    }
  } else {
    if (var.index() == N) {
      return std::forward<Visitor>(vis)(std::get<N>(std::forward<Variant>(var)));
    }
    return match<N - 1, R>(std::forward<Variant>(var), std::forward<Visitor>(vis));
  }
  // One of the branches above always returns; the infinite loop stands in for
  // std::unreachable() so the compiler does not warn about a missing return.
  while (true) {}
}

template <typename Variant, typename Visitor>
constexpr void check_variant_match_visitor()
{
  using variant_tuple = typename variant_types<Variant>::types;
  static_assert(is_invocable_with_any_of<Visitor, variant_tuple>(), "type being matched is not present in the variant");
}

/* Rejects visitors whose argument type is not in the variant at compile time. */
template <typename Variant, typename... Visitors>
constexpr void check_variant_match_visitors()
{
  (check_variant_match_visitor<Variant, Visitors>(), ...);
}

template <class... Args, typename Visitor, typename... Visitors>
[[nodiscard]] constexpr decltype(auto) match(const std::variant<Args...>& var, Visitor&& vis, Visitors&&... visitors)
{
  check_variant_match_visitors<std::variant<Args...>, Visitors...>();

  auto ol = overload(std::forward<Visitor>(vis), std::forward<Visitors>(visitors)...);
  using result_t = decltype(std::invoke(std::move(ol), std::get<0>(var)));

  static_assert(sizeof...(Args) > 0);
  return match<sizeof...(Args) - 1, result_t>(var, std::move(ol));
}

template <class... Args, typename Visitor, typename... Visitors>
[[nodiscard]] constexpr decltype(auto) match(std::variant<Args...>& var, Visitor&& vis, Visitors&&... visitors)
{
  check_variant_match_visitors<std::variant<Args...>, Visitors...>();

  auto ol = overload{std::forward<Visitor>(vis), std::forward<Visitors>(visitors)...};
  using result_t = decltype(std::invoke(std::move(ol), std::get<0>(var)));

  static_assert(sizeof...(Args) > 0);
  return match<sizeof...(Args) - 1, result_t>(var, std::move(ol));
}

template <class... Args, typename Visitor, typename... Visitors>
[[nodiscard]] constexpr decltype(auto) match(std::variant<Args...>&& var, Visitor&& vis, Visitors&&... visitors)
{
  check_variant_match_visitors<std::variant<Args...>, Visitors...>();

  auto ol = overload{std::forward<Visitor>(vis), std::forward<Visitors>(visitors)...};
  using result_t = decltype(std::invoke(std::move(ol), std::move(std::get<0>(var))));

  static_assert(sizeof...(Args) > 0);
  return match<sizeof...(Args) - 1, result_t>(std::move(var), std::move(ol));
}

template <typename Value, typename... Visitors>
inline constexpr bool is_visitable_v = (std::is_invocable_v<Visitors, Value> or ...);

/* Matches only the supplied visitors; noop for everything else. */
template <typename Variant, typename... Visitors>
constexpr auto match_partial(Variant&& var, Visitors&&... vis)
{
  // clang-format off
  return match(
    std::forward<Variant>(var),
    std::forward<Visitors>(vis)...,
    [](const auto&) {}
  );
  // clang-format on
}

namespace detail {

template <template <typename> class NestedType, typename Variant, std::size_t... I>
auto create_variant_from_nested_types(std::index_sequence<I...>)
{
  return std::variant<lab::nested_type_t<std::variant_alternative_t<I, Variant>, NestedType>...>{};
}

} // namespace detail

template <template <typename> class NestedType, typename Variant>
using nested_type_variant = decltype(detail::create_variant_from_nested_types<NestedType, Variant>(
  std::make_index_sequence<std::variant_size_v<Variant>>{}));

} // namespace lab

#define LAB_PLUCK(variant, field) lab::match(variant, [](const auto& x) { return x.field; })

#endif /* LAB_VARIANT_HPP */
