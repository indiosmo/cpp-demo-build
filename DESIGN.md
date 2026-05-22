# Design

This document explains the major design choices behind the order book
submission. The architectural narrative -- pipeline shape, threading
model, component responsibilities -- lives in the
[top-level README](README.md) and in the per-library READMEs under
[`submission/src/`](submission/src/); this document is here to
explain *why* the project made the choices it made.

## How decisions are recorded

Decisions live in three places:

- **Architecturally relevant or broad-impact decisions** live as
  Architecture Decision Records under [`docs/adr/`](docs/adr/),
  numbered and dated. Each ADR captures the context, the options
  considered, the decision drivers, and the chosen outcome with its
  trade-offs. Every ADR has an accompanying decision-matrix HTML
  artefact next to it that scores the options against the drivers.
- **Localized decisions with non-obvious trade-offs** live as
  `/* ... */` comments next to the code they affect. The engine-owned
  shared `boost::pool` for resting orders and the
  `unordered_flat_map`-versus-`unordered_node_map` choice for the
  cross-symbol identity index, for instance, are both explained
  inline in
  [`submission/src/matching_engine/matching_engine/v3/engine.hpp`](submission/src/matching_engine/matching_engine/v3/engine.hpp);
  the intrusive-list-plus-pool layout inside a price level is
  explained at the top of
  [`submission/src/matching_engine/matching_engine/v3/order_book.hpp`](submission/src/matching_engine/matching_engine/v3/order_book.hpp).
- **Everything else** -- the recurring conventions, the strong-typing
  habit, the result-type vocabulary, the runtime-layer split -- is
  the project's C++ design principles, summarised in
  [`docs/cpp-design-principles.md`](docs/cpp-design-principles.md).

The decisions on this page were made by applying those principles
inside the constraints and assumptions below.

## Constraints, assumptions, trade-offs

The exercise is a closed system: well-defined CSV inputs over UDP on a
fixed port, well-defined CSV outputs on stdout, sixteen scenarios that
exercise the matching contract. Those constraints shaped the design
as much as any individual decision below.

- **Inputs are known to be well-formed.** Hidden tests reuse the same
  shapes as the provided scenarios. The decoder treats the documented
  grammar as a precondition, not as something to defend against, and
  the matching engine trusts the typed commands it receives.
- **Inputs are static and finite.** No load test, no live exchange
  feed, no need to plan for transport changes. The wiring layer can
  hard-code the three-thread topology rather than abstracting it.
- **The book is the core of the exercise.** Strong typing, error
  handling, and runtime composition are scaffolding; the matching
  loop and the per-symbol book are what an evaluator most wants to
  read. The submission deliberately spends complexity budget there.
- **The submission ships once.** No future maintainers, no roadmap,
  no migration story. Extensibility seams (transport interface,
  alternate sinks, queue topology) stay where they cost nothing to
  keep and get collapsed where they would just be ceremony.
- **There is a delivery deadline.** When a stock or vendored helper
  short-cuts an implementation, the submission takes the helper even
  at a small performance cost and flags the spot for a perf-driven
  follow-up. The guard rail is correctness and clarity; the simpler
  approach wins ties, and anything that does not pay back inside the
  submission window is deferred to the Improvements section.

One expectation pulls in the opposite direction:

- **The submission still aims to demonstrate production-grade
  judgement.** Code organisation, documentation, testing, error
  handling, build hygiene, and dependency management appear the way
  they would in a real codebase. The aggregated
  `src/`/`test/`/`benchmarks/` layout, the per-domain Catch2 unit
  tests plus the workspace-level black-box scenarios, the ADR trail,
  the `kraken::` alias rule, the strong-typed domain vocabularies,
  and the functional-core / imperative-shell split are not there to
  pass the sixteen scenarios; the scenarios would pass with a single
  `main.cpp`. They exist to make the submission legible as a piece
  of production-shaped engineering.

The two pressures meet in the middle: the codebase looks like
production code at the structural level and acts like a one-shot
submission at the leaf level. Hot-path complexity that would only
matter under sustained traffic is deferred; the seams that let it
slot in later -- transport backend, queue topology, book internals --
stay visible.

### Narrow contracts at the boundaries

One direct consequence is worth naming on its own, because it shapes
the parser, the matching engine's invariant checks, and the absence
of a rejection path from end to end: the submission defines **narrow
contracts** at the input boundaries and **streamlines error
handling** to match.

A production trading system never trusts the wire. It validates
every field of every datagram, treats malformed input as routine,
and turns hostile input into a structured rejection (or a metric, or
a circuit breaker trip) without ever destabilising the matching
loop. That discipline costs real plumbing: per-field result wrapping,
a rejection event on the wire, an `on_rejected` path the publisher
actually exercises, fuzz tests, and an answer for every "what if
this byte is wrong" question.

For this submission the contract is the inverse. `EXERCISE.md` tells
us the hidden tests use the same well-formed CSV shapes as the
provided scenarios. Under those conditions:

- The CSV decoder treats field count, marker validity, numeric
  parseability, side tokens, and symbol length as **preconditions**
  -- asserted with `KRAKEN_ASSERT` where the check is local and
  documented in a comment at the call site where adding the assertion
  would distort a one-line conversion.
- The `decode(...)` boundary still returns `kraken::result<command>`
  so the seat for a future non-CSV decoder with richer errors stays
  intact; no path inside the CSV implementation reaches it.
- The matching engine trusts the typed commands it receives. Its
  internal invariants are checked in debug builds and trusted under
  `NDEBUG`; there is no per-call validation of values the parser has
  already proven correct by construction.
- The session's `on_rejected` callback exists -- the rejection
  surface lives at the right layer architecturally -- but no
  production path triggers it. The wiring lambda logs.
- Startup is the one runtime boundary that surfaces recoverable
  failures (binding the socket, starting the threads). Internally
  the wiring composes `kraken::result` chains; the result is
  consumed at `main` via `boost::leaf::try_handle_*` before any
  domain caller sees it. Cross-loop posting is non-blocking and
  `noexcept` -- the SPSC queue's only failure mode is allocation
  failure, which aborts.

The win is less plumbing, fewer indirection layers, and a parser
whose code structure matches the exercise grammar one-to-one against
`EXERCISE.md`. The cost is that a non-conforming input will fault in
debug or invoke undefined behaviour under `NDEBUG` instead of
producing a clean rejection. For a real deployment the trade is
wrong; for this submission, where the input shape is contractual and
finite, it is the right side of the ergonomics-versus-defensiveness
axis. The Improvements section records the production-shaped
alternative as a follow-up.

## Principles application showcase

Where each principle from
[`docs/cpp-design-principles.md`](docs/cpp-design-principles.md)
lands as concrete code lives in [`docs/showcase.md`](docs/showcase.md).

## Architectural decisions

The four ADRs below capture the decisions whose blast radius reaches
beyond a single file. Each links to the full record and its
companion decision matrix.

- **[ADR 0001 -- C++ project layout.](docs/adr/0001-cpp-project-layout.md)**
  Aggregated `src/` / `test/` / `benchmarks/` trees under
  `submission/`, stuttering `<lib>/<lib>/` headers, library plus thin
  executable. Picked over module-colocated tests because it gives a
  single declaration site for the Catch2 and Google Benchmark
  dependencies and lets tooling target each tree at one stable path.
- **[ADR 0002 -- Copy-vendor third-party utilities.](docs/adr/0002-copy-vendor-third-party-utilities.md)**
  Upstream headers land under `submission/vendor/` with a pinned SHA
  and the upstream LICENSE. Picked over git submodules, CMake
  `FetchContent`, and `ExternalProject` because the grading
  `docker build` must not depend on a configure-time network fetch
  and the `git bundle` delivery format does not carry submodule
  contents.
- **[ADR 0003 -- Parse CSV as fixed-shape commands.](docs/adr/0003-parse-csv-as-fixed-shape-commands.md)**
  Byte zero selects the command type, the message-specific decoder
  splits the known field count, and numeric fields go through
  `kraken::from_chars`. Picked over a general tokenizer or a parser
  library because the exercise grammar is closed; field validity is
  treated as a documented precondition rather than threaded through
  `kraken::result`.
- **[ADR 0004 -- Stage the order-book data structure by iteration.](docs/adr/0004-keep-order-book-as-sorted-price-level-maps.md)**
  Sorted price-level maps for the first iteration to pass the suite
  with the smallest implementation that gives the right semantics;
  intrusive lists plus an object pool for the optimisation
  iteration. The book in the current `kraken_submission` binary is
  the optimisation-iteration shape.

## Localized decisions

A handful of in-code decisions are worth calling out from this page
because the reasoning is non-obvious. In each case the comment in the
referenced header carries the full rationale; the bullets below are
the headlines.

- **Engine-owned shared `boost::pool` for resting-order storage.** A
  single pool is owned by the matching engine and shared across every
  per-symbol book, rather than each book owning its own. Symbol
  activity is uneven, so a shared pool absorbs that variance for free
  -- slots freed by a shallow symbol are immediately available to a
  deep one. See the rationale comment above the `node_pool_` field in
  [`submission/src/matching_engine/matching_engine/v3/engine.hpp`](submission/src/matching_engine/matching_engine/v3/engine.hpp).
- **`unordered_flat_map` for the cross-symbol identity index.** The
  index is used only for point access (insert, find, erase by key
  immediately after the find), so the iterator-and-reference
  stability `unordered_node_map` provides across rehashes is not
  load-bearing for any caller. With that constraint dropped, the flat
  variant gives better lookup-per-cache-line behaviour, with capacity
  reserved at startup so a properly sized deployment never rehashes
  after startup. See the comment above `resting_index_` in the same
  file.
- **Intrusive list plus pool layout inside a price level.** Each
  resting order sits in a pool-allocated node hooked into the price
  level's intrusive list. Nodes never relocate, so the engine's
  cross-symbol identity index can hold raw `order_node*` pointers
  with stable lifetime for the resting order. The header comment in
  [`submission/src/matching_engine/matching_engine/v3/order_book.hpp`](submission/src/matching_engine/matching_engine/v3/order_book.hpp)
  walks through the placement and cancel paths.
- **SPSC blocking queue coupled to the event loop.** Each thread runs
  a `kraken::event_loop` backed by a single-producer / single-consumer
  blocking queue. The three-thread topology has exactly two
  cross-thread edges, each with one producer and one consumer; MPSC
  would pay for synchronisation the topology never exercises. The
  loop is intentionally intertwined with its primary queue. Idle
  behaviour is selectable per loop: `kraken::timed_wait_idle` parks
  on the queue via `wait_dequeue_timed`, `kraken::busy_spin_idle`
  spins. The submission defaults all three loops to `busy_spin_idle`
  -- HFT is the use case the deliverable is graded against; a more
  general multi-queue runtime is left for the Improvements section.
- **spdlog stdout sink, single-threaded variant.** The output
  pipeline ends in a `market_data::spdlog_sink` that wraps
  `spdlog::sinks::stdout_sink_st`. The single-threaded sink skips the
  per-write mutex its `_mt` sibling takes; the runtime layout pins this
  sink to the dedicated publisher thread, so the mutex would only ever
  be uncontended overhead. The fmt ABI that lets the submission take
  spdlog from the system package is recorded in
  [`DEPENDENCIES.md`](DEPENDENCIES.md).

## Improvements

- **Kernel-bypass UDP receiver.** Replace `asio_udp_receiver` with a
  vendor-specific stack (Solarflare ef_vi) or a generic
  kernel-bypass framework (DPDK). The `ef_vi_udp_receiver` stub
  already marks the seat; the wiring shell picks backends at
  composition time.
- **Telemetry and observability.** Per-stage counters, latency
  histograms, and structured logs for the few operational metrics
  that matter (datagrams received, commands accepted, trades
  emitted, queue depth).
- **Benchmarks and measurements.** Using google bench for statistical
  latency measurements is limited as we can't get a full per-request
  distribution. Add a dedicated harness and tooling for computing
  histograms and proper percentiles including tail latencies.
- **Full error handling at the input boundary.** The current narrow
  contracts treat malformed input as undefined behaviour; a
  production deployment would propagate per-field errors through
  `kraken::result`, drive `on_rejected` from the parser, and add
  fuzz tests.
- **Generalised runtime queue topology.** Decouple event loops from a
  single blocking queue, keep pairwise SPSC queues for
  latency-sensitive paths, add MPSC queues for lower-priority fan-in
  work, and switch event-loop polling to non-blocking dequeue across
  multiple inbound queues before sleeping.
- **Further iteration on the core matching algorithm.** Refine the
  API and the internal design to make it easier to reason about.
- **Better control of allocations.** Configurable toggles for reserving
  containers all throughout the component tree, to avoid allocations
  in the hot path.
- **Compile-time side dispatch in matching engine handlers.** The
  runtime side-keyed accessors on `flat_order_book`
  (`best(side)`, `best_level(side)`) and the inline 2-arm switch
  around the match call in `handle(new_order)` could be replaced
  with templated handlers (`handle_new_order_impl<TakerSide>`,
  `handle_cancel_impl<CancelSide>`) and templated book accessors
  (`best<S>()`, `side_map<S>()`), with a single runtime dispatch
  at the public handler entry. Every internal side branch
  collapses into one branch per request. The flatter
  non-templated form was chosen for readability and to compose
  cleanly with the `kraken::result` + leaf decomposition in the
  handlers; the templated form is the natural next step if
  per-branch latency becomes measurable.
- **Other candidates.** A lock-free MPSC queue if the input side ever
  fans out across sockets, NUMA-aware pinning, persisted snapshots
  for crash recovery, property-based tests for matching invariants.
