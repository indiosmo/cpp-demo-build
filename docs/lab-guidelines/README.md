# Lab C++ guidelines

Project-specific mapping layer on top of the shared
[`cpp-guidelines/`](../cpp-guidelines/) submodule. The shared guides describe
generic C++ principles in a `lib::` placeholder namespace; the table below
names the `matching-engine-lab` symbol, helper, macro, or example that should
realise each placeholder as the reframe lands.

This folder is written for the target project identity. Some links still point
at the current implementation anchors under `src/lab/`; those
anchors should move to `src/lab/` during the full rename. New helper work
should follow the `lab::` names here.

## Reading order

| File | Purpose |
|---|---|
| [`design.md`](design.md) | Maps design principles into the matching engine, runtime shell, and lab utility layer. |
| [`testing.md`](testing.md) | Maps testing principles into Catch2 targets, test data, and scenario-shaped tests. |
| [`debugging.md`](debugging.md) | Maps debugging principles into logging, assertions, sanitizers, benchmarks, and trace points. |
| [`agent-examples.md`](agent-examples.md) | Project-specific good/bad pairs for agent edits; load sections on demand. |

## Placeholder mapping

The shared guides use `lib::` for in-house utilities. Substitute the rows
below.

| Shared placeholder | Lab symbol | Current anchor / target note |
|---|---|---|
| `lib::result<T>` | `lab::result<T>` | [`result.hpp`](../../src/lab/lab/result.hpp) |
| `BOOST_LEAF_ASSIGN` | `LAB_ASSIGN(target, expr[, context...])` | Planned lab helper; current code uses [`LAB_LEAF_CHECK`](../../src/lab/lab/result.hpp). |
| `BOOST_LEAF_CHECK` | `LAB_CHECK(expr[, context...])` | Planned lab helper; current code uses [`LAB_LEAF_CHECK`](../../src/lab/lab/result.hpp). |
| `lib::strong_type<T, Tag>` | `lab::strong_type<T, Tag, Skills...>` | [`strong_type.hpp`](../../src/lab/lab/strong_type.hpp) |
| `lib::fixed_string<N>` | `lab::fixed_string<N>` | [`fixed_string.hpp`](../../src/lab/lab/fixed_string.hpp) |
| `lib::inplace_function<Sig, N>` | `lab::inplace_function<Sig, N>` | [`inplace_function.hpp`](../../src/lab/lab/inplace_function.hpp) |
| `lib::scope_exit` | `lab::scope_exit` / `lab::scope_guard` | Planned with the incoming lab helper set. |
| `lib::match` / `lib::match_partial` | `lab::match` / `lab::match_partial` | [`variant.hpp`](../../src/lab/lab/variant.hpp); `match_partial` is planned. |
| `lib::error` | `lab::error` | [`error.hpp`](../../src/lab/lab/error.hpp) |
| `lib::new_error` / `lib::make_error` | `lab::make_error(...)` | Current equivalent is [`make_leaf_error`](../../src/lab/lab/error.hpp); align naming during the rename. |
| `lib::match_error<E>` | `lab::match_error<E>` / `lab::match_errors<E...>` | [`error.hpp`](../../src/lab/lab/error.hpp) |
| catch-all handler macro | `LAB_CATCH_AND_LOG(code_block)` | Planned lab helper; current catch-all pieces live in [`result.hpp`](../../src/lab/lab/result.hpp). |
| logger singleton | `lab::logger` | [`log.hpp`](../../src/lab/lab/log.hpp) |
| logging macros | `LAB_LOG_TRACE` ... `LAB_LOG_CRITICAL` | Current equivalents are [`LAB_LOG_*`](../../src/lab/lab/log.hpp). |
| assert macro | `LAB_ASSERT(expr)` | Current equivalent is [`LAB_ASSERT`](../../src/lab/lab/assert.hpp). |
| event loop | `lab::event_loop` | [`event_loop.hpp`](../../src/lab/lab/event_loop.hpp) |
| SPSC queue | `lab::concurrent_queue<T>` | [`concurrent_queue.hpp`](../../src/lab/lab/concurrent_queue.hpp) |
| testing result unwrap | `LAB_REQUIRE_RESULT(target, expr)` | Current equivalent is [`LAB_REQUIRE_LEAF`](../../src/lab/lab/result.hpp). |
| per-domain `types` namespace | `<domain>::types` in `<domain>/types.hpp` | For example [`order_entry/types.hpp`](../../src/order_entry/order_entry/types.hpp). |
| per-domain error pair | `error_code.hpp` + `errors.hpp` | For example [`matching_engine/`](../../src/matching_engine/matching_engine/). |
| boundary result type | `std::expected<T, E>` when C++26 is available | Current backport anchor is [`expected.hpp`](../../src/lab/lab/expected.hpp). |

Absent or planned:

- `lab::find_element`, `lab::try_emplace_unique`, and related container
  helpers should come over with the lab helper set. Until then, keep local
  lookup helpers near the owning container.
- `lab::clock`, `lab::timer`, `lab::scoped_handle`, and stack-trace-rich
  `lab::error` are target-state utilities, not current surfaces.
- The project has no GUI surface; the Qt testing guide is not mapped here.

## Cross-cutting wiring summary

Five conventions should shape the reframe:

- Domain types and errors stay in the domain that owns them.
- Runtime code stays at the edge: UDP, threads, queues, stdout, and the future
  client app belong outside the synchronous matching core.
- Pipeline stages communicate through typed values and `lab::inplace_function`
  callbacks; application composition wires callbacks near the executable.
- Cross-thread hops use `lab::event_loop` and SPSC queues where the topology is
  one producer to one consumer.
- Helper naming and result ergonomics should follow this mapping, while
  matching-engine vocabulary stays in the domain modules.
