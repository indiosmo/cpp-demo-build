#ifndef MATCHING_ENGINE_TEST_FACTORIES_HPP
#define MATCHING_ENGINE_TEST_FACTORIES_HPP

#include "matching_engine/order_state.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include "lab/fixed_string.hpp"

#include <cstdint>
#include <optional>

/*
 * Shared test helpers for the matching_engine component tests.
 *
 * The two factories produce the value types that bracket the matching
 * engine's surface: matching_engine::order_state for the resting-order tests
 * against the order book, and order_entry::new_order_single for the engine-level
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
 * initializer brace elision -- .cl_ord_id{1} rather than
 * .cl_ord_id = rt::types::cl_ord_id{1} -- because copy-list-initialization of
 * std::optional<strong_type> from {value} would otherwise pick the optional's
 * explicit converting constructor (the strong type's underlying constructor
 * is explicit, which makes the optional's converting constructor explicit
 * too) and fail. Named strong-type constants like `alice` continue to bind
 * to `.client_id = alice` through the strong_type's implicit operator T&
 * conversion.
 */

namespace matching_engine::testing {

namespace rt = order_entry;

inline constexpr rt::types::client_id alice{1};
inline constexpr rt::types::client_id bob{2};
inline constexpr rt::types::client_id carol{3};
inline constexpr rt::types::client_id dave{4};

inline constexpr rt::types::symbol aapl{"AAPL"};
inline constexpr rt::types::security_id aapl_security_id{1};
inline constexpr rt::types::security_exchange bvmf{"BVMF"};

struct order_params
{
  std::optional<std::uint64_t> client_id{};
  std::optional<std::uint64_t> cl_ord_id{};
  std::optional<lab::fixed_string<16>> symbol{};
  std::optional<rt::types::side> side{};
  std::optional<std::uint64_t> price{};
  std::optional<std::uint64_t> quantity{};
};

inline matching_engine::order_state make_order_state(const order_params& params = {})
{
  return matching_engine::order_state{
    .client_id = rt::types::client_id{params.client_id.value_or(alice)},
    .cl_ord_id = rt::types::cl_ord_id{params.cl_ord_id.value_or(1)},
    .security_id = aapl_security_id,
    .symbol = rt::types::symbol{params.symbol.value_or(aapl.get())},
    .security_exchange = bvmf,
    .side = params.side.value_or(rt::types::side::buy),
    .ord_type = rt::types::ord_type::limit,
    .time_in_force = rt::types::time_in_force::day,
    .price = rt::types::price{params.price.value_or(10)},
    .order_qty = rt::types::quantity{params.quantity.value_or(100)},
    .leaves_qty = rt::types::quantity{params.quantity.value_or(100)},
  };
}

inline rt::new_order_single make_new_order_single(const order_params& params = {})
{
  return rt::new_order_single{
    .client_id = rt::types::client_id{params.client_id.value_or(alice)},
    .cl_ord_id = rt::types::cl_ord_id{params.cl_ord_id.value_or(1)},
    .security_id = aapl_security_id,
    .symbol = rt::types::symbol{params.symbol.value_or(aapl.get())},
    .security_exchange = bvmf,
    .side = params.side.value_or(rt::types::side::buy),
    .ord_type = rt::types::ord_type::limit,
    .time_in_force = rt::types::time_in_force::day,
    .order_qty = rt::types::quantity{params.quantity.value_or(100)},
    .price = rt::types::price{params.price.value_or(10)},
  };
}

} // namespace matching_engine::testing

#endif /* MATCHING_ENGINE_TEST_FACTORIES_HPP */
