# Debugging

Lab-side mapping for
[`cpp-guidelines/cpp-debugging-principles/`](../cpp-guidelines/cpp-debugging-principles/).
The shared guide owns the investigation loop. This file names the local
logging, assertion, sanitizer, and trace surfaces that produce evidence for
that loop.

Symbol-to-header lookups live in the
[README placeholder table](README.md#placeholder-mapping).

## Logging

The target logging family is:

```text
LAB_LOG_TRACE  LAB_LOG_DEBUG  LAB_LOG_INFO
LAB_LOG_WARN   LAB_LOG_ERROR  LAB_LOG_CRITICAL
```

Current anchors are the `LAB_LOG_*` macros in
[`log.hpp`](../../src/lab/lab/log.hpp). Diagnostics must stay
off stdout because stdout is the market-data stream. Keep logs on stderr or a
configured diagnostic sink.

The market-data publisher uses an spdlog stdout sink for records, not for
diagnostics. Preserve that separation when renaming or replacing the sink.

## Assertions and preconditions

The target assert macro is `LAB_ASSERT`. Current anchor:
[`assert.hpp`](../../src/lab/lab/assert.hpp).

Use assertions for internal invariants the parser or type system has already
established. Use result errors at runtime boundaries where the caller can
respond, such as configuration, socket setup, and client input.

## Rich result details

Current `lab::error::full_details()` is the local evidence surface for
LEAF-carried failures. The lab target should render location, error code,
payload `what()`, and an optional stack trace in one block when richer error
tooling lands.

Until then, boundary handlers should log `full_details()` for unexpected
errors and structured payload fields for expected domain drops.

## Sanitizer presets

Current presets live in [`CMakePresets.json`](../../CMakePresets.json):

| Preset | Current option | Lab target |
|---|---|---|
| `asan` | `LAB_ASAN=ON` | `LAB_ASAN=ON` |
| `tsan` | `LAB_TSAN=ON` | `LAB_TSAN=ON` |
| `clang` | `LAB_ASAN=ON` with Clang | keep as Clang+ASan |

Drive them through `./build.sh <preset>`. The helper script sources sanitizer
environment defaults from [`scripts/setenv.sh`](../../scripts/setenv.sh).

ThreadSanitizer reports need root-cause triage before suppression. Add
suppressions only after the report is understood and documented.

## Build and static checks

Compiler warnings and sanitizer flags are centralized in
[`cmake/CompilerFlags.cmake`](../../cmake/CompilerFlags.cmake). Formatting is
handled by [`scripts/format.sh`](../../scripts/format.sh) and the pre-commit
hook installed by
[`scripts/install_precommit_hooks.sh`](../../scripts/install_precommit_hooks.sh).

If static-analysis scripts are added, map them here rather than
burying their invocation in README prose.

## Runtime tracing

The server composition layer is the right place to trace cross-thread flow:
UDP receive, decode, session dispatch, engine event emission, encode, and
publish. Current anchor:
[`application.cpp`](../../src/server/src/application.cpp).

For matching defects, trace from the emitted market-data record backward to the
matching-engine request and then into the book operation. Avoid adding
per-order hot-path logs as a first move; unit tests and focused counters are
usually less perturbing.

## Client/server failures

Debug client/server failures by separating:

- command encoding in the client library;
- socket send/receive at the runtime edge;
- decoder behavior in `order_entry`;
- matching behavior in `matching_engine`;
- publisher behavior in `market_data`.

Each layer should have a unit-level reproduction before the integration test
is used as the only signal.
