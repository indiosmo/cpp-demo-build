# Highlights

## Fast

Three named threads on a busy-spin idle, one per pipeline stage.
Bounded, preallocated containers throughout, so the hot path stays
out of the allocator.

The matching core keeps cancellation, matching, and book-update paths
direct and allocation-aware. The benchmark suite is intentionally absent
while the reframe is in flight and will be rebuilt as a separate piece.

## Resilient

- The project builds and tests under GCC and Clang.
- Address + Leak + Undefined and Thread sanitisers each get their
  own build preset and run green.
- The build is clean under `-Werror` against a curated warning set
  well beyond `-Wall -Wextra`. See
  [`cmake/CompilerFlags.cmake`](../cmake/CompilerFlags.cmake) for the
  list.
- Per-library Catch2 unit tests exercise each domain in isolation.
  The client and server binaries can be run together over localhost UDP.

## Modular

Every pipeline stage sits behind a typed interface and plugs in
through a config field. The project shows the seam at two scales:

- **Inside a stage.** The UDP receiver has two implementations in
  tree -- one over Boost.Asio, one over Solarflare's kernel-bypass
  API -- selected by config.
- **Across a stage boundary.** The matching engine separates inbound
  order-entry requests, outbound order-entry lifecycle events, and
  outbound market-data events through typed callbacks. The runtime
  shell wires those callbacks without changing the synchronous core.

The decoder, encoder, publisher, and sink expose the same shape.

## Safe by design

The codebase leans on the type system to move whole classes of bug
to compile time:

- Strong types make every primitive identifier distinct.
- Exhaustive `enum class` switches force every site to handle every
  variant.
- Structured errors propagate through `lab::result<T>` and
  collapse at the boundaries.
- Hot-path containers are bounded by construction.
- The domain libraries are pure synchronous code over value types --
  threads, sockets, and stdout live in the wiring shell.

See [`cpp-design-principles.md`](cpp-design-principles.md) for the
full set and [`showcase.md`](showcase.md) for each principle mapped
onto concrete code.

## Configurable

Threading topology, idle strategy per loop, UDP backend, log sink,
and per-stage tuning are all config fields. The shipped `main`
builds the config programmatically; the same shape loads from a
file when a deployment needs that. Same binary, different config: a
co-located host can busy-spin three cores for the lowest reaction
latency while a shared host yields to the scheduler.
