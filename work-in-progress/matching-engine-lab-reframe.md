# Matching Engine Lab Portfolio Reframe Plan

## Summary

Reframe the repo from a take-home submission into `matching-engine-lab`, a
C++ portfolio demonstrator for a multi-threaded UDP matching engine. Use the
confirmed approach: full legacy utility rename to `lab` and normal root
project layout.

Decision matrix:

| Option | Complexity | Blast radius | Maintenance | Result |
|---|---:|---:|---:|---|
| Public rename only | Low | Low | Poor | Leaves old branding in code |
| Staged internal rename | Medium | Medium | Medium | Safer but prolongs mixed identity |
| Full rename | High | High | Best | Chosen: clean portfolio project |

## Key Changes

- Move layout from `submission/` to root-level `src/`, `test/`, and
  `vendor/`.
- Rename project identity:
  - Repo/docs name: `matching-engine-lab`
  - CMake project: `matching_engine_lab`
  - Utility namespace/library: legacy utility layer -> `lab`
  - CMake aliases: `lab::lab` -> `lab::core`
  - Build options/macros: legacy project prefix -> `LAB_*`
  - Server executable: legacy server name -> `server`
  - Client executable: `client`
  - Runtime namespace: server namespace -> `server`
  - Thread names: `lab-input`, `lab-engine`, `lab-output`
- Keep domain names `order_entry`, `matching_engine`, and `market_data`;
  they are domain vocabulary, not employer branding.
- Rewrite durable docs so they describe a portfolio/demo project:
  - `README.md`, `DEPENDENCIES.md`, `DEVELOPING.md`, `INDEX.md`
  - `docs/*.md`, ADRs, per-library READMEs
  - Replace prompt references with an internal protocol/spec document, likely
    `docs/protocol.md` or expanded `docs/engine-specs.md`.
- Delete interview/submission artifacts:
  - assessment prompt
  - sample runner page
  - generated submission page
  - generated assessment reports under `reports/`
  - take-home wording in comments, scripts, and docs
- Preserve the existing matching behavior. The CSV protocol was an
  intermediate demo surface; phase 5 replaced it with JSON over the same typed
  domain boundaries.
- Phase 6 moves the codec architecture toward the Abacus layering model:
  normalized order routing, canonical FIX rendering, venue specs, concrete FIX
  engine stubs, and glue layers that bind those pieces without collapsing the
  boundaries.
- Phase 7 turns the order-routing codec scaffold into a working local FIX
  client/server loop, using the Abacus `aor` / `aorfix` /
  `aorfix_onixs_fix` layering as the reference shape and a package-free
  QuickFIX-compatible boundary for this portfolio repo.

## Phase Handoffs

### Phase 1 Handoff

Status: completed.

Phase 1 moved the implementation into the root project layout and completed
the identity rename:

- moved library sources to `src/`;
- merged module unit tests into root `test/`;
- moved dependency declarations under `vendor/`;
- renamed the utility module, headers, namespace, macros, and CMake alias to
  `lab` / `lab::core` / `LAB_*`;
- renamed the wiring shell and executable to `server`;
- updated thread names to `lab-input`, `lab-engine`, and `lab-output`;
- rewired root CMake to add `vendor`, `src`, and `test` directly;
- updated durable docs and scripts for the new layout.

Verification completed:

- `./build.sh debug` passed with 70/70 tests.
- Focused search found no remaining legacy branding or `submission/` paths in
  durable project surfaces.

### Phase 2 Handoff

Status: completed.

Phase 2 removed the legacy harness and generated-report workflow surfaces:

- deleted `run_submission.sh`;
- deleted `run_local_submission.sh`;
- deleted tracked generated reports under `reports/`.
- deleted the root assessment prompt, sample runner page, generated submission
  page, and stale standalone design summary.

Phase 2 also updated local workflow documentation and stale comments so the
durable path is the regular local build:

- `README.md` now points at `./build.sh`, `ctest`, and direct server runs;
- `DEVELOPING.md` documents the local server workflow;
- `DEPENDENCIES.md` describes system packages as local setup inputs;
- `INDEX.md`, `docs/highlights.md`, and lab guideline docs no longer describe
  the grading harness as the active workflow;
- CMake and source comments no longer explain behavior in terms of grading or
  report generation.

Verification completed:

- `./build.sh debug` passed with 70/70 tests.
- `git diff --check` passed.
- Focused searches over touched durable surfaces found no remaining grading
  workflow references.

### Phase 3 Handoff

Status: completed.

Phase 3 replaced the shell scenario runner with a first-class UDP client path:

- added `src/order_client/` with typed send APIs for
  `order_entry::new_order_single`, `cancel_order`, `flush`, and `request`;
- added CSV command encoding for outbound order commands;
- added a Boost.Asio-backed UDP sender with configurable endpoint,
  defaulting to `127.0.0.1:1234`;
- added the `client` executable with `--host`, `--port`, and `--input`;
- added `--host` and `--port` to the `server` executable;
- removed the CSV scenario fixture tree from `test/`;
- deleted `test/run_tests.sh`.

Verification completed:

- `ctest --test-dir _build/debug --output-on-failure` passed with the
  remaining unit-test suite.

### Phase 3.1 Handoff

Status: dropped.

Phase 3.1 was removed from the reframe. A separate packaged environment would
duplicate the local C++ toolchain build and make the demo path too heavy for
this portfolio repo.

The retained demo surface is host-local:

- `examples/scenarios/crossing-orders.jsonl` stores the scenario input;
- `examples/expected/crossing-orders-market-data.jsonl` stores the expected
  market-data output;
- `README.md`, `DEVELOPING.md`, `DEPENDENCIES.md`, and `INDEX.md` describe the
  regular `./build.sh` plus direct `server` / `client` workflow.

Verification completed:

- `./build.sh debug` passed with 78/78 tests.
- Local smoke test passed: started `_build/debug/server` on a throwaway UDP
  port, sent `examples/scenarios/crossing-orders.jsonl` with
  `_build/debug/client`, and observed the expected market-data JSONL sequence.
- `git diff --check` passed.

### Phase 4 Handoff

Status: completed.

Phase 4 should replace the current exercise-shaped request and market-data
vocabulary with industry-standard order-entry and market-data message models.
The important boundary is conceptual: order-entry messages and market-data
messages are separate domains, even when they share names such as price,
quantity, side, or security identifier. Use explicit conversions at the
matching-engine boundary rather than shared message structs.

Phase 4 renamed the inbound domain to `order_entry`, introduced typed
`new_order_single`, `replace_order`, `cancel_order`, `execution_report`, and
`cancel_reject` messages, and split the matching engine's outbound callbacks
into order-entry lifecycle events and market-data events. Market data now uses
`security_definition`, `security_status`, `execution_summary`, `trade`, and
`mbo_book_update` as distinct domain events. The CSV demo wire protocol remains
compact, with the decoder and client encoder translating into the typed model.

Verification completed:

- `./build.sh debug` passed with 71/71 tests.

### Phase 5 Handoff

Status: completed for the current demo protocol.

Phase 5 replaced the CSV demo protocol with JSON at the active runtime
boundaries:

- added `lab::json` as a focused nlohmann-backed adapter for strong types,
  fixed strings, optionals, chrono durations, typed parsing, and JSONL reads;
- added `order_entry::json_decoder` for inbound UDP command datagrams;
- added `order_client::json_encoder` and rewired `order_client::client` to send
  JSON command datagrams;
- changed the `client` executable to read JSONL commands, decode them into
  typed `order_entry::request` values, and send them through the typed client;
- added `market_data::json_encoder` and rewired the runtime publisher so stdout
  carries JSON market-data records;
- removed the CSV decoder and encoder sources and their tests from the active
  build;
- updated the README, index, engine spec, module docs, and lab guidelines so
  the documented workflow is JSON over UDP and JSONL at file/stdout boundaries.

Verification completed:

- `./build.sh debug` passed with 78/78 tests.

The current client remains a one-way scenario sender. A later response-channel
phase can add client-side decoding for `execution_report` and `cancel_reject`
and server-side publication of those order-entry lifecycle events if the demo
needs an interactive order-entry response stream.

### Phase 6 Handoff

Status: completed.

Phase 6 scaffolded the fully modular codec architecture. The order-routing
side now follows the Abacus layering model with matching-engine-lab names:

- `mor` -- normalized Matching Engine Order Routing messages and interfaces;
- `morfix` -- canonical FIX rendering of `mor`;
- `ospec` -- venue tags, values, and normalization functions;
- `quickfix_fix` -- QuickFIX-compatible local FIX engine boundary in place of
  the OnixS vendor wrappers;
- `morfix_quickfix` -- glue layer that binds `morfix`, `ospec`, and the FIX
  engine boundary.

Market data now has the same principle with a smaller surface:

- `mmd` -- normalized Matching Engine Market Data events;
- `mmd_json` -- JSON rendering of `mmd` events while preserving the phase 5
  JSONL record shape;
- `mmdfix` -- FIX-shaped market-data records for the first trade and MBO
  book-update slice;
- `mmd_transport` -- encoded-record delivery boundaries.

`ospec::b3` uses `b3-entrypoint-messages-8.4.2.xml` and
`b3-market-data-messages-2.2.0.xml` as reference anchors for the simplified B3
surface.

Durable docs updated:

- `README.md` lists the new codec-stack modules.
- `INDEX.md` maps the new modules and headers.
- `docs/engine-specs.md` names the active JSON path and the codec scaffold.
- `docs/lab-guidelines/design.md` and `testing.md` describe the local module
  responsibilities and test coverage.
- `docs/adr/0005-layer-codecs-behind-normalized-routing-and-market-data-domains.md`
  records the layering decision.

Verification completed:

- `./build.sh debug` passed with 93/93 tests.

### Phase 7 Handoff

Status: completed for the local FIX loop.

Phase 7 made the order-routing FIX stack usable end to end without collapsing
the module boundaries created in phase 6:

- `mor` remains the normalized order-routing domain and keeps compatibility
  conversions to and from `order_entry`;
- `morfix` owns canonical FIX-shaped requests/events plus bidirectional
  conversions to and from `mor`;
- `ospec::b3` owns the first working B3 order-entry tag and value slice with
  bidirectional value normalization;
- `quickfix_fix` owns the local QuickFIX-compatible message, text codec, and
  in-memory initiator/acceptor session bridge;
- `morfix_quickfix` owns B3 initiator and acceptor codecs that map
  `morfix` messages to and from `quickfix_fix::message`;
- tests prove that a `morfix` request can travel through initiator encode,
  local FIX delivery, acceptor decode, server response encode, and initiator
  event decode.

Decision matrix for the phase-7 implementation path:

| Option | Complexity | Blast radius | Reversibility | Testability | Result |
|---|---:|---:|---:|---:|---|
| Add real QuickFIX now | High | High | Medium | Medium | Too much dependency and runtime surface before the codec contract is stable. |
| Build a local QuickFIX-compatible bridge | Medium | Medium | High | High | Chosen: exercises real client/server flow with deterministic unit tests. |
| Keep JSON-only runtime and just document FIX | Low | Low | High | Low | Does not satisfy the phase goal of a working FIX client/server path. |

Completed implementation:

- added `quickfix_fix` error scaffolding, a text message codec, and an
  in-memory `session_pair` shape;
- added reverse `morfix` conversions back to `mor`;
- expanded `ospec::b3` with order-entry response tags and parse helpers;
- replaced `morfix_quickfix` scaffold failures with B3 request/event
  encode/decode logic for new/replace/cancel requests, execution reports, and
  cancel rejects;
- added `quickfix_fix` unit tests for message field replacement, text
  encode/decode, and in-memory session delivery;
- replaced the old `morfix_quickfix` scaffold-failure tests with request,
  response, error-path, and full initiator/acceptor loop tests;
- updated README, INDEX, ADR, engine spec, and lab guideline docs for the
  working local FIX loop.

Follow-ups:

- decide whether the FetchContent `quickfix` dependency should remain as a
  parked later-integration dependency or be deferred until a real package-backed
  session layer is implemented;
- broaden B3 field coverage beyond the first order-entry request/response
  slice;
- add a narrow runtime or example-level client/server harness if the portfolio
  demo should expose FIX beside the JSON UDP app.

Verification completed:

- focused codec/session test pass:
  `ctest --test-dir _build/debug -R "^(quickfix_fix|morfix|ospec|morfix_quickfix)/" --output-on-failure`
  passed with 20/20 tests;
- `./build.sh debug` passed with 104/104 tests;
- `git diff --check` passed.

## Phase 2: Legacy Harness Removal

- Delete grading-specific machinery:
  - `run_submission.sh`
  - `run_local_submission.sh`
- Update `README.md` and `DEVELOPING.md` to document regular local workflows:
  - `./build.sh`
  - direct `cmake --preset=debug`
  - `ctest --test-dir _build/debug --output-on-failure`
  - running `./_build/debug/server`
- Remove grading/package assumptions from `DEPENDENCIES.md`; keep package
  inventory as developer setup context only.

## Phase 3: Runtime Refactor and Client

- Replace the black-box harness with a real UDP client library and app.
- Add `src/order_client/`:
  - public typed client API using `order_entry::new_order_single`,
    `cancel_order`, and `flush`
  - command encoding for outbound order commands
  - UDP sender backed by Boost.Asio
  - configurable endpoint, defaulting to `127.0.0.1:1234`
- Add `src/client/` executable:
  - sends commands from a file or stdin
  - supports a simple CLI: `--host`, `--port`, `--input`
  - exits nonzero on file/socket/config errors
- Keep `server` as the matching-engine server that listens
  on UDP and writes market-data records to stdout.
- Remove the old scenario files and shell-runner workflow from `test/`.
- Keep scenario-shaped coverage in focused unit tests and local demo commands.

### Phase 3.1: Local Demo Decision

Status: dropped.

Keep the complete demo on the host-local path after the client and server path
exists. Scenario inputs and expected market-data outputs live under
`examples/`, and documentation should present `./build.sh`, direct `server`,
and direct `client` commands as the supported demo workflow.

## Phase 4: Industry Message Vocabulary

Move the domain model from exercise-specific command and output names toward
the vocabulary used by trading systems. This phase should preserve the matching
semantics while changing the public domain language and event surfaces.

### Order Entry And Routing

Replace the current routing command and acknowledgement vocabulary with an
order-entry model:

- `new_order_single` -- submit a new order.
- `replace_order` -- amend a live order by original client order identifier.
- `cancel_order` -- request cancellation of a live order.
- `execution_report` -- report order acceptance, rejection, replacement,
  cancellation, partial fill, fill, and expiration.
- `cancel_reject` -- reject a cancel or replace request that cannot be applied.

Use industry field names where they fit:

- client identifiers: `cl_ord_id`, `orig_cl_ord_id`;
- exchange identifiers: `order_id`, `exec_id`;
- instrument identity: `security_id`, `symbol`, `security_exchange`;
- order terms: `side`, `ord_type`, `time_in_force`, `order_qty`, `price`;
- lifecycle state: `exec_type`, `ord_status`, `cum_qty`, `leaves_qty`,
  `last_qty`, `last_px`, `avg_px`, `transact_time`;
- rejection fields: `reject_reason`, `text`.

Keep the matching engine's internal taker/maker vocabulary where it is the
right domain language. The order-entry edge translates external order
lifecycle messages into matching requests and translates matching outcomes back
into `execution_report` or `cancel_reject`.

### Market Data Domain

Split market data completely from order entry. Market-data events should not
reuse order-entry message structs or lifecycle enums. Model the basic event set
after B3 UMDF, but keep it as a domain model rather than a wire schema:

- `security_definition` -- instrument reference data.
- `security_status` -- instrument trading state updates.
- `execution_summary` -- summary of one matching event that produced one or
  more trades.
- `trade` -- one completed trade for an instrument.
- `mbo_book_update` -- market-by-order book update for one resting order or a
  delete-through event.

Initial field set:

- `security_definition`: `security_id`, `symbol`, `security_exchange`,
  `security_group`, `security_type`, optional `security_subtype`,
  `min_price_increment`, `round_lot`, and `currency`.
- `security_status`: `security_id`, `security_exchange`,
  `trading_session_id`, `security_trading_status`,
  `security_trading_event`, and `transact_time`.
- `execution_summary`: `security_id`, `aggressor_side`, `last_px`,
  `fill_qty`, optional `traded_hidden_qty`, optional `cancel_qty`,
  `aggressor_time`, and `transact_time`.
- `trade`: `security_id`, `trade_id`, `price`, `quantity`,
  optional `buyer`, optional `seller`, `trade_condition`,
  optional `trade_sub_type`, `trade_date`, and `transact_time`.
- `mbo_book_update`: `security_id`, `update_action`, `side`,
  `resting_order_id`, `price`, `quantity`, optional `previous_quantity`,
  and `transact_time`.

Do not model snapshot or incremental feeds as domain-event variants in this
phase. Snapshot and incremental behavior is a transport or packaging concern:
the same `security_definition`, `security_status`, `execution_summary`,
`trade`, and `mbo_book_update` values should be usable by either packaging
style later.

### Implementation Shape

- Rename or wrap `order_entry` APIs so external naming moves to order-entry
  vocabulary without leaking old command names into new public headers.
- Replaced `market_data::order_ack`, `cancel_ack`, and `top_of_book` outputs
  with the market-data event set above.
- Add narrow conversion functions at the boundaries:
  - order entry request to matching-engine request;
  - matching-engine outcome to order-entry `execution_report` /
    `cancel_reject`;
  - matching-engine book and trade outcomes to market-data events.
- Keep order-entry and market-data strong types distinct unless a shared type
  is truly infrastructure-level, such as a timestamp or raw decimal utility.
- Document the new message model in `docs/engine-specs.md` or a new
  `docs/protocol.md`, then point README-level docs at that durable source.

## Phase 5: JSON Wire Protocol And Codecs

Phase 5 is the current baseline. The active demo protocol is JSON over UDP for
order-entry commands and JSONL for files and stdout. `lab::json` carries the
shared JSON helpers, `order_entry::json_decoder` decodes inbound command
datagrams, `order_client::json_encoder` encodes outgoing command datagrams, and
`market_data::json_encoder` writes market-data records. This phase removed the
CSV codecs from the active build.

Keep this path buildable while phase 6 scaffolds the modular codec stack. The
current JSON command surface can move behind the new module boundaries once
`mor` and `mmd` exist.

## Phase 6: Modular Codec Architecture Scaffold

Scaffold the order-routing and market-data codec architecture so later phases
can fill in real B3/FIX behavior without changing module boundaries.

### Order Routing Stack

Follow the Abacus layering model, renamed for this project:

| Abacus module | Matching-engine-lab module | Role |
|---|---|---|
| `aor` | `mor` | Normalized Matching Engine Order Routing domain used by the matching engine, clients, risk stages, and tests. |
| `aorfix` | `morfix` | Canonical FIX-shaped rendering of `mor`, independent of any FIX engine. |
| `ospec` | `ospec` | Venue-specific tags, values, field wrappers, and normalization functions. |
| `onixs_fix` / `onixs_bentry` | `quickfix_fix` | QuickFIX-compatible local FIX engine boundary used instead of vendor wrappers. |
| `aorfix_onixs_fix` | `morfix_quickfix` | Glue layer that composes `morfix`, `ospec`, and the FIX engine boundary into initiator and acceptor sessions. |

`mor` owns the normalized request and event model:

- requests: `new_order_single`, `replace_request`, `cancel_request`, and
  `flush_request` for the lab scenario surface;
- events: `execution_report`, `cancel_reject`, and any explicit session or
  parser reject event the runtime needs;
- interfaces: `source`, `sink`, and `pipeline_stage` with bidirectional wiring
  helpers matching the Abacus callback convention;
- conversions between current `order_entry` messages and `mor` while the
  rename is staged.

`morfix` owns the canonical FIX layer:

- FIX-flavoured request and event structs with standard tag names and value
  enums;
- initiator and acceptor session abstractions that translate between `mor` and
  FIX-shaped messages;
- ClOrdID, OrigClOrdID, ExecID, request correlation, and order lifecycle
  bookkeeping that belongs to FIX order routing rather than the matching
  engine.

`ospec` owns venue profiles. Start with `ospec::b3` and keep the first slice
small:

- tag constants and strong field wrappers for the fields used by
  `new_order_single`, `replace_request`, `cancel_request`,
  `execution_report`, and `cancel_reject`;
- value constants and normalization functions for side, order type,
  time-in-force, order status, execution type, liquidity, aggressor, reject
  reason, and security identity;
- references to `b3-entrypoint-messages-8.4.2.xml` for order-entry business
  messages and field names.

The `quickfix_fix` layer is a stub in this phase. It should expose local typed
session and message interfaces that `morfix_quickfix` can depend on, with no
hard dependency on a QuickFIX package until a later implementation phase.

`morfix_quickfix` owns the concrete adapter shape:

- `initiator_codec` and `acceptor_codec` interfaces modelled after
  Abacus `aorfix_onixs_fix`;
- `codecs::b3` stubs that map between `morfix` messages and the local FIX
  message abstraction using `ospec::b3` normalization;
- `initiator_sink`, `acceptor_source`, and runtime session stubs that compile
  and wire callbacks, even when the codec bodies return
  `not_implemented`-style errors.

### Market Data Stack

Market data uses the same modular boundary with fewer moving parts because the
matching engine produces the events:

- `mmd` owns normalized Matching Engine Market Data events:
  `security_definition`, `security_status`, `execution_summary`, `trade`, and
  `mbo_book_update`;
- `mmd_json` owns JSON and JSONL rendering of `mmd` events, replacing the
  current `market_data::json_encoder` location once the module exists;
- `mmdfix` owns FIX-shaped market-data records and B3 normalization hooks;
- transport interfaces separate encoded payloads from delivery, with initial
  stdout and stub WebSocket/FIX transports.

Use `b3-market-data-messages-2.2.0.xml` as the reference for B3 market-data
message names, field names, and enumerated values. The first scaffold should
cover only the messages already emitted by the engine.

### Migration Shape

- Add empty or minimal CMake targets for `mor`, `morfix`, `ospec`,
  `quickfix_fix`, `morfix_quickfix`, `mmd`, `mmd_json`, `mmdfix`, and
  market-data transport stubs.
- Move or wrap current `order_entry` messages behind `mor` without changing
  matching behavior.
- Move or wrap current `market_data` messages behind `mmd` without changing
  matching behavior.
- Keep the current JSON demo path running through compatibility adapters while
  the new module names land.
- Update `README.md`, `INDEX.md`, module READMEs, and lab guidelines after the
  scaffold lands so durable docs describe the modular codec architecture.
- Add an ADR for the order-routing and market-data codec layering decision.

## Phase 7: Working FIX Client/Server Loop

Phase 7 turns the phase-6 scaffold into a deterministic local FIX
client/server path. The goal is not to replace the active JSON UDP demo yet;
the goal is to prove the order-routing codec stack can carry real messages
through the Abacus-shaped layers.

### Scope

The first working loop should cover the order-entry business messages already
present in the lab:

- client requests:
  - `new_order_single`;
  - `replace_request` / FIX `OrderCancelReplaceRequest`;
  - `cancel_request` / FIX `OrderCancelRequest`;
- server events:
  - `execution_report`;
  - `order_cancel_reject`.

`flush_request` remains lab-local and should not be encoded as a FIX business
message.

### Module Responsibilities

`mor`:

- keep the normalized request/event vocabulary stable;
- keep conversions to and from `order_entry`;
- add tests for any new event fields needed by the FIX path.

`morfix`:

- keep canonical FIX-shaped structs independent of a FIX engine package;
- add reverse conversions from `morfix` requests/events back to `mor`;
- keep request-correlation and `ExecID` helpers small until runtime session
  behavior needs more state.

`ospec::b3`:

- add B3 tag constants for the fields carried by the phase-7 request and
  response messages;
- provide bidirectional normalization for `side`, `ord_type`,
  `time_in_force`, `exec_type`, `ord_status`, and reject reasons;
- keep the first slice anchored to `b3-entrypoint-messages-8.4.2.xml`.

`quickfix_fix`:

- provide a package-free `message` abstraction with `MsgType` and ordered
  fields;
- provide a text codec for FIX-style `tag=value` payloads so tests can inspect
  real wire-shaped records;
- provide an in-memory initiator/acceptor session pair for deterministic tests.

`morfix_quickfix`:

- implement B3 initiator request encoding and event decoding;
- implement B3 acceptor request decoding and event encoding;
- keep unsupported message types as structured `lab::result` failures;
- add a full local loop test that proves initiator and acceptor codecs compose
  over the `quickfix_fix` session boundary.

### Runtime Integration

Phase 7 should stop at a local FIX loop unless the codec/session layer is
already green. A later phase can decide whether to expose a CLI FIX client,
wire FIX into `server`, or keep FIX as a library-level demonstrator beside the
JSON UDP app.

### Documentation Updates

After the working path is verified:

- update module READMEs for `mor`, `morfix`, `ospec`, `quickfix_fix`, and
  `morfix_quickfix`;
- update `INDEX.md` for added headers, sources, and tests;
- update `README.md` only if the user-facing demo workflow changes;
- update `docs/lab-guidelines/design.md` and `testing.md` with the local
  phase-7 codec/session testing pattern.

## Test Plan

- After phase 1:
  - `./build.sh debug`
  - `./build.sh release server`
  - focused search for legacy branding, `submission/` paths, and assessment
    vocabulary in durable project surfaces
- After phase 2:
  - `./build.sh debug`
  - confirm no grading wrapper docs remain
- After phase 3:
  - unit tests for client command encoding
  - unit tests for client config validation
  - integration test: start `server`, send fixture
    commands with `client`, compare market-data output
  - sanitizer pass with `./build.sh asan`
- After phase 4:
  - unit tests for order-entry lifecycle mapping:
    `new_order_single`, `replace_order`, `cancel_order`,
    `execution_report`, and `cancel_reject`
  - unit tests for market-data event construction:
    `security_definition`, `security_status`, `execution_summary`, `trade`,
    and `mbo_book_update`
  - regression tests proving order-entry types and market-data types are not
    interchangeable at API boundaries
  - scenario-shaped tests updated from legacy CSV records to the new event model
    or its chosen wire encoding
- After phase 5:
  - unit tests for `lab::json` strong type, optional, fixed string, chrono,
    typed parse, and JSONL helpers
  - order-entry client codec tests for request encoding
  - order-entry server codec tests for request decoding
  - market-data server codec tests for event encoding
  - JSON parse or approval tests for representative order-entry and
    market-data variants
  - integration test: start `server`, send JSONL commands with `client`, and
    compare JSONL market-data output
- After phase 6:
  - compile tests for the `mor` source, sink, pipeline-stage, and wiring
    interfaces
  - conversion tests between current `order_entry` messages and `mor`
    requests/events
  - `morfix` session and lifecycle tests for ClOrdID, OrigClOrdID, ExecID, and
    request correlation stubs
  - `ospec::b3` normalization tests for the simplified order-entry and
    market-data value sets
  - `morfix_quickfix` codec-interface tests proving the initiator and acceptor
    stubs compile and route typed failures through `lab::result`
  - `mmd` and `mmd_json` tests proving current market-data events still encode
    to the phase 5 JSONL shape through the new boundary
  - focused build: `./build.sh debug`
- After phase 7:
  - `quickfix_fix` tests for message field replacement, FIX text
    encode/decode, malformed payloads, and in-memory session delivery
  - `morfix` reverse-conversion tests for requests and events
  - `ospec::b3` bidirectional normalization tests for the order-entry value
    set
  - `morfix_quickfix` tests for request encoding, request decoding, event
    encoding, event decoding, unsupported message handling, and missing
    required fields
  - local loop test: initiator request -> FIX message -> acceptor request,
    then acceptor event -> FIX message -> initiator event
  - focused build for the touched codec targets
  - `./build.sh debug`

## Assumptions

- The deleted `work-in-progress/index-skill.md` shown in `git status` is
  user-owned and should not be restored unless explicitly requested.
- `work-in-progress/` remains ephemeral; the reframe plan can live there
  during execution but should not be linked from durable docs.
- CSV has been replaced in the active demo path. Phase 6 should preserve the
  JSON path while the modular codec scaffold lands.
