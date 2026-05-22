# Principles Application Showcase

Where each principle from [`cpp-design-principles.md`](cpp-design-principles.md)
lands as concrete code. Principle headings link back to their
rationale; the body points at the files that show each idea in
practice.

## [Functional core, imperative shell](cpp-design-principles.md#functional-core-imperative-shell)

No domain library includes asio, `std::thread`, or `std::mutex`. All
I/O and threading sit in
[`server::application`](../src/server/src/application.cpp)
-- the receiver, the three loops, and the stdout sink are owned by
that class.

## [Domain-owned vocabulary with explicit composition](cpp-design-principles.md#domain-owned-vocabulary-with-explicit-composition)

Each domain owns its own `types.hpp` --
[`order_entry/types.hpp`](../src/order_entry/order_entry/types.hpp)
and
[`market_data/types.hpp`](../src/market_data/market_data/types.hpp)
declare distinct strong-type tags so values cannot cross-assign.
`matching_engine` is the composition point and depends on both
vocabularies directly; the peer domains stay independent.

## [Compile-time correctness -- strong types](cpp-design-principles.md#compile-time-correctness)

Designated-initializer construction at every emit site -- e.g. the
`market_data::trade` built inside
[`matching_engine::v3::engine::handle(new_order_single)`](../src/matching_engine/src/v3/engine.cpp)
-- compiles only if every named field receives the correct strong
type.

[`strong_type_test.cpp`](../test/lab/src/strong_type_test.cpp)
is the small Catch2 showcase for those contracts. Its `STATIC_REQUIRE`
checks pin equality, ordering, construction, operator return types, and
the absence of unsupported arithmetic at compile time, while ordinary
`CHECK` assertions cover runtime-only integrations such as hashing and
formatting.

## [Compile-time correctness -- exhaustive variants](cpp-design-principles.md#compile-time-correctness)

The CSV encoder visits `market_data::message` with one explicit
branch per alternative
([`csv_encoder.cpp`](../src/market_data/src/csv_encoder.cpp));
adding a new event alternative without an `encode_*` branch fails to
compile here. The engine's command dispatch in
[`engine.cpp`](../src/matching_engine/src/v3/engine.cpp)
(`engine::send` -> `lab::match` -> `handle(...)` overloads) enforces
the same property through overload resolution.

## [Error handling](cpp-design-principles.md#error-handling)

The result-aware startup chain in
[`application::start`](../src/server/src/application.cpp)
uses `LAB_LEAF_CHECK` for the socket bind and each loop's
`start()`. The matching engine's public `send` / `handle` overloads
return `void`: their `lab::result`-bearing helpers
(`handle_new_order_single_impl`, `find_book`, `check_duplicate`) collapse
failures into `boost::leaf::try_handle_all` at the public boundary,
matching the narrow-contract stance in
[`docs/engine-specs.md`](engine-specs.md).
Cross-thread posting goes through
[`lab::event_loop::post`](../src/lab/lab/event_loop.hpp),
which is `noexcept` and asserts the enqueue succeeded -- the only
failure mode is allocation failure, which is not recoverable at
this layer.

## Cross-cutting concerns

[`lab/log.hpp`](../src/lab/lab/log.hpp) is the
shared surface for one concern that cuts across runtime components:
diagnostic logging. The `LAB_LOG_*` macros capture source
locations, route through one configurable logger, and default to
stderr so stdout remains the market-data stream.

The runtime shell configures the logger once in
[`application::configure_logger`](../src/server/src/application.cpp).
Runtime components use the same facade for lifecycle, rejection, and
queue-overflow messages. The domain stages keep their own message
vocabularies focused on trading events, while the cross-cutting
concern stays behind a small `lab::` utility.

## [Declarative style](cpp-design-principles.md#declarative-style)

[`engine::handle(new_order_single)`](../src/matching_engine/src/v3/engine.cpp)
reads as a sequence of named stages -- duplicate check, ack, match,
residual, top-of-book -- with `removed_liquidity`, `is_market`, and
`should_place_order` staged before the branching. The named `crosses`
predicate in
[`matching.hpp`](../src/matching_engine/matching_engine/v3/matching.hpp)
factors out the market-versus-limit cross condition so the matching loop
reads as stages, not guards.

## [Performance discipline](cpp-design-principles.md#performance-discipline)

Four order-book variants live in the tree: the vector-backed v1, the
intrusive-list-plus-per-book-pool v2, and the v3 pair that lifts node
allocation out of the book itself (`v3::std_order_book` and the
production `v3::flat_order_book`, exposed through the top-level
[`matching_engine::order_book`](../src/matching_engine/matching_engine/order_book.hpp)
alias). A head-to-head
[benchmark](../benchmarks/order_book/src/matching_engine_order_book_benchmark.cpp)
parameterises place/cancel/traverse across all four shapes -- the
production choice is measured, not asserted.

## [Behaviour-first tests](cpp-design-principles.md#behaviour-first-tests)

[`matching_engine_test.cpp`](../test/matching_engine/src/matching_engine_test.cpp)
drives the engine through `send` and captures emissions via
`on_event` -- no sockets, no loops, no threads. The test bodies
assert the contract from [`engine-specs.md`](engine-specs.md), not the
implementation.

[`fixed_string_test.cpp`](../test/lab/src/fixed_string_test.cpp)
extends the same idea to the type-system contract:

- `static_assert` blocks pin constructibility, conversion, and
  trait/concept satisfaction at compile time -- e.g.
  `!std::is_constructible_v<fs_strict, std::string_view>` locks in
  the strict policy's refusal to accept unchecked input, and
  `lab::FixedString<fs_strict>` confirms the concept matches the
  template.
- A local `has_truncate_from` concept probes for the presence of a
  factory that only the strict policy exposes, so the
  policy-dependent surface is asserted both ways
  (`has_truncate_from<fs_strict>` and `!has_truncate_from<fs_auto>`).
- Catch2's `GENERATE(table<...>())` drives the runtime cases --
  labelled rows keep failures self-describing under `CAPTURE`, and
  the same table form covers success paths, overflow paths, and
  `remove_suffix` semantics without per-case duplication.
- A small `capture_error_code` helper adapts `lab::result` into a
  `std::error_code` so error paths can be compared directly against
  `lab::error_code::out_of_bounds` instead of unpacking leaf
  handlers per assertion.
