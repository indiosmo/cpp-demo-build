#include "catch2/catch_test_macros.hpp"

#include "kraken/algorithm.hpp"

#include <string_view>

using namespace std::string_view_literals;

TEST_CASE("trim - no whitespace")
{
  REQUIRE(kraken::trim("hello") == "hello"sv);
  REQUIRE(kraken::trim("a") == "a"sv);
}

TEST_CASE("trim - leading whitespace")
{
  REQUIRE(kraken::trim("   value") == "value"sv);
  REQUIRE(kraken::trim("\t\t\tvalue") == "value"sv);
}

TEST_CASE("trim - trailing whitespace")
{
  REQUIRE(kraken::trim("value   ") == "value"sv);
  REQUIRE(kraken::trim("value\r\n") == "value"sv);
}

TEST_CASE("trim - both sides")
{
  REQUIRE(kraken::trim("  value  ") == "value"sv);
  REQUIRE(kraken::trim("\t value \n") == "value"sv);
}

TEST_CASE("trim - preserves inner whitespace")
{
  REQUIRE(kraken::trim("  a b c  ") == "a b c"sv);
  REQUIRE(kraken::trim("  N, 1, IBM  ") == "N, 1, IBM"sv);
}

TEST_CASE("trim - all whitespace")
{
  REQUIRE(kraken::trim("   ").empty());
  REQUIRE(kraken::trim(" \t\r\n").empty());
  REQUIRE(kraken::trim("").empty());
}

TEST_CASE("trim - default character set")
{
  REQUIRE(kraken::trim(" payload") == "payload"sv);
  REQUIRE(kraken::trim("\tpayload") == "payload"sv);
  REQUIRE(kraken::trim("\rpayload") == "payload"sv);
  REQUIRE(kraken::trim("\npayload") == "payload"sv);
}

TEST_CASE("trim - non-default characters preserved")
{
  REQUIRE(kraken::trim("xxvaluexx") == "xxvaluexx"sv);
}

TEST_CASE("trim - explicit character set")
{
  REQUIRE(kraken::trim("xxvaluexx", "x") == "value"sv);
  REQUIRE(kraken::trim("--value--", "-") == "value"sv);
  REQUIRE(kraken::trim("xyvaluexy", "xy") == "value"sv);
}

TEST_CASE("ltrim - leading only")
{
  REQUIRE(kraken::ltrim("  value  ") == "value  "sv);
  REQUIRE(kraken::ltrim("value") == "value"sv);
  REQUIRE(kraken::ltrim("   ").empty());
}

TEST_CASE("rtrim - trailing only")
{
  REQUIRE(kraken::rtrim("  value  ") == "  value"sv);
  REQUIRE(kraken::rtrim("value") == "value"sv);
  REQUIRE(kraken::rtrim("   ").empty());
}

TEST_CASE("trim - constexpr")
{
  static_assert(kraken::trim("  hi  ") == "hi"sv);
  static_assert(kraken::ltrim("  hi  ") == "hi  "sv);
  static_assert(kraken::rtrim("  hi  ") == "  hi"sv);
  static_assert(kraken::trim("   ").empty());
}

TEST_CASE("split_fields - N tokens")
{
  const auto fields = kraken::split_fields<3>("a,b,c");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - trims tokens")
{
  const auto fields = kraken::split_fields<3>(" a , b\t,\r\nc ");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - preserves inner whitespace")
{
  const auto fields = kraken::split_fields<2>(" a b , c d ");

  REQUIRE(fields[0] == "a b"sv);
  REQUIRE(fields[1] == "c d"sv);
}

TEST_CASE("split_fields - empty slots")
{
  const auto fields = kraken::split_fields<3>("a,,c");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1].empty());
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - trailing empty slot")
{
  const auto fields = kraken::split_fields<3>("a,b,");

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2].empty());
}

TEST_CASE("split_fields - new_order body")
{
  // Mirrors the body that csv_decoder hands in after stripping the marker and
  // its trailing comma from "N, 1, IBM, 10, 100, B, 1".
  const auto fields = kraken::split_fields<6>(" 1, IBM, 10, 100, B, 1");

  REQUIRE(fields[0] == "1"sv);
  REQUIRE(fields[1] == "IBM"sv);
  REQUIRE(fields[2] == "10"sv);
  REQUIRE(fields[3] == "100"sv);
  REQUIRE(fields[4] == "B"sv);
  REQUIRE(fields[5] == "1"sv);
}

TEST_CASE("split_fields - custom delimiter")
{
  const auto fields = kraken::split_fields<3>("a|b|c", '|');

  REQUIRE(fields[0] == "a"sv);
  REQUIRE(fields[1] == "b"sv);
  REQUIRE(fields[2] == "c"sv);
}

TEST_CASE("split_fields - arity 1")
{
  const auto fields = kraken::split_fields<1>("  only  ");

  REQUIRE(fields[0] == "only"sv);
}
