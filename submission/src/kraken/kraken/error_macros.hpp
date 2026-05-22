#ifndef KRAKEN_ERROR_MACROS_HPP
#define KRAKEN_ERROR_MACROS_HPP

#include <concepts>
#include <string>
#include <string_view>
#include <system_error>

/*
 * Turns an `enum class error_code` in namespace NS into a usable
 * std::error_code domain: error_category subclass driven by NS::to_string,
 * free make_error_code in NS, and the is_error_code_enum specialisation.
 * Use once per domain, immediately after the error_code enum.
 */
#define KRAKEN_DEFINE_ERROR_CATEGORY(NS)                              \
  namespace NS::detail {                                              \
                                                                      \
  struct error_category : public std::error_category                  \
  {                                                                   \
  private:                                                            \
    template <typename T>                                             \
      requires std::convertible_to<T, std::string_view>               \
    static constexpr std::string string_cast(const T& t)              \
    {                                                                 \
      return std::string{std::string_view{t}};                        \
    }                                                                 \
                                                                      \
  public:                                                             \
    const char* name() const noexcept override                        \
    {                                                                 \
      return #NS "::error_category";                                  \
    }                                                                 \
                                                                      \
    std::string message(int ec) const override                        \
    {                                                                 \
      return string_cast(NS::to_string(static_cast<error_code>(ec))); \
    }                                                                 \
  };                                                                  \
                                                                      \
  inline const std::error_category& category()                        \
  {                                                                   \
    static error_category instance;                                   \
    return instance;                                                  \
  }                                                                   \
                                                                      \
  } /* namespace NS::detail*/                                         \
                                                                      \
  namespace NS {                                                      \
  [[nodiscard]] inline std::error_code make_error_code(error_code e)  \
  {                                                                   \
    return std::error_code{static_cast<int>(e), detail::category()};  \
  }                                                                   \
  } /* namespace NS */                                                \
                                                                      \
  namespace std {                                                     \
  template <>                                                         \
  struct is_error_code_enum<NS::error_code> : true_type               \
  {                                                                   \
  };                                                                  \
  }

#endif /* KRAKEN_ERROR_MACROS_HPP */
