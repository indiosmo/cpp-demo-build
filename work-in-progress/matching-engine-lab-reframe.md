# Matching Engine Lab Portfolio Reframe Plan

## Summary

Reframe the repo from a Kraken take-home submission into
`matching-engine-lab`, a C++ portfolio demonstrator for a multi-threaded UDP
matching engine. Use the confirmed approach: full `kraken` to `lab` rename
for internal utility code and normal root project layout.

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
  - Utility namespace/library: `kraken` -> `lab`
  - CMake aliases: `kraken::kraken` -> `lab::core`
  - Build options/macros: `KRAKEN_*` -> `LAB_*`
  - Server executable: `kraken_submission` -> `matching_engine_lab_server`
  - Client executable: `matching_engine_lab_client`
  - Runtime namespace: `kraken_submission` -> `matching_engine_lab_server`
  - Thread names: `kraken-input`, `kraken-engine`, `kraken-output` ->
    `lab-input`, `lab-engine`, `lab-output`
- Keep domain names `order_routing`, `matching_engine`, and `market_data`;
  they are domain vocabulary, not employer branding.
- Rewrite durable docs so they describe a portfolio/demo project:
  - `README.md`, `DESIGN.md`, `DEPENDENCIES.md`, `DEVELOPING.md`, `INDEX.md`
  - `docs/*.md`, ADRs, per-library READMEs
  - Replace `EXERCISE.md` references with an internal protocol/spec document,
    likely `docs/protocol.md` or expanded `docs/engine-specs.md`.
- Delete interview/submission artifacts:
  - `EXERCISE.md`
  - `sample.md`
  - `submission.html`
  - generated assessment reports under `reports/`
  - take-home wording in comments, scripts, and docs
- Preserve the existing matching behavior and CSV protocol unless phase 3
  explicitly extends it.

## Phase 2: Docker Removal

- Delete grading-specific Docker machinery:
  - `Dockerfile`
  - `.dockerignore`
  - `run_submission.sh`
  - `run_local_submission.sh`
  - `test/run_tests.sh` once phase 3 replaces its role
- Update `README.md` and `DEVELOPING.md` to document regular local workflows:
  - `./build.sh`
  - direct `cmake --preset=debug`
  - `ctest --test-dir _build/debug --output-on-failure`
  - running `./_build/debug/matching_engine_lab_server`
- Remove Docker/grading/package assumptions from `DEPENDENCIES.md`; keep
  package inventory as developer setup context only.

## Phase 3: Runtime Refactor and Client

- Replace the black-box harness with a real UDP client library and app.
- Add `src/order_client/`:
  - public typed client API using `order_routing::new_order`,
    `cancel_order`, and `flush`
  - CSV encoding for outbound order commands
  - UDP sender backed by Boost.Asio
  - configurable endpoint, defaulting to `127.0.0.1:1234`
- Add `src/matching_engine_lab_client/` executable:
  - sends commands from a file or stdin
  - supports a simple CLI: `--host`, `--port`, `--input`
  - exits nonzero on file/socket/config errors
- Keep `matching_engine_lab_server` as the matching-engine server that listens
  on UDP and writes market data CSV to stdout.
- Convert old scenario files into non-grading examples or integration fixtures:
  - move useful CSV inputs under `examples/scenarios/` or
    `test/integration/fixtures/`
  - remove "expected even outputs" and assessment language
  - add integration tests that start `matching_engine_lab_server`, run
    `matching_engine_lab_client`, and compare stdout for representative
    scenarios.

## Test Plan

- After phase 1:
  - `./build.sh debug`
  - `./build.sh release matching_engine_lab_server`
  - `rg -n "Kraken|kraken|submission|interview|exercise|grader|grading|recruiter|bundle|kraken_submission|EXERCISE"`
- After phase 2:
  - `./build.sh debug`
  - confirm no Docker wrapper docs remain
- After phase 3:
  - unit tests for client CSV encoding
  - unit tests for client config validation
  - integration test: start `matching_engine_lab_server`, send fixture
    commands with `matching_engine_lab_client`, compare market-data output
  - sanitizer pass with `./build.sh asan`

## Assumptions

- The deleted `work-in-progress/index-skill.md` shown in `git status` is
  user-owned and should not be restored unless explicitly requested.
- `work-in-progress/` remains ephemeral; the reframe plan can live there
  during execution but should not be linked from durable docs.
- The CSV order and market-data protocol remains part of the demo, but it is
  documented as Matching Engine Lab's sample wire protocol rather than an
  interview requirement.
