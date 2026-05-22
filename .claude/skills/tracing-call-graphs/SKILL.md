---
name: tracing-call-graphs
description: Trace call graphs and error propagation paths through matching-engine-lab C++ code.
---

# Tracing Call Graphs

Use this skill to produce an accurate call graph from a target function,
including error and exception paths. This repo uses templates, variants,
callbacks, and result macros, so a lexical call graph is not enough.

## Required Reading

- [`docs/lab-guidelines/design.md`](../../../docs/lab-guidelines/design.md)
  for result handling, runtime split, and callback wiring.
- [`docs/lab-guidelines/debugging.md`](../../../docs/lab-guidelines/debugging.md)
  for logging, error details, and tracing layers.
- [`references/tool-selection.md`](references/tool-selection.md) for LSP vs
  search tool choices.

## Workflow

1. Read the target function.
2. Identify the nearest error boundary: `try_handle_all`, `try_catch_all`,
   `try/catch`, or public boundary that consumes a result.
3. List fallible call sites:
   - `LAB_ASSIGN`, `LAB_CHECK`, or current LEAF wrapper macros;
   - direct calls returning `lab::result<T>` or current equivalent;
   - `lab::make_error`, current `make_leaf_error`, `boost::leaf::new_error`;
   - `throw`;
   - virtual calls or callbacks whose implementation may fail.
4. For each fallible call, resolve the concrete callee and recurse.
5. Stop at error creation, thrown exceptions, infallible functions, external
   opaque calls, or already-traced functions.
6. Produce a structured report.

## Resolution Rules

- For a member call, read the owning class header to find the member type.
- For abstract interfaces, find all concrete overrides and select the one
  reachable from the current composition.
- For templates, find the instantiation site.
- For variant visitors such as `lab::match` or `std::visit`, list every
  alternative in the variant and trace each overload.
- For callback fields, trace the composition site that assigns the callback.

Parallelize independent fallible call sites when the user explicitly asks for
subagents or parallel work.

## Error Site Classification

For each error site, record:

- file path and line number;
- error type or error code;
- trigger condition;
- call chain from the target function;
- whether the error is realistic, theoretically unreachable, or opaque;
- whether a caller wraps or replaces the original error.

## Report Template

```text
## Error Trace: <class>::<method>

File: <path>:<line>

<what the function does and where errors are handled>

### Call Site: <expression> (line N)

<call-chain tree>

| # | Error Type | Location | Condition |
|---|---|---|---|
| 1 | <type/code> | <path>:<line> | <runtime condition> |

### Summary

Total distinct error types: N

| Error Type | Count | Source |
|---|---:|---|
| <type/code> | N | <brief source> |

Notes:
- realistic vs unreachable errors
- opaque external calls
- error-boundary observations
```

## Patterns Requiring Manual Handling

### Variant Visitors

`lab::match`, current variant helpers, and `std::visit` instantiate the visitor
for each alternative. Read the variant typedef and trace every branch.

### Result Macros

Result macros propagate errors; they do not create them. Trace into the
expression argument to find the creation site. If a lab wrapper attaches local
context, report the wrapper and note the original error.

### Runtime Callbacks

Callbacks assigned in the composition layer can hide cross-thread call edges.
Read the wiring shell to resolve `on_*` fields before concluding a stage has no
downstream calls.

### Matching Engine Containers

Book lookup, identity-index lookup, duplicate detection, and node allocation
often encode domain errors. Trace to the helper or container owner rather than
stopping at the map operation.

## Pitfalls

- Do not stop at the first level.
- Do not report graceful `std::nullopt` or silent drops as errors unless the
  boundary contract treats them as failures.
- Do not count propagated errors as new error sites.
- Do not ignore the error boundary; handled errors do not escape the function.
