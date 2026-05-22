---
name: systematic-debugging
description: Debug bugs and test failures systematically before proposing fixes.
---

# Systematic Debugging

Use this skill for bugs, test failures, flakes, crashes, and unexpected
behavior. The method lives in
[`docs/cpp-guidelines/cpp-debugging-principles/`](../../../docs/cpp-guidelines/cpp-debugging-principles/).
The lab-specific tools live in
[`docs/lab-guidelines/debugging.md`](../../../docs/lab-guidelines/debugging.md).

## Required Reading

1. [`cpp-debugging-principles/README.md`](../../../docs/cpp-guidelines/cpp-debugging-principles/README.md)
   -- root cause, pattern, hypothesis, fix.
2. [`docs/lab-guidelines/debugging.md`](../../../docs/lab-guidelines/debugging.md)
   -- logging, assertions, sanitizer presets, runtime tracing, and
   performance evidence.

## The Rule

```text
NO FIXES WITHOUT ROOT CAUSE INVESTIGATION FIRST
```

If root cause is not known, the next action is evidence gathering. Do not
change production code to test a guess unless the change is explicitly framed
as a minimal experiment and will be reverted or turned into the real fix after
confirmation.

## Red Flags

Stop and return to investigation if you catch yourself thinking:

- "Just try changing X and see if it works."
- "It is probably X."
- "Add multiple changes, run tests."
- "Skip the test; I will manually verify."
- "One more fix attempt."
- "Pattern says X but I will adapt it differently."
- You are proposing solutions before tracing data flow.

If two fix attempts fail, stop stacking changes. Present the evidence and
re-open the investigation. A third failed fix means the pattern or design may
be wrong for the requirement.

## Debugging Loop

1. **Root cause investigation.** Reproduce the failure, read the full error,
   trace data backward, and identify the failing boundary.
2. **Pattern analysis.** Find working examples in the repo and list concrete
   differences.
3. **Hypothesis.** State one specific hypothesis and test only that.
4. **Implementation.** Add or update the smallest test that captures the bug,
   then make one production change.

## Tooling

- [`scripts/find-polluter.sh`](scripts/find-polluter.sh) -- find a test that
  creates unwanted file or process state.
- [`scripts/run-with-sanitizers.sh`](scripts/run-with-sanitizers.sh) -- rebuild
  with `asan` or `tsan` and rerun tests.
- `LAB_LOG_DEBUG` / current logging macros -- add temporary boundary logs when
  tracing data flow.
- `lab::error::full_details()` / current `full_details()` -- render structured
  result failures at boundaries.
- Benchmark and performance scripts -- use only when the symptom is
  performance-sensitive or load-dependent.

## Matching-Engine Tracing Lens

Separate failures by layer:

- UDP receive and runtime wiring;
- order command decode;
- session dispatch and rejection;
- matching-engine book mutation;
- market-data event emission;
- CSV encode and stdout publish;
- future client command encode and UDP send.

Do not debug all layers through an end-to-end process test if a synchronous
unit test can pin the failing layer.

## After The Fix

- The failing test now passes.
- The original reproduction no longer fails.
- The narrow relevant suite passes.
- A broader build or sanitizer pass runs when the bug involved shared code,
  memory, lifetime, or threading.
- The fix preserves the portfolio goal: clearer behavior, cleaner workflow,
  and no new assessment-era framing.
