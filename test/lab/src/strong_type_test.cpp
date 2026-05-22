#include <catch2/catch_test_macros.hpp>

#include "lab/strong_type.hpp"

#include <concepts>
#include <functional>

namespace {

struct EqualityOnly
{
  int value;

  [[nodiscard]] friend constexpr bool operator==(const EqualityOnly&, const EqualityOnly&) = default;
};

struct Pair
{
  int left;
  int right;

  [[nodiscard]] friend constexpr bool operator==(const Pair&, const Pair&) = default;
};

using equality_only = lab::strong_type<EqualityOnly, struct EqualityOnlyTag>;
using pair_id = lab::strong_type<Pair, struct PairTag>;
using quantity = lab::strong_type<int, struct QuantityTag>;
using price = lab::strong_type<int, struct PriceTag>;

template <typename L, typename R>
concept HasMinus = requires (L left, R right) { left - right; };

template <typename L, typename R, typename T>
concept MinusReturns = requires (L left, R right) {
  { left - right } -> std::same_as<T>;
};

} // namespace

TEST_CASE("strong_type - equality")
{
  STATIC_REQUIRE(equality_only{EqualityOnly{7}} == equality_only{EqualityOnly{7}});
  STATIC_REQUIRE(equality_only{EqualityOnly{7}} != equality_only{EqualityOnly{8}});
}

TEST_CASE("strong_type - ordering")
{
  STATIC_REQUIRE(quantity{1} < quantity{2});
  STATIC_REQUIRE(quantity{2} > quantity{1});
  STATIC_REQUIRE(quantity{2} >= quantity{2});
  STATIC_REQUIRE(quantity{1} <= quantity{2});
}

TEST_CASE("strong_type - construction")
{
  STATIC_REQUIRE(pair_id{1, 2}.get() == Pair{.left = 1, .right = 2});
}

TEST_CASE("strong_type - hash and format")
{
  CHECK(std::hash<quantity>{}(quantity{42}) == std::hash<int>{}(42));
  CHECK(fmt::format("{}", quantity{42}) == "42");
}

TEST_CASE("strong_type - same-type arithmetic preserves the strong type")
{
  STATIC_REQUIRE(std::same_as<decltype(quantity{7} + quantity{3}), quantity>);
  STATIC_REQUIRE(std::same_as<decltype(quantity{7} - quantity{3}), quantity>);
  STATIC_REQUIRE(std::same_as<decltype(quantity{7} * quantity{3}), quantity>);
  STATIC_REQUIRE(std::same_as<decltype(quantity{7} / quantity{3}), quantity>);
  STATIC_REQUIRE(std::same_as<decltype(quantity{7} % quantity{3}), quantity>);
  STATIC_REQUIRE(std::same_as<decltype(+quantity{7}), quantity>);
  STATIC_REQUIRE(std::same_as<decltype(-quantity{7}), quantity>);

  STATIC_REQUIRE((quantity{7} + quantity{3}).get() == 10);
  STATIC_REQUIRE((quantity{7} - quantity{3}).get() == 4);
  STATIC_REQUIRE((quantity{7} * quantity{3}).get() == 21);
  STATIC_REQUIRE((quantity{7} / quantity{3}).get() == 2);
  STATIC_REQUIRE((quantity{7} % quantity{3}).get() == 1);

  auto remaining = quantity{10};
  remaining -= quantity{4};
  CHECK(remaining == quantity{6});

  remaining += quantity{5};
  CHECK(remaining == quantity{11});

  remaining *= quantity{2};
  CHECK(remaining == quantity{22});

  remaining /= quantity{11};
  CHECK(remaining == quantity{2});

  remaining %= quantity{2};
  CHECK(remaining == quantity{0});
}

TEST_CASE("strong_type - mixed arithmetic still promotes through Callable")
{
  STATIC_REQUIRE(MinusReturns<quantity, price, int>);
  STATIC_REQUIRE(MinusReturns<quantity, int, int>);
  STATIC_REQUIRE(MinusReturns<int, quantity, int>);

  STATIC_REQUIRE(quantity{7} - price{3} == 4);
  STATIC_REQUIRE(quantity{7} - 3 == 4);
  STATIC_REQUIRE(7 - quantity{3} == 4);
}

TEST_CASE("strong_type - arithmetic is only available when the underlying supports it")
{
  STATIC_REQUIRE(!HasMinus<pair_id, pair_id>);
}
