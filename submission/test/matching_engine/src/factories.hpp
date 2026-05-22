#ifndef MATCHING_ENGINE_TEST_FACTORIES_HPP
#define MATCHING_ENGINE_TEST_FACTORIES_HPP

#include "matching_engine/order_state.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include "kraken/fixed_string.hpp"

#include <cstdint>
#include <optional>

/*
 * Shared test helpers for the matching_engine component tests.
 *
 * The two factories produce the value types that bracket the matching
 * engine's surface: matching_engine::order_state for the resting-order tests
 * against the order book, and order_routing::new_order for the engine-level
 * tests that drive the matching loop end-to-end. Both share the same
 * order_params parameter struct (every field optional) following the factory
 * pattern in docs/cpp-guides/cpp-testing-principles/test-helpers.md, so a
 * call site states only the values that distinguish the order from the
 * default. The defaults are deliberately uninteresting (alice / AAPL / buy /
 * 10 / 100) so each call reads as "an order, with this one thing changed".
 *
 * The optional fields hold raw underlying primitives (uint64_t, fixed_string)
 * rather than the strong-typed domain values. The factory wraps them into
 * the strong types on the way out. This lets call sites use designated-
 * initializer brace elision -- .order_id{1} rather than
 * .order_id = rt::types::user_order_id{1} -- because copy-list-initialization of
 * std::optional<strong_type> from {value} would otherwise pick the optional's
 * explicit converting constructor (the strong type's underlying constructor
 * is explicit, which makes the optional's converting constructor explicit
 * too) and fail. Named strong-type constants like `alice` continue to bind
 * to `.user = alice` through the strong_type's implicit operator T&
 * conversion.
 */

namespace matching_engine::testing {

namespace rt = order_routing;

inline constexpr rt::types::user_id alice{1};
inline constexpr rt::types::user_id bob{2};
inline constexpr rt::types::user_id carol{3};
inline constexpr rt::types::user_id dave{4};

inline constexpr rt::types::symbol aapl{"AAPL"};

struct order_params
{
  std::optional<std::uint64_t> user{};
  std::optional<std::uint64_t> order_id{};
  std::optional<kraken::fixed_string<8>> instrument{};
  std::optional<rt::types::side> order_side{};
  std::optional<std::uint64_t> limit_price{};
  std::optional<std::uint64_t> quantity{};
};

inline matching_engine::order_state make_order_state(const order_params& params = {})
{
  return matching_engine::order_state{
    .user = rt::types::user_id{params.user.value_or(alice)},
    .order_id = rt::types::user_order_id{params.order_id.value_or(1)},
    .instrument = rt::types::symbol{params.instrument.value_or(aapl)},
    .order_side = params.order_side.value_or(rt::types::side::buy),
    .limit_price = rt::types::price{params.limit_price.value_or(10)},
    .remaining_quantity = rt::types::quantity{params.quantity.value_or(100)},
  };
}

inline rt::new_order make_new_order(const order_params& params = {})
{
  return rt::new_order{
    .user = rt::types::user_id{params.user.value_or(alice)},
    .order_id = rt::types::user_order_id{params.order_id.value_or(1)},
    .instrument = rt::types::symbol{params.instrument.value_or(aapl)},
    .order_side = params.order_side.value_or(rt::types::side::buy),
    .limit_price = rt::types::price{params.limit_price.value_or(10)},
    .order_quantity = rt::types::quantity{params.quantity.value_or(100)},
  };
}

} // namespace matching_engine::testing

#endif /* MATCHING_ENGINE_TEST_FACTORIES_HPP */
