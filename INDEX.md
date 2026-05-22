# INDEX

## System overview

Multi-threaded UDP-driven order book matching engine. The `server` binary ingests JSON order-entry commands over UDP, matches them against per-symbol books, and publishes the resulting market-data records to stdout. The `client` binary sends typed order-entry commands from files or stdin to the server. The codebase is organised as static libraries for shared vocabulary, order entry, order clients, market data, matching, runtime wiring, and the scaffolded order-routing / market-data codec stack, plus Catch2 unit tests.

## Tech stack

- C++26
- CMake (presets: debug, release, asan, tsan, clang)
- Ninja generator
- Boost (toolchain-provided via `BOOST_ROOT`, header-only) -- https://www.boost.org -- LEAF, container_hash, pfr, algorithm, stacktrace, asio, intrusive, pool, unordered
- fmt (FetchContent, header-only) -- https://github.com/fmtlib/fmt
- nlohmann/json (FetchContent, header-only) -- https://github.com/nlohmann/json
- spdlog (FetchContent) -- https://github.com/gabime/spdlog
- Catch2 v3 (FetchContent, tests) -- https://github.com/catchorg/Catch2
- QuickFIX (FetchContent, scaffolded for later integration) -- https://github.com/quickfix/quickfix
- NamedType (FetchContent, MIT) -- https://github.com/joboccara/NamedType
- SG14 inplace_function (FetchContent, BSL-1.0) -- https://github.com/WG21-SG14/SG14
- moodycamel ReaderWriterQueue (FetchContent, BSD-2-Clause) -- https://github.com/cameron314/readerwriterqueue

## Directory structure

```
matching-engine-lab/
|-- b3-entrypoint-messages-8.4.2.xml      B3 entrypoint reference protocol file for codec scaffolds
|-- b3-market-data-messages-2.2.0.xml     B3 market-data reference protocol file for codec scaffolds
|-- cmake/                                shared CMake modules (compiler flags, sanitizers)
|-- docs/                                 ADRs, runbooks, C++ design principles, engine spec, runtime docs
|   |-- adr/                              architecture decision records (numbered, dated)
|   `-- runbooks/                         step-by-step local and operational procedures
|-- examples/                             JSON configs, demo scenarios, and expected market-data output
|-- scripts/                              developer scripts (formatting, pre-commit, toolchain install)
|-- src/                                  library sources and client/server executables
|   |-- lab/                              general-purpose vocabulary helpers
|   |   |-- lab/                          public headers
|   |   |-- libs/                         network adapter sub-library (asio + ef_vi UDP receivers)
|   |   `-- src/                          reserved for utility translation units
|   |-- client/                          CLI sender; the client executable target
|   |-- market_data/                      outbound domain: typed events to JSON records to stdout
|   |-- matching_engine/                  composition domain: per-symbol books and the production matching loop
|   |-- mmd/                              normalized market-data event domain
|   |-- mmd_json/                         JSON rendering for normalized market-data events
|   |-- mmd_transport/                    encoded market-data delivery transport boundaries
|   |-- mmdfix/                           FIX-shaped market-data records
|   |-- mor/                              normalized order-routing domain
|   |-- morfix/                           FIX-shaped order-routing records
|   |-- morfix_quickfix/                  glue between morfix, ospec, and the FIX engine boundary
|   |-- order_client/                     typed client API: order-entry requests to UDP JSON datagrams
|   |-- order_entry/                     inbound domain: UDP JSON bytes to typed order-entry requests
|   |-- ospec/                            venue tag and value normalization profiles
|   |-- quickfix_fix/                     local QuickFIX-compatible message/session boundary
|   `-- server/                           wiring shell + main; the server executable target
|-- test/                                 module Catch2 tests
`-- vendor/                               CMake FetchContent dependency declarations
```

## Modules

### lab

Base path: `src/lab/lab/`

The project's general-purpose vocabulary layer -- the "internal Boost" the rest of the code is written against. It owns a small set of in-house primitives (strong types, fixed-size strings, an event loop, a structured error carrier, match-over-variant) and re-exports third-party types behind a `lab::` alias so a dependency swap is a single-header change. A `network` sub-namespace adds two UDP-receiver adapters (Boost.Asio and an `ef_vi` stub) plus their shared value types.

Headers:

- (P) algorithm.hpp        -- `string_view` `trim` / `ltrim` / `rtrim` plus a fixed-arity `split_fields<N>` that returns `array<string_view, N>`
- (P) assert.hpp           -- `LAB_ASSERT(expr)` macro: prints expression, location, and a `boost::stacktrace` dump, then aborts
- (P) charconv.hpp         -- `from_chars<T>` returning `lab::result`, with exact / partial modes and overloads for strong types and fixed strings
- (I) concurrent_queue.hpp -- Aliases `moodycamel::ReaderWriterQueue` (and its blocking variant) as the project's SPSC lock-free queues
- (P) defaulted_field.hpp  -- Transparent config-field wrapper carrying a compile-time default plus `LAB_DEFAULTED_FIELD`
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
- (P) json.hpp             -- nlohmann/json adapter with strong-type, fixed-string, optional, field, and result-returning parse helpers
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

Inbound-edge domain. Turns one framed wire packet into one typed order-entry request through pure synchronous code over value types. Owns the request vocabulary, the strong-typed field primitives, the abstract decoder boundary and its JSON implementation, the pipeline-stage session that drives the decoder, and the structured error vocabulary raised at the decoder boundary. A `runtime/` shell wraps the session with a UDP receiver behind a `setup`/`start`/`poll`/`stop` lifecycle.

Headers:

- (I) json_decoder.hpp           -- JSON decoder for the order-entry command protocol.
- (I) decoder.hpp                -- Abstract decoder boundary: one packet to one `lab::result<request>`; supports test doubles.
- (I) error_code.hpp             -- `order_entry::error_code` enum (201xxx range) plus `to_string` and category registration.
- (P) errors.hpp                 -- Structured `invalid_field`, `missing_field`, `unknown_order`, `parser_error` carried through `boost::leaf`.
- (I) factories.hpp              -- `make_rejection` overloads composing a `rejection` from each structured decoder error and the raw payload.
- (P) messages.hpp               -- Request values `new_order_single`/`replace_order`/`cancel_order`/`flush`, lifecycle events `execution_report`/`cancel_reject`, and the request/event variants.
- (I) session.hpp                -- Pipeline-stage `session`: synchronous `send(packet)` plus `on_request`/`on_rejected` callbacks.
- (P) types.hpp                  -- Strong-typed order-entry primitives (`client_id`, `cl_ord_id`, `orig_cl_ord_id`, `security_id`, `symbol`, `price`, `quantity`) plus order lifecycle enums.
- (P) runtime/session.hpp        -- Threaded composer owning UDP receiver, decoder, and inner session; lifecycle `setup`/`start`/`poll`/`stop`.
- (P) runtime/session_config.hpp -- Variant-typed config selecting the UDP receiver backend (asio or ef_vi), decoder, and decoder datagram-size guard.

### order_client

Base path: `src/order_client/order_client/`

Typed client library for driving the server from examples and local tools. It keeps callers on the same `order_entry` request vocabulary as the server boundary, encodes each request to the inbound JSON command protocol, and sends each command as a UDP datagram through a Boost.Asio-backed sender.

Headers:

- (P) client.hpp      -- Public typed API: `connect()` plus `send(new_order_single)`, `send(replace_order)`, `send(cancel_order)`, `send(flush)`, and `send(request)`.
- (I) json_encoder.hpp -- Encodes typed order-entry requests into outbound JSON command records.
- (I) udp_sender.hpp  -- Boost.Asio UDP sender with configurable endpoint and datagram-size guard.

### mor

Base path: `src/mor/mor/`

Normalized Matching Engine Order Routing domain. It wraps the current
`order_entry` request and lifecycle-event vocabulary with target codec-stack
names, plus callback interfaces for source/sink/pipeline wiring. The matching
engine runtime still uses the phase 5 JSON path; these conversions keep later
codec work buildable while that migration proceeds.

Headers:

- (P) messages.hpp    -- Normalized order-routing requests (`new_order_single`, `replace_request`, `cancel_request`, `flush_request`) and events (`execution_report`, `cancel_reject`, `parser_reject`).
- (P) interfaces.hpp  -- Source, sink, and pipeline-stage callback interfaces with wiring helpers.
- (P) conversions.hpp -- Compatibility conversions between `order_entry` messages and normalized `mor` messages.

### morfix

Base path: `src/morfix/morfix/`

Canonical FIX-shaped order-routing layer. It translates between normalized
`mor` requests/events and FIX-flavoured records, and owns lifecycle scaffolding
such as request correlation and `ExecID` allocation.

Headers:

- (P) messages.hpp    -- FIX-shaped order-routing requests, execution reports, and cancel rejects using standard tag names as field vocabulary.
- (P) conversions.hpp -- Conversion helpers between normalized `mor` requests/events and FIX-shaped records.
- (P) session.hpp     -- Lifecycle-state scaffold for `ExecID` allocation and request lookup by `ClOrdID`.

### ospec

Base path: `src/ospec/ospec/`

Venue specification layer. The first profile is `ospec::b3`, which carries tag
constants, reference protocol file names, and normalized value mappings for the
simplified order-routing and market-data surface.

Headers:

- (P) b3.hpp -- B3 tag constants plus bidirectional value-normalization functions for order routing and market data.

### quickfix_fix

Base path: `src/quickfix_fix/quickfix_fix/`

Local QuickFIX-compatible FIX engine boundary. It provides the typed message,
text codec, and in-memory session interfaces that codec glue depends on through
a package-free implementation for the local FIX loop.

Headers:

- (P) codec.hpp      -- FIX-style `tag=value` text encoder/decoder with SOH and printable delimiter support.
- (I) error_code.hpp -- `quickfix_fix::error_code` enum for local session and text-codec failures.
- (I) errors.hpp     -- Structured errors for disconnected sessions and malformed or incomplete FIX text records.
- (P) message.hpp -- Simple FIX message abstraction with `MsgType`, ordered fields, and tag lookup.
- (P) session.hpp -- Session interface with outbound `send`, inbound message/reject callbacks, and an in-memory connected session pair.

### morfix_quickfix

Base path: `src/morfix_quickfix/morfix_quickfix/`

Glue between `morfix`, `ospec`, and the local QuickFIX-compatible boundary.
The B3 initiator and acceptor codecs map the first order-entry request and
response slice between canonical `morfix` records and local
`quickfix_fix::message` values.

Headers:

- (P) codecs.hpp     -- Initiator and acceptor codec interfaces plus B3 request/event codec implementations.
- (I) error_code.hpp -- `morfix_quickfix::error_code` enum for unsupported codec operations.
- (I) errors.hpp     -- Structured error payloads for unsupported codec operations.

### mmd

Base path: `src/mmd/mmd/`

Normalized Matching Engine Market Data domain. It wraps the current
`market_data` event vocabulary with target codec-stack names so JSON, FIX,
binary codecs, and transports can build against one event surface.

Headers:

- (P) messages.hpp    -- Normalized market-data events: security definition/status, execution summary, trade, and MBO book update.
- (P) conversions.hpp -- Compatibility conversions between current `market_data` messages and normalized `mmd` messages.

### mmd_json

Base path: `src/mmd_json/mmd_json/`

JSON rendering for normalized market-data events. It preserves the phase 5
JSONL record contract while market-data encoding moves behind the modular
boundary.

Headers:

- (P) json_encoder.hpp -- Encodes every `mmd::message` alternative to the existing JSON record shape.

### mmdfix

Base path: `src/mmdfix/mmdfix/`

Canonical FIX-shaped market-data layer. The first scaffold covers the active
trade and MBO book-update events emitted by the matching engine.

Headers:

- (P) messages.hpp    -- FIX-shaped incremental refresh and trade capture records.
- (P) conversions.hpp -- Conversion helpers from normalized `mmd` trade and MBO book-update events.

### mmd_transport

Base path: `src/mmd_transport/mmd_transport/`

Market-data delivery transport boundaries. Transports consume encoded records,
so payload formatting stays separate from delivery choices such as stdout,
WebSocket, or FIX session publication.

Headers:

- (P) sinks.hpp -- Encoded-record sink interface and capture sink for tests and local wiring.

### market_data

Base path: `src/market_data/market_data/`

Outbound boundary that turns typed events into JSON records and hands them to a sink. The encoder, publisher pipeline stage, and sink interface are pure synchronous code over value types; thread ownership lives in the runtime composer. The default backend is an spdlog single-threaded stdout logger pinned to the publisher thread, which skips the per-write mutex of the multi-threaded variant.

Headers:

- (I) json_encoder.hpp             -- `json_encoder` and `json_encoder_detail` free functions: one JSON record per message variant.
- (I) encoder.hpp                  -- Abstract encoder interface: one typed message to one wire record via `std::string encode(const message&)`.
- (P) messages.hpp                 -- Market-data event structs (`security_definition`, `security_status`, `execution_summary`, `trade`, `mbo_book_update`) and the `message` variant.
- (I) publisher.hpp                -- Pipeline stage holding `encoder&` and `sink&`; `send()` encodes then writes.
- (I) sink.hpp                     -- Abstract sink interface: `write(string_view record)` to whatever line-oriented transport the wiring picks.
- (I) spdlog_sink.hpp              -- Default sink: single-threaded spdlog stdout logger, raw `%v` pattern, `flush_on(info)`.
- (P) types.hpp                    -- Strong-type market-data vocabulary (`security_id`, `symbol`, `security_exchange`, `price`, `quantity`, `order_id`, `trade_id`) plus side, book-update, status, and trade-condition enums.
- (P) runtime/publisher.hpp        -- Runtime composer that owns the chosen encoder and sink and constructs the inner `market_data::publisher`.
- (P) runtime/publisher_config.hpp -- Variant-of-variants JSON config surface (`encoder_config`, `sink_config`); each new backend is a new alternative.

### matching_engine

Base path: `src/matching_engine/matching_engine/`

Matches incoming order-entry requests against per-symbol bid/ask ladders and emits order-entry lifecycle events separately from market-data events. Owns one shared pool over every resting `order_node`, a cross-symbol `(client_id, cl_ord_id)` identity index for cancellation and replacement in one hop, and one preallocated book per configured symbol. The production intrusive-list book and matching engine live directly at the module root.

Headers:

- (I) conversions.hpp           -- Free `to_market_side` mapping order-entry side to market-data side at emit sites.
- (P) engine.hpp                -- Production engine: owns pool, identity index, books map; dispatches via `lab::match` and `boost::leaf`.
- (P) engine_config.hpp         -- Engine startup config: `valid_symbols`, `expected_resting_orders`, `node_pool_chunk_size`.
- (I) error_code.hpp            -- `error_code` enum (`duplicate_order`, `unknown_symbol`) in the 202xxx range.
- (I) errors.hpp                -- Structured leaf payloads (`duplicate_order`, `unknown_symbol`) for new-order rejections.
- (I) factories.hpp             -- `make_order_state(new_order_single)`: builds the matcher's taker / resting payload.
- (I) matching.hpp              -- Side-agnostic `match` template: walks levels, calls `consume_level`, bulk-erases consumed prefix.
- (P) order_book.hpp            -- Production order book over `flat_map`; per-level FIFO with running `total_remaining`.
- (I) order_node.hpp            -- Intrusive `list_base_hook<normal_link>` wrapping `order_state`; safe-shutdown invariant.
- (P) order_state.hpp           -- Resting-order payload struct carrying client id, cl_ord_id, instrument identity, order terms, order quantity, and leaves quantity.
- (I) types.hpp                 -- Composite `order_key{client_id, cl_ord_id}` plus boost/std hash bindings for the identity index.
- (P) runtime/engine.hpp        -- Composer holding `optional<engine>`; exposes `setup` / `send` / `on_market_data` / `on_order_entry` for the wiring shell.
- (P) runtime/engine_config.hpp -- Deployment-default mirror of `engine_config` using defaulted JSON fields (defaults: chunk 32, expected 1024).

### server

Base path: `src/server/server/`

Wiring shell that composes the project's domain runtimes into a three-thread application (input loop, processing loop, output loop) pinned to named threads, with cross-thread hops implemented as `post` closures and the receiver driven by a poller registered on the input loop. Owns the three `lab::event_loop` instances, the three runtime composers, and the optional `boost::asio::io_context` the asio receiver backend borrows. Brings the pipeline up outbound-to-inbound so each consumer is live before its producer can post, and tears it down in reverse; backend selection is a config-time choice rather than a wiring change. Provides the `server` executable entry point, which translates `SIGINT` / `SIGTERM` into application lifecycle.

Headers:

- (I) application.hpp -- `config` selecting backends and per-loop thread names; `application` owning the loops, runtimes, and `start`/`run`/`stop` lifecycle.

### client

Base path: `src/client/`

Thin command-line sender over `order_client::client`. It loads its target endpoint and input source from a JSON config file, reads JSON commands from a file or stdin, decodes them to typed `order_entry::request` values, and sends each one to the configured UDP endpoint.
