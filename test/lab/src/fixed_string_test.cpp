#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators_all.hpp"

#include "lab/error.hpp"
#include "lab/fixed_string.hpp"
#include "lab/result.hpp"

#include <boost/leaf/handle_errors.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace {

using fs_strict = lab::fixed_string<5>;
using fs_auto = lab::fixed_string<5, lab::fixed_string_truncation_policy::auto_truncate>;
using fs_large = lab::fixed_string<8>;

template <typename T>
concept has_truncate_from = requires (std::string_view sv) { T::truncate_from(sv); };

template <typename Fn>
std::error_code capture_error_code(Fn&& fn)
{
  return boost::leaf::try_handle_all(
           [&]() -> lab::result<std::optional<std::error_code>> {
             auto result = std::forward<Fn>(fn)();
             if (result) {
               return std::nullopt;
             }

             return result.error();
           },
           [](const lab::error& err) { return std::optional{err.error_code()}; },
           [](const std::error_code& ec) { return std::optional{ec}; },
           [] { return std::optional<std::error_code>{}; })
    .value_or(std::error_code{});
}

static_assert(has_truncate_from<fs_strict>);
static_assert(!has_truncate_from<fs_auto>);

static_assert(!std::is_constructible_v<fs_strict, const char (&)[7]>);
static_assert(std::is_constructible_v<fs_strict, const char (&)[6]>);

static_assert(!std::is_constructible_v<fs_strict, fs_large>);
static_assert(std::is_constructible_v<fs_large, fs_strict>);

static_assert(!std::is_constructible_v<fs_strict, std::string_view>);
static_assert(std::is_constructible_v<fs_auto, std::string_view>);

static_assert(fs_strict::max_size == 5);
static_assert(fs_large::max_size == 8);

static_assert(lab::is_fixed_string_v<fs_strict>);
static_assert(lab::is_fixed_string_v<fs_auto>);
static_assert(lab::FixedString<fs_strict>);
static_assert(!lab::FixedString<std::string>);

static_assert(std::is_same_v<lab::truncating_fixed_string<5>, fs_auto>);

} // namespace

TEST_CASE("fixed_string - strict construction")
{
  using fs = fs_strict;

  SECTION("default constructor yields empty string")
  {
    fs value{};
    CHECK(value.empty());
    CHECK(value.size() == 0);
    CHECK(value == "");
  }

  SECTION("from literal")
  {
    auto x = fs{"12345"};
    REQUIRE(x == "12345");
    CHECK(!x.truncated());
  }

  SECTION("from array")
  {
    char arr[6]{'1', '2', '3', '4', '5', '\0'};
    auto x = fs{arr};
    REQUIRE(x == "12345");
    REQUIRE(std::strcmp(x.c_str(), "12345") == 0);
    CHECK(!x.truncated());
  }

  SECTION("from array not null terminated")
  {
    char arr[6]{'1', '2', '3', '4', '5', '6'};
    auto x = fs{arr};
    REQUIRE(x == "12345");
    REQUIRE(std::strcmp(x.c_str(), "12345") == 0);
    CHECK(!x.truncated());
  }

  SECTION("from empty literal")
  {
    auto x = fs{""};
    REQUIRE(x == "");
    CHECK(x.empty());
  }
}

TEST_CASE("fixed_string - strict string factories")
{
  using fs = fs_strict;

  SECTION("from string_view success")
  {
    LAB_REQUIRE_LEAF(auto value, fs::from(std::string_view{"12345"}));
    CHECK(value == "12345");
    CHECK(!value.truncated());
  }

  SECTION("from string_view overflow")
  {
    auto ec = capture_error_code([] { return fs::from(std::string_view{"123456"}); });
    CHECK(ec == lab::error_code::out_of_bounds);
  }

  SECTION("from pointer and size success")
  {
    char raw[6]{'1', '2', '3', '4', '5', '6'};
    LAB_REQUIRE_LEAF(auto value, fs::from(raw, 5));
    CHECK(value == "12345");
    CHECK(!value.truncated());
  }

  SECTION("from pointer and size overflow")
  {
    char raw[6]{'1', '2', '3', '4', '5', '6'};
    auto ec = capture_error_code([&] { return fs::from(raw, 6); });
    CHECK(ec == lab::error_code::out_of_bounds);
  }

  SECTION("truncate_from marks truncation only on overflow")
  {
    auto truncated = fs::truncate_from("123456");
    CHECK(truncated == "12345");
    CHECK(truncated.truncated());

    auto not_truncated = fs::truncate_from("12345");
    CHECK(not_truncated == "12345");
    CHECK(!not_truncated.truncated());
  }
}

TEST_CASE("fixed_string - integral factory")
{
  using fs = fs_strict;

  SECTION("success paths")
  {
    // clang-format off
    auto [label, input, expected] = GENERATE(
      table<const char*, std::int64_t, std::string_view>({
        {"size less",   12,    "12"},
        {"size equals", 12345, "12345"},
        {"negative",    -1234, "-1234"},
      }));
    // clang-format on
    CAPTURE(label, input);

    LAB_REQUIRE_LEAF(auto value, fs::from(input));
    CHECK(value == expected);
    CHECK(!value.truncated());
    CHECK(static_cast<std::size_t>(value.size()) == expected.size());
  }

  SECTION("out_of_bounds paths")
  {
    // clang-format off
    auto [label, input] = GENERATE(
      table<const char*, std::int64_t>({
        {"too large positive", 123456},
        {"too large negative", -12345},
      }));
    // clang-format on
    CAPTURE(label, input);

    auto ec = capture_error_code([input] { return fs::from(input); });
    CHECK(ec == lab::error_code::out_of_bounds);
  }

  SECTION("roundtrip")
  {
    using clordid = lab::fixed_string<40>;
    const long value = 265944835702835;
    LAB_REQUIRE_LEAF(auto str, clordid::from(value));
    CHECK(str.to_string_view() == "265944835702835");
  }

  SECTION("auto_truncate policy keeps strict integral behavior")
  {
    std::int32_t i = 123456;
    auto ec = capture_error_code([i] { return fs_auto::from(i); });
    CHECK(ec == lab::error_code::out_of_bounds);
  }
}

TEST_CASE("fixed_string - auto_truncate constructors")
{
  SECTION("from string_view constructor")
  {
    auto truncated = fs_auto{"123456"};
    CHECK(truncated == "12345");
    CHECK(truncated.truncated());

    auto not_truncated = fs_auto{"12345"};
    CHECK(not_truncated == "12345");
    CHECK(!not_truncated.truncated());
  }

  SECTION("from pointer and size constructor")
  {
    char input[6]{'1', '2', '3', '4', '5', '6'};

    auto truncated = fs_auto{input, 6};
    CHECK(truncated == "12345");
    CHECK(truncated.truncated());

    auto exact = fs_auto{input, 5};
    CHECK(exact == "12345");
    CHECK(!exact.truncated());
  }

  SECTION("alias behaves as auto_truncate")
  {
    lab::truncating_fixed_string<5> value{"123456"};
    CHECK(value == "12345");
    CHECK(value.truncated());
  }
}

TEST_CASE("fixed_string - observers and conversions")
{
  using fs = fs_strict;
  auto value = fs{"abc"};

  CHECK(value.size() == 3);
  CHECK(!value.empty());
  CHECK(std::strcmp(value.c_str(), "abc") == 0);
  CHECK(value.to_string() == "abc");
  CHECK(value.to_string_view() == "abc");

  const std::string_view implicit_sv = value;
  CHECK(implicit_sv == "abc");

  auto length_prefixed = value.length_prefixed_string();
  CHECK(static_cast<std::uint8_t>(length_prefixed[0]) == value.size());
  CHECK(std::string_view{length_prefixed + 1, value.size()} == "abc");
  CHECK(length_prefixed[1 + value.size()] == '\0');
}

TEST_CASE("fixed_string - predicates and comparisons")
{
  using fs = fs_strict;
  auto value = fs{"hello"};

  CHECK(value.contains("ell"));
  CHECK(!value.contains("x"));

  CHECK(value.starts_with("he"));
  CHECK(!value.starts_with("el"));

  CHECK(value.ends_with("lo"));
  CHECK(!value.ends_with("ll"));

  CHECK(value == std::string_view{"hello"});
  CHECK(value != std::string_view{"HELLO"});

  CHECK(value == "hello");
  CHECK(value != "HELLO");
}

TEST_CASE("fixed_string - remove_suffix semantics")
{
  using fs = lab::fixed_string<5>;

  // clang-format off
  auto [label, input, suffix, expected] = GENERATE(
    table<const char*, const char*, const char*, const char*>({
      {"single character suffix",    "hello", "o",     "hell"},
      {"multi-character suffix",     "hello", "lo",    "hel"},
      {"entire string as suffix",    "test",  "test",  ""},
      {"whitespace suffix",          "hi ",   " ",     "hi"},
      {"multiple spaces suffix",     "ab  ",  "  ",    "ab"},
      {"different ending",           "hello", "x",     "hello"},
      {"case sensitive mismatch",    "hello", "O",     "hello"},
      {"suffix in middle",           "hello", "el",    "hello"},
      {"suffix longer than string",  "hi",    "hello", "hi"},
      {"empty string non-empty arg", "",      "x",     ""},
      {"empty string empty arg",     "",      "",      ""},
      {"non-empty string empty arg", "hello", "",      "hello"},
    }));
  // clang-format on

  CAPTURE(label, input, suffix);
  LAB_REQUIRE_LEAF(auto value, fs::from(std::string_view{input}));
  CHECK(value.remove_suffix(suffix) == expected);
}

TEST_CASE("fixed_string - cross-size copy/move propagates truncated")
{
  using fs5 = lab::fixed_string<5>;
  using fs8 = lab::fixed_string<8>;
  using fs5_auto = lab::fixed_string<5, lab::fixed_string_truncation_policy::auto_truncate>;

  auto src = fs5::truncate_from("123456789");
  REQUIRE(src.truncated());
  REQUIRE(src == "12345");

  auto require_truncated_value = [](const auto& value) {
    CHECK(value == "12345");
    CHECK(value.truncated());
  };

  SECTION("copy construct")
  {
    fs8 dst{src};
    require_truncated_value(dst);
  }

  SECTION("move construct")
  {
    auto tmp = src;
    fs8 dst{std::move(tmp)};
    require_truncated_value(dst);
  }

  SECTION("copy assign")
  {
    fs8 dst{"hi"};
    dst = src;
    require_truncated_value(dst);
  }

  SECTION("move assign")
  {
    fs8 dst{"hi"};
    dst = std::move(src);
    require_truncated_value(dst);
  }

  SECTION("copy assign clears truncated")
  {
    fs8 dst{"hi"};
    dst = src;
    fs5 clean{"abc"};
    dst = clean;
    CHECK(dst == "abc");
    CHECK(!dst.truncated());
  }

  SECTION("move assign clears truncated")
  {
    fs8 dst{"hi"};
    dst = src;
    auto clean = fs5{"abc"};
    dst = std::move(clean);
    CHECK(dst == "abc");
    CHECK(!dst.truncated());
  }

  SECTION("copy from auto_truncate policy propagates truncated")
  {
    fs5_auto auto_src{"123456"};
    REQUIRE(auto_src.truncated());

    fs8 dst{auto_src};
    require_truncated_value(dst);
  }

  SECTION("assign from auto_truncate policy propagates truncated")
  {
    fs5_auto auto_src{"123456"};
    REQUIRE(auto_src.truncated());

    fs8 dst{"hi"};
    dst = auto_src;
    require_truncated_value(dst);
  }
}

TEST_CASE("fixed_string - remove_suffix preserves truncated flag")
{
  using fs = lab::fixed_string<5>;

  // clang-format off
  auto [label, source, suffix, expected, expected_truncated] = GENERATE(
    table<const char*, const char*, const char*, const char*, bool>({
      {"truncated source stays truncated",         "123456789", "5", "1234",  true},
      {"non-truncated source stays non-truncated", "abc",       "c", "ab",    false},
      {"truncated source with no suffix match",    "123456",    "x", "12345", true},
    }));
  // clang-format on

  CAPTURE(label, source, suffix);

  auto value = fs::truncate_from(source);
  auto result = value.remove_suffix(suffix);

  CHECK(result == expected);
  CHECK(result.truncated() == expected_truncated);
}

TEST_CASE("fixed_string - hash")
{
  auto a = fs_strict{"hello"};
  auto b = fs_strict{"hello"};
  auto c = fs_strict{"world"};

  SECTION("std::hash - equal values hash equal")
  {
    auto h = std::hash<fs_strict>{};
    CHECK(h(a) == h(b));
    CHECK(h(a) != h(c));
  }

  SECTION("boost hash_value - equal values hash equal")
  {
    CHECK(hash_value(a) == hash_value(b));
    CHECK(hash_value(a) != hash_value(c));
  }
}

TEST_CASE("fixed_string - fmt formatter")
{
  auto value = fs_strict{"hello"};
  CHECK(fmt::format("{}", value) == "hello");
  CHECK(fmt::format("[{}]", fs_strict{""}) == "[]");
}

TEST_CASE("fixed_string - ostream")
{
  auto value = fs_strict{"hello"};
  std::ostringstream os;
  os << value;
  CHECK(os.str() == "hello");
}
