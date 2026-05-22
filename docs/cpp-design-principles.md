# C++ Design Principles

How this codebase approaches C++. Each principle is one line below;
the sections that follow expand each with a short rationale and a
representative example. Other documents in the project link to the
sections here when explaining a specific choice.

- **Functional core, imperative shell.** Domain code is synchronous
  over value types; sockets, threads, and stdout live at the edge.
- **Domain-owned vocabulary with explicit composition.** Each domain
  declares its own types and errors; a composition domain reaches
  across, peer domains do not.
- **Compile-time correctness.** Strong types, exhaustive variant and
  enum matching, designated initializers.
- **Error handling.** Fallible code returns `lab::result<T>`
  internally; public boundaries consume it and return
  `void` / `optional` / `expected`.
- **Declarative style.** Decompose, name predicates, prefer ranges
  and pattern matching over manual loops and `if/else` chains.
- **Performance discipline.** Clarity first; reach for hot-path swaps
  only when a measurement or an explicit invariant calls for one.
- **Behaviour-first tests.** Drive synchronous stages directly
  through `send` and capture emissions via their `on_*` callbacks.

---

## Functional core, imperative shell

Domain libraries are pure synchronous code: value types in, value
types out, no threads, no sockets, no timers. Effects -- network
IO, event loops, stdout, clocks -- live in a wiring shell that
composes the pure stages. Each stage runs on at most one thread by
construction; threading is a property of the shell.

The pay-off is that a domain contract reads as a straight sequence
of value-typed steps, without `asio` or `std::thread` plumbing
interleaved. Unit tests drive each stage without spinning a socket
or a loop.

## Domain-owned vocabulary with explicit composition

Each domain owns the declaration of its own types, requests,
messages, and errors. Peer domains stay independent. A *composition*
domain depends on the public vocabularies of the domains it
composes, rather than redeclaring parallel structs and paying for
conversions on the hot path.

For example: an inbound domain (`order_routing`) owns the request
vocabulary, an outbound domain (`market_data`) owns the message
vocabulary, and a composition domain in the middle (`matching_engine`)
consumes the former and emits the latter. The two peer domains do
not depend on each other.

## Compile-time correctness

Push validation into the compiler wherever the type system can
express it.

- Primitives that can be confused with each other become strong
  types -- `order_routing::user_id` and `user_order_id` are both
  backed by `std::uint64_t` but are distinct types. A function that
  takes them in the wrong order fails to compile.
- Closed sets of alternatives become `enum class` switches with no
  `default`, so the compiler enforces coverage under `-Werror`.
- Command and event families are `std::variant`s consumed with a
  variant-match helper. Adding a new alternative forces every
  meaningful visitor to make an explicit choice at compile time.
- Aggregates use designated initializers, so reordering or renaming
  a field cannot silently change call sites.

The pay-off is that whole classes of mistake never reach the test
suite. The compiler is the first reviewer.

## Error handling

The project alias `lab::result<T>` (wrapping
`boost::leaf::result<T>`) is the **internal** error-carrying
vocabulary. It travels between helpers inside a domain and lets a
handler at the boundary match on rich, structured failure payloads.
Errors live with the domain that produces them in
`<domain>/error_code.hpp` + `<domain>/errors.hpp`.

`lab::result<T>` never crosses a public boundary. At the boundary
of a domain or module, code calls `boost::leaf::try_handle_*` to
consume the result and returns one of:

- `void`, when the failure is handled in place -- logged, retried,
  or surfaced through a callback the caller wired.
- `std::optional<T>`, when "no value" is a sufficient summary and
  the caller does not need a reason.
- `lab::expected<T, E>`, when the caller needs to distinguish
  between different errors. `E` is the consuming domain's own
  error type, not a LEAF carrier.

Returning `lab::result<T>` from a public function forces every
caller into the LEAF machinery; the only exception is a utility
library whose contract *is* "this algorithm can fail" -- e.g. the
helpers in `lab/algorithm.hpp` -- where the algorithm itself
is the boundary.

Unrecoverable runtime failures (the SPSC queue's allocation path,
for instance) are not error-handling territory: they assert and
abort. `lab::event_loop::post` is `noexcept` and crashes on
queue-allocation failure rather than returning a result the caller
cannot meaningfully act on.

Inside the trusted interior, code works on typed values that the
parser boundary has already validated. There is no per-call
defensive ceremony for input the boundary has already rejected:
errors live at the boundary, typed values live in the interior.

## Declarative style

Decompose into simpler types, stage data upfront, give predicates
names, and prefer ranges and pattern matching over manual loops and
`if/else` chains.

Dispatch a command family by visiting its request variant with a
generic lambda that forwards to typed `handle(...)` overloads --
adding a new alternative without a matching overload fails to
compile inside the visitor. Stage the relevant booleans
(`had_trades`, `has_residual`, ...) before the branching so the
post-condition emission logic reads as a flat decision rather than
nested conditionals. Name `constexpr` predicates
(`is_market_order`, `resting_buy_crosses`) so the loop body reads
as a sequence of stages, not guards.

## Performance discipline

The default is the clearest implementation that meets the
requirement. Hot-path swaps appear only where an explicit invariant
or a benchmark calls for one. Anything deferred is recorded as an
`IMPROVEMENT:` comment at the call site.

Committed hot-path idioms:

- `lab::fixed_string<N>` for bounded text -- no per-value
  allocation.
- `lab::inplace_function` for stored callbacks -- callback
  storage inline in the owning stage, no `std::function` heap
  allocation.
- A shared `boost::pool` owned by the composition stage and used by
  every per-key sub-structure under it -- pool-allocated nodes
  never touch the general heap on the hot path.
- An intrusive list of pool-allocated nodes inside each bucket --
  pointer-stable, so an identity index can hold raw node pointers
  for direct cancellation.
- `boost::unordered_node_map` where reference stability across
  rehashes is load-bearing; `boost::unordered_flat_map` where pure
  point access is enough and cache behaviour matters more.
- Reserved capacity at startup for indexes sized from a config
  expectation.

## Behaviour-first tests

Tests assert observable contracts -- domain outputs, formatter
output, parser results -- not internal call sequences.

Because the domain stages are synchronous and hold no threading
state, Catch2 unit tests drive them by calling `send` directly and
capture emissions through `on_*` fields. Component tests exercise
each stage's contract without spinning a socket, an event loop, or
a thread. Workspace-level scenario tests feed the wired-up binary
end-to-end and confirm the assembled pipeline produces the expected
bytes.
