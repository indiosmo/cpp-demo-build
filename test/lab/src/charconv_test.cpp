#include "catch2/catch_test_macros.hpp"

#include "lab/charconv.hpp"
#include "lab/fixed_string.hpp"
#include "lab/strong_type.hpp"

#include <cstdint>

namespace {

using sequence_id = lab::strong_type<std::uint64_t, struct SequenceIdTag>;

} // namespace

TEST_CASE("from_chars - integral")
{
  const auto parsed = lab::from_chars<std::uint64_t>("123");

  REQUIRE(parsed);
  REQUIRE(parsed.value() == 123);
}

TEST_CASE("from_chars - rejects trailing characters")
{
  const auto parsed = lab::from_chars<std::uint64_t>("123x");

  REQUIRE_FALSE(parsed);
}

TEST_CASE("from_chars - strong type")
{
  const auto parsed = lab::from_chars<sequence_id>("42");

  REQUIRE(parsed);
  REQUIRE(parsed.value().get() == 42);
}

TEST_CASE("from_chars - fixed_string input")
{
  const auto text = lab::fixed_string<8>::truncate_from("77");
  const auto parsed = lab::from_chars<sequence_id>(text);

  REQUIRE(parsed);
  REQUIRE(parsed.value().get() == 77);
}
