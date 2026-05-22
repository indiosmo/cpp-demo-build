#ifndef LAB_RESULT_HPP
#define LAB_RESULT_HPP

#include "boost/algorithm/string/replace.hpp"
#include "boost/leaf.hpp"

#include "lab/error.hpp"
#include "lab/fmt.hpp"

#include <cstdlib>
#include <sstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

// clang-format off

/*
 * Top-level handler macros for boost::leaf::try_handle_all. Each emits one
 * lambda; CATCH_ALL chains the three in order (lab::error, std::exception,
 * then any std::error_code). code_block runs after the diagnostic write,
 * typically returning a failure value.
 */
#define LAB_RESULT_CATCH_EXCEPTION(code_block)                                                          \
  [&]([[maybe_unused]] const std::exception& ex, [[maybe_unused]] const boost::leaf::diagnostic_info& diag) { \
    std::fprintf(stderr, "unhandled exception | %s\n", ex.what());                                         \
    code_block                                                                                             \
  },

#define LAB_RESULT_CATCH_LAB_ERROR(code_block)                                                       \
  [&]([[maybe_unused]] const lab::error& err, [[maybe_unused]] const boost::leaf::diagnostic_info& diag) { \
    std::fprintf(stderr, "unhandled lab::error | %s\n", err.full_details().c_str());                    \
    code_block                                                                                             \
  },

#define LAB_RESULT_CATCH_ALL(code_block)                                                                \
  LAB_RESULT_CATCH_LAB_ERROR(code_block)                                                             \
  LAB_RESULT_CATCH_EXCEPTION(code_block)                                                                \
  [&]([[maybe_unused]] const boost::leaf::diagnostic_info& diag,                                           \
      [[maybe_unused]] const std::error_code* ec) {                                                        \
    if (ec) {                                                                                              \
      std::fprintf(stderr, "unhandled error | error_code: %d:%s\n", ec->value(), ec->category().name());   \
    } else {                                                                                               \
      std::fprintf(stderr, "unhandled error\n");                                                           \
    }                                                                                                      \
    code_block                                                                                             \
  }

#define LAB_RESULT_DISCARD_ERRORS() [](){}

#define LAB_RESULT_TOKEN_PASTE(x, y, z)  x ## y ## z
#define LAB_RESULT_TOKEN_PASTE3(x, y, z) LAB_RESULT_TOKEN_PASTE(x, y, z)
#define LAB_RESULT_TMP(prefix)           LAB_RESULT_TOKEN_PASTE3(prefix, _boost_leaf_tmp_, __LINE__)

/*
 * Portable BOOST_LEAF_CHECK replacement. The upstream macro relies on a GCC
 * extension that clang accepts but warns about, so we redefine it in plain
 * C++ to keep -Werror builds clean across both compilers.
 */
#define LAB_LEAF_CHECK(r)                                                                                  \
  do {                                                                                                        \
    auto&& BOOST_LEAF_TMP = (r);                                                                              \
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(BOOST_LEAF_TMP)>::type>::value);  \
    if (!BOOST_LEAF_TMP) {                                                                                    \
      return BOOST_LEAF_TMP.error();                                                                          \
    }                                                                                                         \
  } while (false)

#undef BOOST_LEAF_CHECK
#define BOOST_LEAF_CHECK(r) LAB_LEAF_CHECK(r)

/*
 * BOOST_LEAF_ASSIGN variant that annotates the failure path with an extra
 * lab::error before propagating, so handlers see both the inner cause and
 * the contextual wrapper.
 */
#define LAB_RESULT_ASSIGN_ERROR(v, r, ...)                                                                 \
  auto&& BOOST_LEAF_TMP = r;                                                                                  \
  {                                                                                                           \
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(BOOST_LEAF_TMP)>::type>::value);  \
    if (!BOOST_LEAF_TMP) {                                                                                    \
      auto LAB_RESULT_TMP(lab) = boost::leaf::on_error(lab::error{__VA_ARGS__});                     \
      return boost::leaf::new_error(BOOST_LEAF_TMP.error());                                                  \
    }                                                                                                         \
  }                                                                                                           \
  v = std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value()

#define LAB_RESULT_CHECK_ERROR(r, ...)                                                                     \
  {                                                                                                           \
    auto&& BOOST_LEAF_TMP = (r);                                                                              \
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(BOOST_LEAF_TMP)>::type>::value);  \
    if (!BOOST_LEAF_TMP) {                                                                                    \
      auto LAB_RESULT_TMP(lab) = boost::leaf::on_error(lab::error{__VA_ARGS__});                     \
      return boost::leaf::new_error(BOOST_LEAF_TMP.error());                                                  \
    }                                                                                                         \
  }

/*
 * Collapses a result-returning expression to bool by consuming any failure
 * with LAB_RESULT_CATCH_ALL (which logs through stderr). Building block
 * for the top-level "fail loud and exit" helpers below.
 */
#define LAB_RESULT_TO_BOOL(func)                                                                           \
  [&]() -> bool {                                                                                             \
    return boost::leaf::try_handle_all(                                                                       \
      [&]() -> lab::result<bool> {                                                                         \
        LAB_LEAF_CHECK(func);                                                                              \
        return true;                                                                                          \
      },                                                                                                      \
      LAB_RESULT_CATCH_ALL(return false;));                                                                \
  }()

/*
 * Consume a result-returning call at main(): on failure, log via
 * LAB_RESULT_CATCH_ALL and return EXIT_FAILURE from the enclosing function.
 */
#define LAB_EXIT_ON_ERROR(func) \
  if (!LAB_RESULT_TO_BOOL(func)) { return EXIT_FAILURE; }

/*
 * Test-only BOOST_LEAF_ASSIGN that REQUIREs success, so the test stops at the
 * offending call instead of cascading.
 */
#define LAB_REQUIRE_LEAF(v, r)                                                                             \
  auto&& BOOST_LEAF_TMP = boost::leaf::try_handle_some(                                                       \
    [&]() { return r; },                                                                                      \
    LAB_RESULT_CATCH_ALL(return boost::leaf::new_error();)                                                 \
  );                                                                                                          \
  REQUIRE(BOOST_LEAF_TMP.has_value());                                                                        \
  v = std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value()

// clang-format on

/*
 * BOOST_LEAF_AUTO silently copies even when the source is result<T&>. Force
 * callers to spell out the assign target via BOOST_LEAF_ASSIGN.
 */
#undef BOOST_LEAF_AUTO
#define BOOST_LEAF_AUTO                                                         \
  static_assert(                                                                \
    false,                                                                      \
    "BOOST_LEAF_AUTO always copies even if the function returns a result<T&>. " \
    "Use BOOST_LEAF_ASSIGN and be explicit about the target, "                  \
    "e.g. BOOST_LEAF_ASSIGN(const auto& val, myfunc())")

/* Aliased from boost::leaf so a swap is a one-header change. */
namespace lab {

using boost::leaf::result;

// clang-format off
template <typename T>
concept LeafDiagnostic =
  std::is_same_v<std::decay_t<std::remove_pointer_t<T>>, boost::leaf::error_info> ||
  std::is_same_v<std::decay_t<std::remove_pointer_t<T>>, boost::leaf::diagnostic_info> ||
  std::is_same_v<std::decay_t<std::remove_pointer_t<T>>, boost::leaf::verbose_diagnostic_info>;
// clang-format on

template <typename T>
concept Result = boost::leaf::is_result_type<T>::value;

} // namespace lab

/*
 * fmt support for leaf's diagnostic types. Streams through the type's ostream
 * operator and folds embedded newlines into " -- " so it fits on one log line.
 */
template <typename T>
  requires lab::LeafDiagnostic<T>
struct fmt::formatter<T>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const
  {
    std::ostringstream oss;
    oss << value;

    std::string output = oss.str();
    boost::algorithm::replace_all(output, "\n", " -- ");

    return fmt::format_to(ctx.out(), "{0}", output);
  }
};

#endif /* LAB_RESULT_HPP */
