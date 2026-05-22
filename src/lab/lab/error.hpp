#ifndef LAB_ERROR_HPP
#define LAB_ERROR_HPP

#include "boost/leaf/handle_errors.hpp"
#include "boost/leaf/result.hpp"

#include "lab/error_code.hpp"
#include "lab/errors.hpp"
#include "lab/fmt.hpp"

#include <memory>
#include <optional>
#include <source_location>
#include <string>
#include <system_error>
#include <utility>

namespace lab {

template <typename T>
concept ErrorData = requires (T t) {
  { t.error_code() } -> std::same_as<std::error_code>;
  { t.what() } -> std::same_as<std::string>;
};

/*
 * Type-erased error carrier. Wraps any ErrorData payload so boost::leaf can
 * transport heterogeneous structured errors as a single value type; handlers
 * recover the concrete type via data<T>() / is_type<T>(). Each error captures
 * its construction source_location for diagnostics.
 */
class error
{
public:
  template <ErrorData T>
  explicit error(T&& value, std::source_location loc = std::source_location::current())
    : location_{loc}
    , value_{std::make_unique<concrete_error<std::remove_cvref_t<T>>>(std::forward<T>(value))}
  {
  }

  std::error_code error_code() const
  {
    return value_->error_code();
  }

  std::string what() const
  {
    return fmt::format("{} | {}", error_code().message(), value_->what());
  }

  std::string full_details() const
  {
    return fmt::format("{}:{} | {} | {}", location_.file_name(), location_.line(), error_code().message(), what());
  }

  std::source_location location() const
  {
    return location_;
  }

  template <ErrorData T>
  bool is_type() const
  {
    return dynamic_cast<concrete_error<T>*>(value_.get()) != nullptr;
  }

  template <ErrorData T>
  T* data()
  {
    if (auto* concrete = dynamic_cast<concrete_error<T>*>(value_.get())) {
      return &concrete->value;
    } else {
      return nullptr;
    }
  }

  template <ErrorData T>
  const T* data() const
  {
    if (auto* concrete = dynamic_cast<const concrete_error<T>*>(value_.get())) {
      return &concrete->value;
    } else {
      return nullptr;
    }
  }

private:
  struct error_base
  {
    virtual ~error_base() = default;
    virtual std::error_code error_code() const = 0;
    virtual std::string what() const = 0;
  };

  template <ErrorData T>
  struct concrete_error final : public error_base
  {
    concrete_error(T value)
      : value{std::move(value)}
    {
    }

    std::error_code error_code() const final
    {
      return value.error_code();
    }

    std::string what() const final
    {
      return value.what();
    }

    T value;
  };

  std::source_location location_;
  std::unique_ptr<error_base> value_;
};

/*
 * boost::leaf predicate adapters: handlers select on the concrete payload
 * type carried inside a lab::error.
 */
template <ErrorData T>
struct match_error
{
  using error_type = error;
  const error& matched;

  constexpr static bool evaluate(const error& err) noexcept
  {
    return err.is_type<T>();
  }

  const T& value() const
  {
    return *matched.data<T>();
  }
};

template <ErrorData... Ts>
struct match_errors
{
  using error_type = error;
  const error& matched;

  constexpr static bool evaluate(const error& err) noexcept
  {
    return (... || err.is_type<Ts>());
  }
};

template <ErrorData T>
auto make_leaf_error(T&& value, std::source_location loc = std::source_location::current())
{
  return boost::leaf::new_error(error{std::forward<T>(value), loc});
}

inline auto make_leaf_error(
  std::error_code ec, std::optional<std::string> text = {}, std::source_location loc = std::source_location::current())
{
  return make_leaf_error(lab::errors::generic_error{.ec = ec, .text{std::move(text)}}, loc);
}

} // namespace lab

namespace boost::leaf {

template <lab::ErrorData T>
struct is_predicate<lab::match_error<T>> : std::true_type
{
};

template <lab::ErrorData... Ts>
struct is_predicate<lab::match_errors<Ts...>> : std::true_type
{
};

} // namespace boost::leaf

#endif /* LAB_ERROR_HPP */
