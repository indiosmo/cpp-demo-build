# lab

The project's general-purpose utility library -- the "internal Boost"
that every other library is written against. It owns the small set of
compile-time and runtime primitives the rest of the code depends on,
and re-exports third-party types behind a `lab::` namespace.

The aliasing rule is the load-bearing convention: domain libraries
depend on `lab::` aliases, never on the upstream header directly.
Swapping a dependency out is a one-header change inside this
directory rather than a sweep across call sites. The current inventory
and the provenance of each upstream live in
[`DEPENDENCIES.md`](../../../DEPENDENCIES.md);
[ADR 0002](../../../docs/adr/0002-use-fetchcontent-for-third-party-dependencies.md)
records why dependencies use CMake `FetchContent`.

## Catalogue

| Library | Description |
|---|---|
| `result` / `error` | Structured error carrier over `boost::leaf`. Per-domain `error_code` enums plug into `std::error_code` via a single macro, and typed payloads are recovered in handlers via `data<T>()` / `is_type<T>()`. |
| `expected` | `std::expected` alias for boundary APIs that cross out of a domain. |
| `strong_type` | Nominal typedefs over `NamedType` with project conventions baked in: call-through, comparison, same-type arithmetic that preserves the strong type, hashing, streaming, and a `from(...)` factory for `fixed_string`-backed types. |
| `fixed_string` | Bounded inline string with a configurable truncation policy. Used for short identifiers on the hot path so message types stay allocation free. |
| `defaulted_field` | Transparent config-field wrapper carrying a compile-time default, with JSON field helpers that leave missing keys at the wrapped default. |
| `inplace_function` | Type-erased callable with fixed inline storage. Pipeline-stage `on_*` callbacks are typed as `inplace_function`, so emission and posting onto event loops never allocate and capture-size regressions surface as compile errors. |
| `variant` / `overload` | `match` over `std::variant` with compile-time exhaustiveness, and the `overload` set helper for visitors. |
| `algorithm` | `string_view` trimming and fixed-arity field splitting -- the small set of spellings the codebase reaches for repeatedly. |
| `charconv` | `from_chars` / `to_chars` returning `lab::result`, with exact and partial match modes and built-in support for `strong_type` wrappers. |
| `hash` | Reflective `auto_hash` for aggregates via `boost::pfr`, plus the `LAB_STD_HASH` macro that wires it into `std::hash`. |
| `json` | nlohmann/json adapter for strong types, fixed strings, optionals, field helpers, and `lab::result` parse/read helpers. |
| `concurrent_queue` | Single-producer / single-consumer lock-free queue, used on the two cross-thread edges of the pipeline. Aliased from `moodycamel::ReaderWriterQueue`; the blocking variant (`BlockingReaderWriterQueue`) backs the event-loop task queue. |
| `event_loop` | Pinned worker thread driving a `concurrent_queue` of work items, with a selectable idle strategy (`timed_wait_idle` or `busy_spin_idle`) and graceful shutdown. The runtime layer composes the three pipeline threads on top of it. |
| `log` | Small diagnostic logging facade. Defaults to stderr so the stdout sink can own the market-data stream; the destination is pluggable to a file or null logger at startup. |
| `assert` | Runtime assertion macro with a best-effort `boost::stacktrace` dump on failure. |
| `fmt` | Project-wide include for the vendored fmt library. Domain code includes this rather than the upstream headers directly. |
| [`network`](libs/network/) | UDP ingress sub-namespace. A Boost.Asio receiver for the default build and an `ef_vi` kernel-bypass stub showing the second-backend shape; the wiring shell picks one at composition time. |
