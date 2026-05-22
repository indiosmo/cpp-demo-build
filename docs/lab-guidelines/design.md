# Design

Lab-side mapping for
[`cpp-guidelines/cpp-design-principles/`](../cpp-guidelines/cpp-design-principles/).
The shared guide owns rules and rationale. This file names only the project
headers, helper names, recurring choices, and non-obvious consequences that
matter for `matching-engine-lab`.

Symbol-to-header lookups live in the
[README placeholder table](README.md#placeholder-mapping).

## C++26 target

The portfolio version targets C++26. Prefer standard `std::expected`,
`std::print`, `std::stacktrace`, `std::ranges` algorithms, and contract-style
precondition notation when the toolchain supports them. Keep a compatibility
helper in `lab::` only when the feature is not yet available in the active
compiler.

Do not add a wrapper only to mirror another project. Add it when it gives this
repo a stable vocabulary, cleaner call sites, or a better migration path from
the current C++20 implementation.

## Component layout

The project layout keeps the boost-stuttering shape at module scope:

```text
src/<module>/
  <module>/                public headers, namespace also <module>
    types.hpp
    error_code.hpp
    errors.hpp
    runtime/               threaded or queued wrappers
  src/                     translation units
test/<module>/src/         Catch2 cases mirroring src/<module>/<module>/
benchmarks/<module>/src/   Google Benchmark targets
```

The current anchors are under `src/`.

## Utility layer

`lab` is the core utility layer. The core utility headers
are the matching-engine lab vocabulary layer: result handling, strong types,
bounded strings, variant matching, logging, assertions, hashing, event-loop
primitives, and network adapters.

When the helper layer expands, use the names in this guide where they improve
the surface:

- `LAB_ASSIGN` / `LAB_CHECK` instead of a project-specific LEAF macro name.
- `lab::make_error(...)` instead of `make_leaf_error(...)` at domain call
  sites.
- `lab::match_partial`, `lab::match_errors`, and container lookup helpers when
  they remove repeated error plumbing.

## Domain vocabulary

Keep the domain namespaces: `order_routing`, `matching_engine`, and
`market_data`. They express the pipeline, and they already give the portfolio
piece its technical shape.

Domain primitive aliases live in each domain's nested `types` namespace. The
canonical anchors are
[`order_routing/types.hpp`](../../src/order_routing/order_routing/types.hpp)
and
[`market_data/types.hpp`](../../src/market_data/market_data/types.hpp).
During the C++26 pass, keep bounded wire fields on `lab::fixed_string<N>` or a
standard bounded-string equivalent if one is available and fits.

## Error handling

The current project uses `lab::result<T>` internally and consumes it at
module boundaries. The target shape is result-oriented helper ergonomics with
lab names:

- domain helpers return `lab::result<T>`;
- domain boundaries return `void`, `std::optional<T>`, or `std::expected<T, E>`;
- structured errors live in `error_code.hpp` + `errors.hpp`;
- result pipelines unwrap with `LAB_ASSIGN` and `LAB_CHECK`;
- catch-all boundary handlers use `LAB_CATCH_AND_LOG`.

The first local candidate for container lookup helpers is
[`matching_engine::v3::engine`](../../src/matching_engine/src/v3/engine.cpp):
duplicate-order checks, book lookup, cancel lookup, and error mapping already
show the shape.

## Runtime layer split

Runtime modules wrap the synchronous core with event loops, queues, receivers,
and sinks. Current anchors:

- [`order_routing::runtime::session`](../../src/order_routing/order_routing/runtime/session.hpp)
- [`matching_engine::runtime::engine`](../../src/matching_engine/matching_engine/runtime/engine.hpp)
- [`market_data::runtime::publisher`](../../src/market_data/market_data/runtime/publisher.hpp)
- [`server::application`](../../src/server/server/application.hpp), the
  `server` wiring shell.

The client follows the same split: a synchronous `order_client` library for
command encoding and UDP sending, plus a thin CLI app for files and stdin.

## Pipelines and callback wiring

Pipeline stages use public `on_*` callback fields backed by
`lab::inplace_function`. This gives allocation-free callback storage, but it
has a non-obvious consequence: callback fields must be wired
before teardown in debug builds.

Tests should centralize no-op wiring helpers when a component exposes more
than one callback. Production code should wire every callback in the
application composition layer, not in the domain object constructor.

## Matching engine core

The portfolio value is concentrated in
[`matching_engine::v3`](../../src/matching_engine/matching_engine/v3/).
Plans should preserve the price-time semantics, identity index, pool-backed
intrusive order nodes, and top-of-book emission behavior unless the plan is
explicitly about changing the engine.

Use C++26 and lab helpers to make the core easier to read before
changing the algorithm. If a helper obscures matching vocabulary such as
`aggressor`, `maker`, `resting_price`, or `top_of_book`, keep the explicit
domain code.

## Hot-path helpers

The existing hot-path choices remain part of the demonstrator:

| Idea | Lab target |
|---|---|
| bounded text | `lab::fixed_string<N>` |
| stored callback | `lab::inplace_function<Sig, N>` |
| SPSC cross-thread edge | `lab::concurrent_queue<T>` |
| event-loop task | `lab::inplace_function<void(), N>` |
| stable order storage | pool-backed intrusive nodes |
| point lookup index | flat hash map with startup reservation |

New hot-path helpers need measurement or a clear invariant. The reframe is a
portfolio cleanup first, not a performance rewrite.

## Documentation anchors

`docs/lab-guidelines/` maps generic rules to local symbols. Long-form design
belongs in ADRs and focused docs such as
[`docs/engine-specs.md`](../engine-specs.md). Work-in-progress plans may cite
these guides, but durable docs should not link to `work-in-progress/`.
