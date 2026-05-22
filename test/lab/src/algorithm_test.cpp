#include "catch2/catch_test_macros.hpp"
#include "lab/algorithm.hpp"

#include <string_view>

using namespace std::string_view_literals;

TEST_CASE("trim - no whitespace")
{
  REQUIRE(lab::trim("hello") == "hello"sv);
  REQUIRE(lab::trim("a") == "a"sv);
}

TEST_CASE("trim - leading whitespace")
{
  REQUIRE(lab::trim("   value") == "value"sv);
  REQUIRE(lab::trim("\t\t\tvalue") == "value"sv);
}

TEST_CASE("trim - trailing whitespace")
{
  REQUIRE(lab::trim("value   ") == "value"sv);
  REQUIRE(lab::trim("value\r\n") == "value"sv);
}

TEST_CASE("trim - both sides")
{
  REQUIRE(lab::trim("  value  ") == "value"sv);
  REQUIRE(lab::trim("\t value \n") == "value"sv);
}

TEST_CASE("trim - preserves inner whitespace")
{
  REQUIRE(lab::trim("  a b c  ") == "a b c"sv);
  REQUIRE(lab::trim("  N, 1, IBM  ") == "N, 1, IBM"sv);
}

TEST_CASE("trim - all whitespace")
{
  REQUIRE(lab::trim("   ").empty());
  REQUIRE(lab::trim(" \t\r\n").empty());
  REQUIRE(lab::trim("").empty());
}

TEST_CASE("trim - default character set")
{
  REQUIRE(lab::trim(" payload") == "payload"sv);
  REQUIRE(lab::trim("\tpayload") == "payload"sv);
  REQUIRE(lab::trim("\rpayload") == "payload"sv);
  REQUIRE(lab::trim("\npayload") == "payload"sv);
}

TEST_CASE("trim - non-default characters preserved")
{
  REQUIRE(lab::trim("xxvaluexx") == "xxvaluexx"sv);
}

TEST_CASE("trim - explicit character set")
{
  REQUIRE(lab::trim("xxvaluexx", "x") == "value"sv);
  REQUIRE(lab::trim("--value--", "-") == "value"sv);
  REQUIRE(lab::trim("xyvaluexy", "xy") == "value"sv);
}

TEST_CASE("ltrim - leading only")
{
  REQUIRE(lab::ltrim("  value  ") == "value  "sv);
  REQUIRE(lab::ltrim("value") == "value"sv);
  REQUIRE(lab::ltrim("   ").empty());
}

TEST_CASE("rtrim - trailing only")
{
  REQUIRE(lab::rtrim("  value  ") == "  value"sv);
  REQUIRE(lab::rtrim("value") == "value"sv);
  REQUIRE(lab::rtrim("   ").empty());
}

TEST_CASE("trim - constexpr")
{
  static_assert(lab::trim("  hi  ") == "hi"sv);
  static_assert(lab::ltrim("  hi  ") == "hi  "sv);
  static_assert(lab::rtrim("  hi  ") == "  hi"sv);
  static_assert(lab::trim("   ").empty());
}

TEST_CASE("split_fields - N tokens")
{
  const auto fields = lab::split_fields<3>("a,b,c");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - trims tokens")
{
  const auto fields = lab::split_fields<3>(" a , b\t,\r\nc ");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - preserves inner whitespace")
{
  const auto fields = lab::split_fields<2>(" a b , c d ");

  REQUIRE(fields[0] == "a b"sv);
  REQUIRE(fields[1] == "c d"sv);
}

TEST_CASE("split_fields - empty slots")
{
  const auto fields = lab::split_fields<3>("a,,c");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1].empty());
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - trailing empty slot")
{
  const auto fields = lab::split_fields<3>("a,b,");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2].empty());
}

TEST_CASE("split_fields - new_order body")
{
  // Mirrors the body a delimiter-based codec hands in after stripping a marker and
  // its trailing comma from "N, 1, IBM, 10, 100, B, 1".
  const auto fields = lab::split_fields<6>(" 1, IBM, 10, 100, B, 1");

  REQUIRE(fields[0] == "1"sv);
  REQUIRE(fields[1] == "IBM"sv);
  REQUIRE(fields[2] == "10"sv);
  REQUIRE(fields[3] == "100"sv);
  REQUIRE(fields[4] == "B"sv);
  REQUIRE(fields[5] == "1"sv);
}

TEST_CASE("split_fields - custom delimiter")
{
  const auto fields = lab::split_fields<3>("a|b|c", '|');

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - arity 1")
{
  const auto fields = lab::split_fields<1>("  only  ");

  REQUIRE(fields[0] == "only"sv);
}
