# Testing

Lab-side mapping for
[`cpp-guidelines/cpp-testing-principles/`](../cpp-guidelines/cpp-testing-principles/).
The shared guide owns rules and rationale. This file names the local test
layout, test data, result helpers, and scenario-shaped tests for
`matching-engine-lab`.

Symbol-to-header lookups live in the
[README placeholder table](README.md#placeholder-mapping).

## Test target layout

Tests live under the root `test/` tree:

```text
test/<module>/
  CMakeLists.txt
  src/test_<feature>.cpp
```

Targets stay module-scoped and use Catch2 with CTest discovery. Anchors are
under [`test/`](../../test/).

Tag convention:

- module first: `[matching_engine]`, `[order_entry]`, `[order_client]`,
  `[market_data]`, `[mor]`, `[morfix]`, `[ospec]`, `[quickfix_fix]`,
  `[morfix_quickfix]`, `[mmd_json]`, `[lab]`;
- then test type or feature: `[unit]`, `[integration]`, `[codec]`,
  `[runtime]`, `[error]`.

Process-level client/server runs are local workflow checks rather than a CTest
surface in this repo.

## Result-aware assertions

Current tests use `LAB_REQUIRE_LEAF` and local helpers such as the
`capture_error_code` function in
[`fixed_string_test.cpp`](../../test/lab/src/fixed_string_test.cpp).
The target lab surface should provide:

- `LAB_REQUIRE_RESULT(target, expr)` for success-path unwraps in tests.
- `lab::testing::require_error<E>(callable)` when the payload type matters.
- `lab::testing::capture_error_code(callable)` when the code is the asserted
  contract.

Add these helpers before broadening error-path tests; otherwise each test file
will grow its own local LEAF handling.

## Factories and test data

Production factories live beside the domain they construct. Anchors:

- [`order_entry/factories.hpp`](../../src/order_entry/order_entry/factories.hpp)
- [`matching_engine/factories.hpp`](../../src/matching_engine/matching_engine/factories.hpp)

Test-only defaults live in
[`test/matching_engine/src/factories.hpp`](../../test/matching_engine/src/factories.hpp).
Move reusable test factories toward `<module>/testing/factories.hpp` when they
are useful across several test files; keep one-off helpers local.

## Callback wiring in tests

`lab::inplace_function` callback fields must be assigned before destruction.
The current project already has this shape on sessions, runtime stages, and
matching engines. Tests that ignore a callback should wire an explicit noop
rather than rely on default construction.

Add a `wire_noop_callbacks` helper when a component exposes several callbacks
or inherits callbacks from a base stage. Keep that helper in the test fixture
area for the owning module.

## Scenario-shaped tests

Keep focused unit tests driving synchronous domain stages directly. Broad
scenario-shaped coverage belongs in module tests with typed values and local
helpers, not wire fixture directories.

## Codec tests

`order_entry::json_decoder` and `market_data::json_encoder` are codec
boundaries. Their tests should stay table-driven and assert wire-visible
contracts. Anchors:

- [`json_decoder_test.cpp`](../../test/order_entry/src/json_decoder_test.cpp)
- [`json_encoder_test.cpp`](../../test/market_data/src/json_encoder_test.cpp)

The `order_client` encoder tests cover outbound order commands. The CLI process
is kept thin; exercise command shapes through the library and parser tests.

The codec scaffold adds focused compile-and-contract tests:

- `mor` tests cover source/sink wiring and compatibility conversions.
- `morfix` tests cover bidirectional FIX-shaped conversion and lifecycle
  correlation.
- `ospec` tests cover B3 tag anchors and bidirectional value normalization.
- `quickfix_fix` tests cover local FIX message replacement, text
  encode/decode, and in-memory session delivery.
- `morfix_quickfix` tests cover B3 request/event encoding, decoding,
  unsupported-message failures, and local initiator/acceptor loop delivery.
- `mmd_json` tests prove normalized market-data events preserve the current
  JSON record shape.

## Performance Support

When the benchmark suite returns, keep workloads readable and repeatable, name
scenario builders in domain vocabulary, and document host assumptions beside
the benchmark entry points.
