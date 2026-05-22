#ifndef KRAKEN_STRONG_TYPE_HPP
#define KRAKEN_STRONG_TYPE_HPP

#include "boost/container_hash/hash.hpp"
#include "NamedType/named_type.hpp"

#include "kraken/fixed_string.hpp"
#include "kraken/fmt.hpp"
#include "kraken/result.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>

/*
 * NamedType-backed nominal typedef with the project's default skill bundle:
 * call-through, comparison, hashing, streaming, and a fixed_string-aware
 * Factory.
 */

namespace kraken {

template <typename T, typename Parameter, template <typename> class... Skills>
class strong_type;

} // namespace kraken

namespace fluent {

/*
 * Extend NamedType's Callable/Dereferencable skills to this project's wrapper
 * type. Upstream provides these specializations only for fluent::NamedType;
 * kraken::strong_type keeps the skill spelling while owning construction,
 * comparison, and formatting policy locally.
 */
template <typename T, typename Parameter, template <typename> class... Skills>
struct FunctionCallable<kraken::strong_type<T, Parameter, Skills...>>
  : crtp<kraken::strong_type<T, Parameter, Skills...>, FunctionCallable>
{
  [[nodiscard]] constexpr operator const T&() const
  {
    return this->underlying().get();
  }

  [[nodiscard]] constexpr operator T&()
  {
    return this->underlying().get();
  }
};

template <typename T, typename Parameter, template <typename> class... Skills>
struct MethodCallable<kraken::strong_type<T, Parameter, Skills...>>
  : crtp<kraken::strong_type<T, Parameter, Skills...>, MethodCallable>
{
  [[nodiscard]] constexpr const std::remove_reference_t<T>* operator->() const
  {
    return std::addressof(this->underlying().get());
  }

  [[nodiscard]] constexpr std::remove_reference_t<T>* operator->()
  {
    return std::addressof(this->underlying().get());
  }
};

template <typename T, typename Parameter, template <typename> class... Skills>
struct Dereferencable<kraken::strong_type<T, Parameter, Skills...>>
  : crtp<kraken::strong_type<T, Parameter, Skills...>, Dereferencable>
{
  [[nodiscard]] constexpr T& operator*() &
  {
    return this->underlying().get();
  }

  [[nodiscard]] constexpr const std::remove_reference_t<T>& operator*() const&
  {
    return this->underlying().get();
  }
};

template <typename T>
struct Factory : crtp<T, Factory>
{
  /*
   * Extend the upstream Factory skill with the fixed_string constructors this
   * codebase uses at parser/config boundaries.
   */
  template <typename... Args>
    requires(kraken::is_fixed_string_v<typename T::UnderlyingType>)
  static kraken::result<T> from(Args&&... args)
  {
    BOOST_LEAF_ASSIGN(auto ret, T::UnderlyingType::from(std::forward<Args>(args)...));
    return T{ret};
  }

  template <typename... Args>
    requires(kraken::is_fixed_string_v<typename T::UnderlyingType>)
  [[nodiscard]] static T truncate_from(Args&&... args)
  {
    return T{T::UnderlyingType::truncate_from(std::forward<Args>(args)...)};
  }
};

} // namespace fluent

namespace kraken {

namespace detail {

template <typename T, typename... Args>
concept NonNarrowingConstructible = requires { T{std::declval<Args>()...}; };

template <typename T, typename... Args>
inline constexpr bool is_single_underlying_arg_v = false;

template <typename T, typename Arg>
inline constexpr bool is_single_underlying_arg_v<T, Arg> = std::is_same_v<std::decay_t<Arg>, T>;

template <typename T>
struct Comparable : fluent::crtp<T, Comparable>
{
  /*
   * Replace fluent::Comparable so equality-only underlying types can still be
   * compared for equality. Upstream derives equality from ordering; this
   * version uses `==` directly and only requires `<` for ordered operations.
   */
  [[nodiscard]] constexpr bool operator<(const Comparable& other) const
  {
    return this->underlying().get() < other.underlying().get();
  }

  [[nodiscard]] constexpr bool operator>(const Comparable& other) const
  {
    return other.underlying().get() < this->underlying().get();
  }

  [[nodiscard]] constexpr bool operator<=(const Comparable& other) const
  {
    return !(other < *this);
  }

  [[nodiscard]] constexpr bool operator>=(const Comparable& other) const
  {
    return !(*this < other);
  }

  [[nodiscard]] constexpr bool operator==(const Comparable& other) const
  {
    return this->underlying().get() == other.underlying().get();
  }

  [[nodiscard]] constexpr bool operator!=(const Comparable& other) const
  {
    return !(*this == other);
  }
};

/*
 * Extend the default strong_type surface with same-type arithmetic, but keep it
 * narrower than fluent::Arithmetic: no increments, bitwise operators, or shifts,
 * and each operator only participates when the underlying expression is valid.
 *
 * Same-type arithmetic preserves the strong type. Mixed expressions still come
 * from Callable's implicit underlying-reference conversion:
 *
 *   quantity - quantity      -> quantity
 *   quantity - price         -> UnderlyingType, when both underlyings combine
 *   quantity - UnderlyingType -> UnderlyingType
 *   UnderlyingType - quantity -> UnderlyingType
 *
 * Re-wrapping a mixed arithmetic result is therefore explicit at the call
 * site. `auto raw = trade_quantity - price;` drops to the underlying type;
 * `quantity suspicious{trade_quantity - price};` is still allowed, but the
 * construction makes the domain mismatch visible in review.
 */
template <typename T>
struct Arithmetic : fluent::crtp<T, Arithmetic>
{
  [[nodiscard]] constexpr T operator+() const
    requires requires (const typename T::UnderlyingType& value) { +value; }
  {
    return T{+this->underlying().get()};
  }

  [[nodiscard]] constexpr T operator-() const
    requires requires (const typename T::UnderlyingType& value) { -value; }
  {
    return T{-this->underlying().get()};
  }

  [[nodiscard]] constexpr T operator+(const T& other) const
    requires requires (const typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left + right; }
  {
    return T{this->underlying().get() + other.get()};
  }

  constexpr T& operator+=(const T& other)
    requires requires (typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left += right; }
  {
    this->underlying().get() += other.get();
    return this->underlying();
  }

  [[nodiscard]] constexpr T operator-(const T& other) const
    requires requires (const typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left - right; }
  {
    return T{this->underlying().get() - other.get()};
  }

  constexpr T& operator-=(const T& other)
    requires requires (typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left -= right; }
  {
    this->underlying().get() -= other.get();
    return this->underlying();
  }

  [[nodiscard]] constexpr T operator*(const T& other) const
    requires requires (const typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left * right; }
  {
    return T{this->underlying().get() * other.get()};
  }

  constexpr T& operator*=(const T& other)
    requires requires (typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left *= right; }
  {
    this->underlying().get() *= other.get();
    return this->underlying();
  }

  [[nodiscard]] constexpr T operator/(const T& other) const
    requires requires (const typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left / right; }
  {
    return T{this->underlying().get() / other.get()};
  }

  constexpr T& operator/=(const T& other)
    requires requires (typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left /= right; }
  {
    this->underlying().get() /= other.get();
    return this->underlying();
  }

  [[nodiscard]] constexpr T operator%(const T& other) const
    requires requires (const typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left % right; }
  {
    return T{this->underlying().get() % other.get()};
  }

  constexpr T& operator%=(const T& other)
    requires requires (typename T::UnderlyingType& left, const typename T::UnderlyingType& right) { left %= right; }
  {
    this->underlying().get() %= other.get();
    return this->underlying();
  }
};

} // namespace detail

template <typename T, typename Parameter, template <typename> class... Skills>
class FLUENT_EBCO strong_type
  : public fluent::Callable<strong_type<T, Parameter, Skills...>>
  , public detail::Comparable<strong_type<T, Parameter, Skills...>>
  , public detail::Arithmetic<strong_type<T, Parameter, Skills...>>
  , public fluent::Hashable<strong_type<T, Parameter, Skills...>>
  , public fluent::Printable<strong_type<T, Parameter, Skills...>>
  , public fluent::Factory<strong_type<T, Parameter, Skills...>>
  , public Skills<strong_type<T, Parameter, Skills...>>...
{
public:
  using UnderlyingType = T;
  using ref = strong_type<T&, Parameter, Skills...>;

  strong_type() = default;

  explicit constexpr strong_type(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    : value_{value}
  {
  }

  template <typename T_ = T>
    requires(!std::is_reference_v<T_>)
  explicit constexpr strong_type(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    : value_{std::move(value)}
  {
  }

  template <typename... Args>
    requires(sizeof...(Args) > 1 || (sizeof...(Args) == 1 && !detail::is_single_underlying_arg_v<T, Args...>)) &&
            detail::NonNarrowingConstructible<T, Args...>
  explicit constexpr strong_type(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    : value_{std::forward<Args>(args)...}
  {
  }

  [[nodiscard]] constexpr T& get() noexcept
  {
    return value_;
  }

  [[nodiscard]] constexpr const std::remove_reference_t<T>& get() const noexcept
  {
    return value_;
  }

  constexpr operator ref()
    requires(!std::is_reference_v<T>)
  {
    return ref{value_};
  }

  struct argument
  {
    strong_type operator=(T&& value) const
    {
      return strong_type{std::forward<T>(value)};
    }

    template <typename U>
      requires detail::NonNarrowingConstructible<T, U>
    strong_type operator=(U&& value) const
    {
      return strong_type{std::forward<U>(value)};
    }

    argument() = default;
    argument(const argument&) = delete;
    argument(argument&&) = delete;
    argument& operator=(const argument&) = delete;
    argument& operator=(argument&&) = delete;
  };

private:
  T value_;
};

template <typename T>
struct is_strong_type : std::false_type
{
};

template <typename T, typename Parameter, template <typename> class... Skills>
struct is_strong_type<strong_type<T, Parameter, Skills...>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_strong_type_v = is_strong_type<std::remove_cvref_t<T>>::value;

template <typename T, typename Parameter, template <typename> class... Skills>
  requires strong_type<T, Parameter, Skills...>::is_hashable
[[nodiscard]] std::size_t hash_value(const strong_type<T, Parameter, Skills...>& x) noexcept
{
  return std::hash<T>{}(x.get());
}

template <typename T, typename Parameter, template <typename> class... Skills>
  requires strong_type<T, Parameter, Skills...>::is_printable
std::ostream& operator<<(std::ostream& os, const strong_type<T, Parameter, Skills...>& object)
{
  object.print(os);
  return os;
}

} // namespace kraken

namespace std {
template <typename T, typename Parameter, template <typename> class... Skills>
struct hash<kraken::strong_type<T, Parameter, Skills...>>
{
  [[nodiscard]] std::size_t operator()(const kraken::strong_type<T, Parameter, Skills...>& x) const noexcept
  {
    static_assert(noexcept(std::hash<T>{}(x.get())));

    return std::hash<T>{}(x.get());
  }
};
} // namespace std

template <typename T, typename Parameter, template <typename> class... Skills>
struct fmt::formatter<kraken::strong_type<T, Parameter, Skills...>>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const kraken::strong_type<T, Parameter, Skills...>& val, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "{0}", val.get());
  }
};

#endif /* KRAKEN_STRONG_TYPE_HPP */
