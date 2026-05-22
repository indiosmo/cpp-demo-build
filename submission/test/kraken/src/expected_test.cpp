#include "catch2/catch_test_macros.hpp"

#include "kraken/expected.hpp"

#include <string>

TEST_CASE("expected - value")
{
  const auto parsed = kraken::expected<int, std::string>{42};

  REQUIRE(parsed);
  REQUIRE(*parsed == 42);
}

TEST_CASE("expected - error")
{
  const auto parsed = kraken::expected<int, std::string>{kraken::make_unexpected("invalid")};

  REQUIRE_FALSE(parsed);
  REQUIRE(parsed.error() == "invalid");
}
