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

- Move layout from `submission/` to root-level `src/`, `test/`,
  `benchmarks/`, and `vendor/`.
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
- Preserve the existing matching behavior. The CSV protocol is only an
  intermediate demo surface; phase 5 replaces it with JSON over the same typed
  domain boundaries.

## Phase Handoffs

### Phase 1 Handoff

Status: completed.

Phase 1 moved the implementation into the root project layout and completed
the identity rename:

- moved library sources to `src/`;
- merged module unit tests into root `test/`;
- moved Google Benchmark targets to `benchmarks/`;
- moved copy-vendored dependencies to `vendor/`;
- renamed the utility module, headers, namespace, macros, and CMake alias to
  `lab` / `lab::core` / `LAB_*`;
- renamed the wiring shell and executable to `server`;
- updated thread names to `lab-input`, `lab-engine`, and `lab-output`;
- rewired root CMake to add `vendor`, `src`, `test`, and `benchmarks`
  directly;
- updated durable docs and scripts for the new layout.

Verification completed:

- `./build.sh debug` passed with 70/70 tests.
- Focused search found no remaining legacy branding or `submission/` paths in
  durable project surfaces.

### Phase 2 Handoff

Status: completed.

Phase 2 removed the Docker and generated-report workflow surfaces:

- deleted `Dockerfile`;
- deleted `.dockerignore`;
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
  the Docker or grading harness as the active workflow;
- CMake and source comments no longer explain behavior in terms of Docker,
  grading, or report generation.

Verification completed:

- `./build.sh debug` passed with 70/70 tests.
- `git diff --check` passed.
- Focused searches over touched durable surfaces found no remaining Docker
  wrapper or grading workflow references.

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

Phase 3.1 should reintroduce Docker as a compose stack for the complete local
environment. This is a project-owned demo and development workflow, not a
grading harness. At this stage, the stack should run the client and matching
engine together.

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

Status: pending.

Phase 5 should replace the CSV demo protocol with JSON codecs built on a
`lab::json` port of Abacus `mil::json`. Phase 4 owns the domain vocabulary;
phase 5 owns how clients, servers, scenario files, and stdout encode those
typed messages.

## Phase 2: Docker Removal

- Delete grading-specific Docker machinery:
  - `Dockerfile`
  - `.dockerignore`
  - `run_submission.sh`
  - `run_local_submission.sh`
- Update `README.md` and `DEVELOPING.md` to document regular local workflows:
  - `./build.sh`
  - direct `cmake --preset=debug`
  - `ctest --test-dir _build/debug --output-on-failure`
  - running `./_build/debug/server`
- Remove Docker/grading/package assumptions from `DEPENDENCIES.md`; keep
  package inventory as developer setup context only.

## Phase 3: Runtime Refactor and Client

- Replace the black-box harness with a real UDP client library and app.
- Add `src/order_client/`:
  - public typed client API using `order_entry::new_order_single`,
    `cancel_order`, and `flush`
  - CSV encoding for outbound order commands
  - UDP sender backed by Boost.Asio
  - configurable endpoint, defaulting to `127.0.0.1:1234`
- Add `src/client/` executable:
  - sends commands from a file or stdin
  - supports a simple CLI: `--host`, `--port`, `--input`
  - exits nonzero on file/socket/config errors
- Keep `server` as the matching-engine server that listens
  on UDP and writes market data CSV to stdout.
- Remove the old scenario files and shell-runner workflow from `test/`.
- Keep scenario-shaped coverage in focused unit tests and local demo commands.

### Phase 3.1: Compose Stack

Reintroduce Docker as a compose-based local environment after the client and
server path exists. Model the structure after the `fleet` and `observability`
repos:

- Use a small root compose file as the entry point.
- Put service-specific compose fragments under a Docker or infrastructure
  subtree, then include them from the root compose file.
- Use shared env-file anchors for image versions and local overrides.
- Use a named bridge network for the matching-engine lab stack.
- Define one service for `server` and one service for
  `client`; the client should target the server by service
  name inside the compose network.
- Treat health checks, startup ordering, and one-shot client runs as part of
  the compose contract so the stack can demonstrate a full scenario.
- Keep scenario inputs and expected market-data outputs in the same example or
  integration-fixture locations chosen for phase 3, rather than embedding them
  in compose files.
- Document the compose workflow as an optional full-environment path alongside
  the local build workflow.

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

Replace CSV order-entry input and market-data output with JSON. Keep the typed
domain model as the source of truth and make JSON a concrete codec layer around
those domain values.

### Lab JSON Port

Port Abacus `mil::json` into the lab utility layer as `lab::json`:

- base the port on `/home/msi/abacus_workspace/abacus/src/mil/mil/json.hpp`;
- rename the macros and helper names to the lab vocabulary, for example
  `LAB_AUTO_JSON_PFR`, `LAB_AUTO_JSON_PFR_NAMESPACE`, `lab::json::dump`,
  `lab::json::try_parse`, and `lab::json::read_jsonl`;
- preserve support for strong types, fixed strings, optional fields,
  defaulted fields, enums by name, chrono values, variants with explicit type
  tags, and Boost.Asio buffers;
- add focused `test/lab/` coverage based on
  `/home/msi/abacus_workspace/abacus/test/mil/json/src/test_json.cpp` and
  `test_complex_type.cpp`;
- register auto JSON for the order-entry and market-data namespaces after the
  phase 4 domain types exist.

Use Abacus as a reference, not a drop-in namespace copy. The public utility in
this repo should be `lab::json`.

### Message Envelope

Use one JSON object per message. Each object should carry a stable message
type discriminator plus the domain fields for that message. Prefer the same
field names as the phase 4 domain types so examples, tests, and logs read like
the C++ model.

For files and stdout, use JSONL: one JSON message object per line. For UDP, use
one JSON message object per datagram. This keeps the existing local workflow
simple while making the payload self-describing.

### Order Entry Codecs

Add concrete codecs for the two order-entry roles:

- client-side codec:
  - encodes `new_order_single`, `replace_order`, and `cancel_order` requests;
  - decodes `execution_report` and `cancel_reject` responses;
  - is used by `order_client` and the `client` executable.
- server-side codec:
  - decodes order-entry requests from UDP datagrams;
  - encodes order-entry responses emitted by the matching pipeline;
  - is wired at the server boundary before requests enter the matching engine
    and after matching outcomes become order-entry responses.

The codec interfaces should be explicit, following the direction of Abacus
AOR/FIX codecs:

- `/home/msi/abacus_workspace/abacus/src/aor/aor/messages.hpp` for normalized
  request and response type shape;
- `/home/msi/abacus_workspace/abacus/src/aorfix_onixs_fix/aorfix_onixs_fix/initiator_codec.hpp`
  for the client-side encode-request/decode-response split;
- `/home/msi/abacus_workspace/abacus/src/aorfix_onixs_fix/aorfix_onixs_fix/acceptor_codec.hpp`
  for the server-side decode-request/encode-response split;
- `/home/msi/abacus_workspace/abacus/src/aorfix_onixs_fix/src/codecs/abacus/`
  and `test/aorfix_onixs_fix/src/test_abacus_codec.cpp` for concrete codec
  implementation and round-trip test shape.

### Market Data Codecs

Add a concrete market-data JSON codec:

- client-side codec:
  - decodes `security_definition`, `security_status`, `execution_summary`,
    `trade`, and `mbo_book_update` messages.
- server-side codec:
  - encodes those market-data messages from typed domain events.

Market data stays separate from order entry. Do not put order-entry responses
and market-data events into one shared message variant unless a boundary needs
an envelope that can carry both streams.

Use `/home/msi/abacus_workspace/abacus/src/macuco/macuco/market_data.hpp` and
`/home/msi/abacus_workspace/abacus/src/macuco/macuco/bumdf_adapter.hpp` only as
reference material for UMDF-shaped naming and normalization boundaries. Abacus
market data is not yet properly normalized and modularized, so do not copy its
module shape into this repo.

### Migration Shape

- Add `src/lab/lab/json.hpp` and its tests before replacing codecs.
- Add JSON codecs next to the owning domains:
  - order-entry JSON codec under `src/order_entry/`;
  - order-client wrapper under `src/order_client/`;
  - market-data JSON codec under `src/market_data/`.
- Replace CSV decoder and encoder wiring in `server`, `client`, and examples.
- Remove or deprecate CSV codecs once JSON integration tests cover the same
  scenario behavior.
- Update README-level diagrams and command examples so the runtime pipeline says
  JSON decoder/encoder and examples use JSONL.
- Add or update an ADR for the project wire format decision.

## Test Plan

- After phase 1:
  - `./build.sh debug`
  - `./build.sh release server`
  - focused search for legacy branding, `submission/` paths, and assessment
    vocabulary in durable project surfaces
- After phase 2:
  - `./build.sh debug`
  - confirm no Docker wrapper docs remain
- After phase 3:
  - unit tests for client CSV encoding
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
  - unit tests for `lab::json` strong type, optional, defaulted field, enum,
    variant, buffer parse, and JSONL helpers
  - order-entry client codec tests for request encoding and response decoding
  - order-entry server codec tests for request decoding and response encoding
  - market-data server codec tests for event encoding
  - market-data client codec tests for event decoding
  - JSON round-trip or approval tests for representative messages from every
    order-entry and market-data variant
  - integration test: start `server`, send JSONL commands with `client`, compare
    JSONL order-entry responses and market-data output

## Assumptions

- The deleted `work-in-progress/index-skill.md` shown in `git status` is
  user-owned and should not be restored unless explicitly requested.
- `work-in-progress/` remains ephemeral; the reframe plan can live there
  during execution but should not be linked from durable docs.
- CSV remains an implementation detail only until phase 5 replaces it with JSON.
