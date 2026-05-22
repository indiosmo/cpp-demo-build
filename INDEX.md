# INDEX

## System overview

Multi-threaded UDP-driven order book matching engine. The `server` binary ingests CSV order-entry commands over UDP, matches them against per-symbol books, and publishes the resulting market-data records to stdout. The `client` binary sends typed order-entry commands from files or stdin to the server. The codebase is organised as static libraries for shared vocabulary, order entry, order clients, market data, matching, and runtime wiring, plus Catch2 unit tests and Google Benchmark microbenchmarks.

## Tech stack

- C++26
- CMake (presets: debug, release, asan, tsan, clang)
- Ninja generator
- Boost (header-only) -- https://www.boost.org -- LEAF, container_hash, pfr, algorithm, stacktrace, asio, intrusive, pool, unordered
- fmt 9.1 (`libfmt-dev`, header-only) -- https://github.com/fmtlib/fmt
- spdlog (`libspdlog-dev`) -- https://github.com/gabime/spdlog
- Catch2 (tests) -- https://github.com/catchorg/Catch2
- Google Benchmark (microbenchmarks) -- https://github.com/google/benchmark
- NamedType (vendored, MIT) -- https://github.com/joboccara/NamedType
- SG14 inplace_function (vendored, BSL-1.0) -- https://github.com/WG21-SG14/SG14
- moodycamel ReaderWriterQueue (vendored, BSD-2-Clause) -- https://github.com/cameron314/readerwriterqueue

## Directory structure

```
matching-engine-lab/
|-- cmake/                                shared CMake modules (compiler flags, sanitizers)
|-- docs/                                 ADRs, C++ design principles, engine spec, runtime docs, performance data
|   |-- adr/                              architecture decision records (numbered, dated)
|   `-- performance/                      benchmark results and raw data
|-- scripts/                              developer scripts (formatting, pre-commit, vendor sync)
|-- src/                                  library sources and client/server executables
|   |-- lab/                              general-purpose vocabulary helpers
|   |   |-- lab/                          public headers
|   |   |-- libs/                         network adapter sub-library (asio + ef_vi UDP receivers)
|   |   `-- src/                          reserved for utility translation units
|   |-- client/                          CLI sender; the client executable target
|   |-- market_data/                      outbound domain: typed events to CSV records to stdout
|   |-- matching_engine/                  composition domain: per-symbol books and the production matching loop
|   |-- order_client/                     typed client API: order-entry requests to UDP CSV datagrams
|   |-- order_entry/                     inbound domain: UDP CSV bytes to typed order-entry requests
|   `-- server/                           wiring shell + main; the server executable target
|-- test/                                 module Catch2 tests
|-- benchmarks/                           Google Benchmark microbenchmarks
|   |-- lab/                              vocabulary-layer benchmarks
|   |-- matching_engine/                  engine throughput / latency benchmarks
|   `-- order_book/                       per-shape order-book microbenchmarks
|-- vendor/                               copy-vendored third-party utilities
`-- work-in-progress/                     working notes not yet promoted to docs/
```

## Modules

### lab

Base path: `src/lab/lab/`

The project's general-purpose vocabulary layer -- the "internal Boost" the rest of the code is written against. It owns a small set of in-house primitives (strong types, fixed-size strings, an event loop, a structured error carrier, match-over-variant) and re-exports every copy-vendored third-party header behind a `lab::` alias so a vendor swap is a single-header change. A `network` sub-namespace adds two UDP-receiver adapters (Boost.Asio and an `ef_vi` stub) plus their shared value types.

Headers:

- (P) algorithm.hpp        -- `string_view` `trim` / `ltrim` / `rtrim` plus a fixed-arity `split_fields<N>` that returns `array<string_view, N>`
- (P) assert.hpp           -- `LAB_ASSERT(expr)` macro: prints expression, location, and a `boost::stacktrace` dump, then aborts
- (P) charconv.hpp         -- `from_chars<T>` returning `lab::result`, with exact / partial modes and overloads for strong types and fixed strings
- (I) concurrent_queue.hpp -- Aliases `moodycamel::ReaderWriterQueue` (and its blocking variant) as the project's SPSC lock-free queues
- (P) error.hpp            -- Type-erased `lab::error` value carrying any payload with `error_code()` / `what()`, plus leaf predicate adapters and `make_leaf_error`
- (P) error_code.hpp       -- `lab::error_code` enum (generic, invalid_argument, out_of_bounds, ...) wired into `std::error_code` via the category macro
- (P) error_macros.hpp     -- `LAB_DEFINE_ERROR_CATEGORY(NS)`: turns any domain's `error_code` enum into a usable `std::error_code` domain
- (I) errors.hpp           -- `lab::errors::generic_error` catch-all payload: an `error_code` plus optional free-form text
- (P) event_loop.hpp       -- Pinned `jthread` running a blocking SPSC task queue plus pollers, with timed-wait or busy-spin idle strategies
- (P) expected.hpp         -- Aliases `std::expected` as `lab::expected` for boundary value-or-error APIs
- (P) fixed_string.hpp     -- Bounded inline string templated on capacity and a strict / auto-truncate policy, with hash, fmt, and ostream support
- (P) fmt.hpp              -- Project-wide aggregate include of the upstream fmt headers behind one entry point
- (P) hash.hpp             -- `auto_hash<T>` reflective hasher via `boost::pfr`, plus the `LAB_STD_HASH` and `LAB_HASH_VALUE` macros
- (P) inplace_function.hpp -- Alias of SG14 `inplace_function` with capacity as a template parameter, so capture-size regressions are compile errors
- (P) log.hpp              -- Diagnostic logger facade with console / null / file backends and `LAB_LOG_*` macros that capture the call site
- (I) overload.hpp         -- Canonical `overload<Ts...>` aggregate plus its deduction guide for building visitor sets from lambdas
- (P) result.hpp           -- Aliases `boost::leaf::result` as `lab::result` and supplies the project's leaf scaffolding (assign / check / catch-all / exit-on-error macros)
- (P) strong_type.hpp      -- NamedType-backed nominal typedef with call-through, comparison, same-type arithmetic, hashing, fmt, and a fixed-string-aware factory
- (P) variant.hpp          -- `lab::match` recursive-index dispatcher over `std::variant`, plus shape helpers (`extend_variant`, `concat_variants_t`, ...) and `LAB_PLUCK`
- (P) network/asio_udp_receiver.hpp  -- Boost.Asio UDP receiver: parses the endpoint, binds, and drives `on_datagram` from `async_receive_from`
- (P) network/ef_vi_udp_receiver.hpp -- Stub Solarflare `ef_vi` kernel-bypass UDP receiver with the poll-driven shape (`open` / `poll` / `close`)
- (P) network/types.hpp              -- Network value types: `endpoint_config { address; port }` and `datagram_view = string_view`

### order_entry

Base path: `src/order_entry/order_entry/`

Inbound-edge domain. Turns one framed wire packet into one typed order-entry request through pure synchronous code over value types. Owns the request vocabulary, the strong-typed field primitives, the abstract decoder boundary and its CSV implementation, the pipeline-stage session that drives the decoder, and the structured error vocabulary raised at the decoder boundary. A `runtime/` shell wraps the session with a UDP receiver behind a `setup`/`start`/`poll`/`stop` lifecycle.

Headers:

- (I) csv_decoder.hpp            -- CSV decoder for the fixed-shape CSV protocol.
- (I) decoder.hpp                -- Abstract decoder boundary: one packet to one `lab::result<request>`; supports test doubles.
- (I) error_code.hpp             -- `order_entry::error_code` enum (201xxx range) plus `to_string` and category registration.
- (P) errors.hpp                 -- Structured `invalid_field`, `missing_field`, `unknown_order`, `parser_error` carried through `boost::leaf`.
- (I) factories.hpp              -- `make_rejection` overloads composing a `rejection` from each structured decoder error and the raw payload.
- (P) messages.hpp               -- Request values `new_order_single`/`replace_order`/`cancel_order`/`flush`, lifecycle events `execution_report`/`cancel_reject`, and the request/event variants.
- (I) session.hpp                -- Pipeline-stage `session`: synchronous `send(packet)` plus `on_request`/`on_rejected` callbacks.
- (P) types.hpp                  -- Strong-typed order-entry primitives (`client_id`, `cl_ord_id`, `orig_cl_ord_id`, `security_id`, `symbol`, `price`, `quantity`) plus order lifecycle enums.
- (P) runtime/session.hpp        -- Threaded composer owning UDP receiver, decoder, and inner session; lifecycle `setup`/`start`/`poll`/`stop`.
- (P) runtime/session_config.hpp -- Variant-typed config selecting the UDP receiver backend (asio or ef_vi) and the decoder.

### order_client

Base path: `src/order_client/order_client/`

Typed client library for driving the server from examples and local tools. It keeps callers on the same `order_entry` request vocabulary as the server boundary, encodes each request to the inbound CSV command protocol, and sends each command as a UDP datagram through a Boost.Asio-backed sender.

Headers:

- (P) client.hpp      -- Public typed API: `connect()` plus `send(new_order_single)`, `send(replace_order)`, `send(cancel_order)`, `send(flush)`, and `send(request)`.
- (I) csv_encoder.hpp -- Encodes typed order-entry requests into outbound CSV command records.
- (I) udp_sender.hpp  -- Boost.Asio UDP sender with configurable endpoint.

### market_data

Base path: `src/market_data/market_data/`

Outbound boundary that turns typed events into CSV records and hands them to a sink. The encoder, publisher pipeline stage, and sink interface are pure synchronous code over value types; thread ownership lives in the runtime composer. The default backend is an spdlog single-threaded stdout logger pinned to the publisher thread, which skips the per-write mutex of the multi-threaded variant.

Headers:

- (I) csv_encoder.hpp              -- `csv_encoder` and `csv_encoder_detail` free functions: one CSV record per message variant.
- (I) encoder.hpp                  -- Abstract encoder interface: one typed message to one wire record via `std::string encode(const message&)`.
- (P) messages.hpp                 -- Market-data event structs (`security_definition`, `security_status`, `execution_summary`, `trade`, `mbo_book_update`) and the `message` variant.
- (I) publisher.hpp                -- Pipeline stage holding `encoder&` and `sink&`; `send()` encodes then writes.
- (I) sink.hpp                     -- Abstract sink interface: `write(string_view record)` to whatever line-oriented transport the wiring picks.
- (I) spdlog_sink.hpp              -- Default sink: single-threaded spdlog stdout logger, raw `%v` pattern, `flush_on(info)`.
- (P) types.hpp                    -- Strong-type market-data vocabulary (`security_id`, `symbol`, `security_exchange`, `price`, `quantity`, `order_id`, `trade_id`) plus side, book-update, status, and trade-condition enums.
- (P) runtime/publisher.hpp        -- Runtime composer that owns the chosen encoder and sink and constructs the inner `market_data::publisher`.
- (P) runtime/publisher_config.hpp -- Variant-of-variants config surface (`encoder_config`, `sink_config`); each new backend is a new alternative.

### matching_engine

Base path: `src/matching_engine/matching_engine/`

Matches incoming order-entry requests against per-symbol bid/ask ladders and emits order-entry lifecycle events separately from market-data events. Owns one shared pool over every resting `order_node`, a cross-symbol `(client_id, cl_ord_id)` identity index for cancellation and replacement in one hop, and one preallocated book per configured symbol. Production picks the v3 intrusive-list book through module-root aliases.

Headers:

- (I) conversions.hpp           -- Free `to_market_side` mapping order-entry side to market-data side at emit sites.
- (P) engine.hpp                -- Production engine alias: `using engine = v3::engine`.
- (P) engine_config.hpp         -- Engine startup config: `valid_symbols`, `expected_resting_orders`, `node_pool_chunk_size`.
- (I) error_code.hpp            -- `error_code` enum (`duplicate_order`, `unknown_symbol`) in the 202xxx range.
- (I) errors.hpp                -- Structured leaf payloads (`duplicate_order`, `unknown_symbol`) for new-order rejections.
- (I) factories.hpp             -- `make_order_state(new_order_single)`: builds the matcher's taker / resting payload.
- (P) order_book.hpp            -- Production aliases: `order_book = v3::flat_order_book`, `order_node = v3::order_node`.
- (P) order_state.hpp           -- Resting-order payload struct carrying client id, cl_ord_id, instrument identity, order terms, order quantity, and leaves quantity.
- (I) types.hpp                 -- Composite `order_key{client_id, cl_ord_id}` plus boost/std hash bindings for the identity index.
- (I) v1/engine.hpp             -- v1 engine: in-place `match_buy` / `match_sell` over vector-backed books; logs rejections.
- (I) v1/order_book.hpp         -- v1 book: `std::map<price, std::vector<order_state>>` per side; cancel scans linearly.
- (I) v2/engine.hpp             -- v2 engine: drives the book through `fill_top_*_front`; engine-owned `boost::pool`.
- (I) v2/order_book.hpp         -- v2 book: intrusive list per level over a caller-owned pool; `place` returns a node handle.
- (P) v3/engine.hpp             -- v3 engine: owns pool, identity index, books map; dispatches via `lab::match` and `boost::leaf`.
- (I) v3/matching.hpp           -- Side-agnostic `match` template: walks levels, calls `consume_level`, bulk-erases consumed prefix.
- (I) v3/order_book.hpp         -- v3 templated book: `flat_order_book` over `flat_map`; per-level FIFO with running `total_remaining`.
- (I) v3/order_node.hpp         -- Intrusive `list_base_hook<normal_link>` wrapping `order_state`; safe-shutdown invariant.
- (P) runtime/engine.hpp        -- Composer holding `optional<engine>`; exposes `setup` / `send` / `on_market_data` / `on_order_entry` for the wiring shell.
- (P) runtime/engine_config.hpp -- Deployment-default mirror of `engine_config` (defaults: chunk 32, expected 1024).

### server

Base path: `src/server/server/`

Wiring shell that composes the project's domain runtimes into a three-thread application (input loop, processing loop, output loop) pinned to named threads, with cross-thread hops implemented as `post` closures and the receiver driven by a poller registered on the input loop. Owns the three `lab::event_loop` instances, the three runtime composers, and the optional `boost::asio::io_context` the asio receiver backend borrows. Brings the pipeline up outbound-to-inbound so each consumer is live before its producer can post, and tears it down in reverse; backend selection is a config-time choice rather than a wiring change. Provides the `server` executable entry point, which translates `SIGINT` / `SIGTERM` into application lifecycle.

Headers:

- (I) application.hpp -- `config` selecting backends and per-loop thread names; `application` owning the loops, runtimes, and `start`/`run`/`stop` lifecycle.

### client

Base path: `src/client/`

Thin command-line sender over `order_client::client`. It reads CSV commands from `--input` or stdin, decodes them to typed `order_entry::request` values, and sends each one to the configured UDP endpoint. CLI options are `--host`, `--port`, and `--input`.
