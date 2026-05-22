# Testing

Lab-side mapping for
[`cpp-guidelines/cpp-testing-principles/`](../cpp-guidelines/cpp-testing-principles/).
The shared guide owns rules and rationale. This file names the local test
layout, fixtures, result helpers, and integration shapes for
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

Targets stay module-scoped and use Catch2 with CTest discovery. Current anchors
are under [`test/`](../../test/).

Tag convention:

- module first: `[matching_engine]`, `[order_routing]`, `[market_data]`,
  `[lab]`;
- then test type or feature: `[unit]`, `[integration]`, `[codec]`,
  `[runtime]`, `[error]`, `[benchmark-support]`.

Tests that boot threads, sockets, or the future client/server pair are
integration tests and should live in their own files.

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

## Factories and fixtures

Production factories live beside the domain they construct. Current anchors:

- [`order_routing/factories.hpp`](../../src/order_routing/order_routing/factories.hpp)
- [`matching_engine/factories.hpp`](../../src/matching_engine/matching_engine/factories.hpp)

Test-only defaults currently live in
[`test/matching_engine/src/factories.hpp`](../../test/matching_engine/src/factories.hpp).
During the layout reframe, move reusable test factories toward
`<module>/testing/factories.hpp` when they are useful across several test
files; keep one-off helpers local.

## Callback wiring in tests

`lab::inplace_function` callback fields must be assigned before destruction.
The current project already has this shape on sessions, runtime stages, and
matching engines. Tests that ignore a callback should wire an explicit noop
rather than rely on default construction.

Add a `wire_noop_callbacks` helper when a component exposes several callbacks
or inherits callbacks from a base stage. Keep that helper in the test fixture
area for the owning module.

## Scenario and integration tests

The `test/` directory holds current CSV scenarios. The portfolio version should
keep useful scenarios as demonstration fixtures.

Target shape:

- scenario CSV inputs and expected market-data outputs under
  `test/integration/fixtures/` or `examples/scenarios/`;
- a client/server integration test that starts `matching_engine_lab_server`,
  sends commands through `matching_engine_lab_client`, and compares stdout;
- focused unit tests that still drive synchronous domain stages directly.

Integration tests should wait on observed output or process state, not fixed
sleep durations. Use predicate-based waits once the lab helper layer has them.

## Codec tests

`order_routing::csv_decoder` and `market_data::csv_encoder` are codec
boundaries. Their tests should stay table-driven and assert wire-visible
contracts. Current anchors:

- [`csv_decoder_test.cpp`](../../test/order_routing/src/csv_decoder_test.cpp)
- [`csv_encoder_test.cpp`](../../test/market_data/src/csv_encoder_test.cpp)

When the client library lands, add encoder tests for outbound order commands
there instead of testing the CLI process for every CSV shape.

## Benchmark support

Benchmarks are portfolio evidence only when their fixtures are readable and
repeatable. Keep microbenchmarks under `benchmarks/<module>/`, with scenario
builders named in domain vocabulary. Avoid generated benchmark bodies that are
hard to audit.

The current performance chart data can remain committed, but any new benchmark
story should document the workload and host assumptions in the performance
guide.
