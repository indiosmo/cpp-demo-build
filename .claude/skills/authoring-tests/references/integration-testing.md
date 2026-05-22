# Integration Testing Heuristics

Use this reference after reading the shared testing philosophy:

- [`philosophy.md#component-vs-integration-tests`](../../../../docs/cpp-guidelines/cpp-testing-principles/philosophy.md)
- [`test-patterns.md`](../../../../docs/cpp-guidelines/cpp-testing-principles/test-patterns.md)
- [`docs/lab-guidelines/testing.md`](../../../../docs/lab-guidelines/testing.md)

## Lifecycle Tests

A lifecycle test follows one entity through a complete workflow, sharing the
arrange/act prefix across outcomes via `SECTION`. Reach for this shape when:

- the workflow crosses components;
- one order or session is threaded through several stages;
- multiple outcomes diverge from the same setup.

```cpp
TEST_CASE_METHOD(fixture, "order - lifecycle - crossing limit", "[matching_engine][integration]")
{
  auto order = ft::make_limit_buy();
  engine.send(order);
  REQUIRE(probe.acks().size() == 1);

  SECTION("rests residual")
  {
    engine.send(ft::make_partially_crossing_sell());
    CHECK(probe.top_of_book_events().size() == 1);
  }

  SECTION("fills completely")
  {
    engine.send(ft::make_fully_crossing_sell());
    CHECK(probe.trades().size() == 1);
  }
}
```

If two outcomes share less than roughly 80% of their setup, write separate
`TEST_CASE`s instead.

## What Belongs At Integration Level

Keep integration tests for:

- cross-component coordination;
- error paths only visible at a boundary;
- lifecycle outcomes involving several stages;
- triggered side effects across components;
- API smoke tests for realistic wiring.

Move down to component tests:

- pure matching semantics;
- CSV codec field-level behavior;
- formatter output for one message type;
- container and helper edge cases.

## Client/Server Scenarios

Phase 3 introduces a UDP client. Use end-to-end process tests sparingly:

- one smoke scenario for client -> server -> stdout;
- one failure-path scenario for client input/configuration;
- representative scenarios that demonstrate the portfolio workflow.

Do not move every order-book rule into a spawned-process test. The synchronous
domain tests should remain the main source of behavioral detail.
