#include "catch2/catch_test_macros.hpp"

#include "kraken/charconv.hpp"
#include "kraken/fixed_string.hpp"
#include "kraken/strong_type.hpp"

#include <cstdint>

namespace {

using sequence_id = kraken::strong_type<std::uint64_t, struct SequenceIdTag>;

} // namespace

TEST_CASE("from_chars - integral")
{
  const auto parsed = kraken::from_chars<std::uint64_t>("123");

  REQUIRE(parsed);
  REQUIRE(parsed.value() == 123);
}

TEST_CASE("from_chars - rejects trailing characters")
{
  const auto parsed = kraken::from_chars<std::uint64_t>("123x");

  REQUIRE_FALSE(parsed);
}

TEST_CASE("from_chars - strong type")
{
  const auto parsed = kraken::from_chars<sequence_id>("42");

  REQUIRE(parsed);
  REQUIRE(parsed.value().get() == 42);
}

TEST_CASE("from_chars - fixed_string input")
{
  const auto text = kraken::fixed_string<8>::truncate_from("77");
  const auto parsed = kraken::from_chars<sequence_id>(text);

  REQUIRE(parsed);
  REQUIRE(parsed.value().get() == 77);
}
