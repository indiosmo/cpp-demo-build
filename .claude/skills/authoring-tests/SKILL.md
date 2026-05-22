---
name: authoring-tests
description: Author Catch2 tests for matching-engine-lab.
---

# Authoring Tests

Use this skill when writing or refactoring tests. The generic testing rules
live in [`docs/cpp-guidelines/cpp-testing-principles/`](../../../docs/cpp-guidelines/cpp-testing-principles/).
The lab-specific mapping lives in
[`docs/lab-guidelines/testing.md`](../../../docs/lab-guidelines/testing.md).

The project is being reframed as a portfolio demonstrator. Tests should serve
both as verification and as readable examples of matching-engine behavior.

## Required Reading

Always read:

1. [`philosophy.md`](../../../docs/cpp-guidelines/cpp-testing-principles/philosophy.md)
   -- behavior over implementation, component vs integration scope.
2. [`docs/lab-guidelines/testing.md`](../../../docs/lab-guidelines/testing.md)
   -- local test layout, result helpers, fixtures, scenario tests, and
   callback wiring.

Load on demand:

| When the test needs | Open |
|---|---|
| `TEST_CASE` / `SECTION` / `GENERATE` / table-driven layout | [`test-patterns.md`](../../../docs/cpp-guidelines/cpp-testing-principles/test-patterns.md) |
| factories, fixtures, probes, RAII providers | [`test-helpers.md`](../../../docs/cpp-guidelines/cpp-testing-principles/test-helpers.md) |
| success and failure paths on result-style code | [`error-path-testing.md`](../../../docs/cpp-guidelines/cpp-testing-principles/error-path-testing.md) |
| Catch2 discovery, tags, `REQUIRE` vs `CHECK` | [`catch2-conventions.md`](../../../docs/cpp-guidelines/cpp-testing-principles/catch2-conventions.md) |
| async waits without sleeps | [`condition-based-waiting.md`](../../../docs/cpp-guidelines/cpp-testing-principles/condition-based-waiting.md) |
| client/server or cross-component tests | [`references/integration-testing.md`](references/integration-testing.md) |

## Workflow

### 1. Read For Intent

State the contract in your own words before writing assertions:
preconditions, postconditions, invariants, edge cases, and error conditions.
If you cannot state it, keep reading or ask the user.

Assert independently known expected values. Do not derive expected values by
copying logic from the implementation under test.

For matching-engine behavior, prefer the observable contract in
[`docs/engine-specs.md`](../../../docs/engine-specs.md) over internal helper
behavior.

### 2. Stay In Test Scope

Do not modify production code just to make a test easier. If a public surface
is missing, stop and report what you need.

Use production factories from the owning module and test-only factories when
they exist. If a helper is only useful in one file, keep it local.

### 3. Encode Bugs Honestly

If implementation contradicts the spec, comments, or domain logic, write the
test against the correct expected behavior and report the discrepancy. Do not
adjust expectations to match behavior you believe is wrong.

### 4. Project Gotchas

- **Callback wiring.** Objects with `lab::inplace_function` callback fields
  need every callback assigned before teardown. During the transition, current
  names may still use the legacy namespace, but the rule is identical.
- **Result assertions.** Use lab result-aware helpers once they land:
  `LAB_REQUIRE_RESULT`, `lab::testing::require_error<E>`, and
  `lab::testing::capture_error_code`. Until then, use the current local LEAF
  helpers consistently.
- **Scenario fixtures.** Old assessment scenarios should become demonstration
  fixtures. Avoid writing new tests that depend on hidden-test framing.
- **Runtime integration.** Tests that boot threads, sockets, or the future
  client/server pair are integration tests. Keep detailed matching semantics in
  synchronous unit tests where possible.

### 5. Test File Shape

```cpp
#include <catch2/catch_test_macros.hpp>

#include "<module>/<component>.hpp"
#include "<module>/testing/factories.hpp"

namespace {

namespace ft = <module>::testing;

} // namespace

TEST_CASE("<component> - <scenario> - <outcome>", "[<module>][<component>]")
{
  // arrange / act / assert
}
```

Target layout:

```text
test/<module>/src/test_<component>.cpp
```

Current files may still live under `submission/test/` until the root layout
reframe lands.

### 6. Review The Batch

After a batch of tests, review for:

- duplicated arrange/act blocks;
- cases that differ only by input values and should use `GENERATE` or a table;
- repeated assertion sequences that deserve a helper;
- setup shared by 80% or more of multiple outcomes and better represented by
  `SECTION`.

## Done Criteria

- Assertions encode independently known expected behavior.
- Every callback on objects under test is wired.
- Process-wide state mutations have RAII cleanup.
- Fallible code has success and failure-path coverage.
- Mutating fallible functions have rollback assertions where applicable.
- Bugs discovered while reading are reported, not silently encoded as expected.
- The test reads as a useful behavior example for the portfolio project.
