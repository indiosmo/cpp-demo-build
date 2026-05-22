# Lab Agent Examples

Project-specific good/bad pairs that complement
[`cpp-guidelines/agent/cpp-agent-examples.md`](../cpp-guidelines/agent/cpp-agent-examples.md).
The shared file covers generic `lib::` placeholders; this file covers
`matching-engine-lab` shapes that only show up once those placeholders map to
the lab vocabulary in [`README.md`](README.md#placeholder-mapping).

Slice this file by heading. Load only the section relevant to the edit.

## Result Unwrap

Domain code should unwrap fallible helpers with the lab result macros once they
land. Until the rename, the current code still uses the existing LEAF wrappers.

Good -- bind the success value and keep the failure trail intact:

```cpp
LAB_ASSIGN(auto* book, find_book(request.instrument));
LAB_CHECK(check_duplicate(order_key{.user = request.user, .order_id = request.order_id}));
```

Good -- attach local context where the failing helper cannot see it:

```cpp
LAB_ASSIGN(
  auto* node,
  find_resting(request),
  matching_engine::errors::unknown_order{
    .user = request.user,
    .order_id = request.order_id,
  });
```

Bad -- `.value()` throws away the structured result path:

```cpp
auto* book = find_book(request.instrument).value();
```

Bad -- raw pointer-like access on a result is an unchecked unwrap:

```cpp
find_book(request.instrument)->place(node);
```

## Strong Types

Keep arithmetic and comparisons in the strong type when the operation is
defined there.

Good -- domain quantities stay typed:

```cpp
auto total_quantity = types::quantity{0};
for (const auto& order : level.orders) {
  total_quantity += order.remaining_quantity;
}
```

Bad -- unwraps early, then rewraps after the loop:

```cpp
std::uint64_t total_quantity = 0;
for (const auto& order : level.orders) {
  total_quantity += order.remaining_quantity.get();
}
return types::quantity{total_quantity};
```

Good -- format strong values directly when a formatter exists:

```cpp
LAB_LOG_WARN("cancel miss: user={} order_id={}", request.user, request.order_id);
```

Bad -- unwraps only to satisfy a formatter that already accepts the wrapper:

```cpp
LAB_LOG_WARN("cancel miss: user={} order_id={}", request.user.get(), request.order_id.get());
```

## Callback Wiring

Pipeline stages expose `on_*` callback fields. Tests that ignore a callback
still wire it to a noop before the object is destroyed.

Good -- local fixture wires every callback once:

```cpp
void wire_noop_callbacks(order_entry::session& session)
{
  session.on_request = [](const order_entry::request&) {};
  session.on_rejected = [](const order_entry::rejection&) {};
}
```

Bad -- a test leaves an ignored callback unassigned:

```cpp
order_entry::session session;
session.on_request = [&](const order_entry::request& request) {
  captured.push_back(request);
};
```

## Logging And Stdout

Stdout is the market-data stream. Diagnostics go through the logger.

Good -- diagnostic output uses the logger:

```cpp
LAB_LOG_WARN("rejecting new_order_single: unknown symbol {}", request.instrument);
```

Bad -- diagnostic output pollutes the record stream:

```cpp
std::cout << "rejecting new_order_single: unknown symbol " << request.instrument << '\n';
```

## Scenario Tests

Scenario-shaped examples are demonstration assets. Keep detailed behavior in
synchronous unit tests.

Good -- unit test drives the domain directly:

```cpp
matching_engine::engine engine{make_engine_config()};
engine.on_event = [&](const market_data::message& message) {
  emitted.push_back(message);
};

engine.send(make_limit_buy());

REQUIRE(emitted == expected_messages);
```

Bad -- every matching rule is tested only through a spawned server process:

```cpp
run_server();
run_client("crossing-order.jsonl");
CHECK(read_stdout() == expected_jsonl);
```

## Documentation Comments

Comments describe the current code. They do not narrate the refactor.

Good -- present-tense invariant:

```cpp
// ack precedes trades so downstream consumers see the order id before fills.
emit_ack(request);
```

Bad -- history belongs in git:

```cpp
// ack emission used to happen downstream, but it was moved here.
emit_ack(request);
```
