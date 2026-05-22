# 1. Organize the C++ project tree as root-level aggregated modules

**Status:** accepted

**Date:** 2026-05-13

**Companion:** comparative scoring for the layout choice lives in
[`0001-cpp-project-layout-matrix.html`](0001-cpp-project-layout-matrix.html).

## Context and Problem Statement

`matching-engine-lab` is a C++26 portfolio project with multiple libraries,
applications, tests, and vendored utility headers. It needs the shape of a
normal C++ repository: source and build entry points at the repository root,
with modules grouped by project role.

The layout should stay comfortable as the project grows from the current server
and core domains to a client library, more unit tests, and a future benchmark
suite.

## Decision Drivers

- The repository root should be the CMake project root.
- The layout should scale to multiple libraries and applications without moving
  existing files.
- Tooling should be able to target production code and tests by top-level path.
- Unit-test dependencies should be declared once for the test tree.
- Public headers should use stable include paths that can map onto an install
  prefix.
- Non-trivial applications should keep process entry points thin and put
  testable behavior in application libraries.
- Third-party utility code should live in a dedicated root-level vendor tree.

## Considered Options

1. **Flat single-target layout.** Keep one executable target with all logic
   behind `main.cpp`.
2. **Module-colocated tests.** Put each module under `src/<module>/` and
   place that module's tests beneath it.
3. **Root-level aggregated trees.** Use sibling top-level trees: `src/` for
   libraries and applications, `test/` for unit tests, and `vendor/` for
   copied third-party utility sources. Reintroduce microbenchmarks under a
   root-level `benchmarks/` tree when the benchmark suite returns.

## Decision Outcome

Chosen option: **root-level aggregated trees**.

The aggregated shape gives the project stable top-level paths for each class of
work. Formatters, static analysis, coverage, and test discovery can point at
one tree without per-module exclusions. Catch2 is found once at the test root,
while each module keeps a small local `CMakeLists.txt` for its own targets.

The root CMake project owns shared options, compiler flags, CTest setup, and
module delegation. `src/` holds production modules and applications. `test/`
mirrors module names where focused coverage exists.

### Resulting Shape

```text
matching-engine-lab/
  CMakeLists.txt
  CMakePresets.json
  cmake/
  build.sh
  docs/
  examples/
  src/
    lab/
      lab/*.hpp
      libs/<component>/
      src/*.cpp
    order_entry/
      order_entry/*.hpp
      src/*.cpp
    order_client/
      order_client/*.hpp
      src/*.cpp
    matching_engine/
      matching_engine/*.hpp
      src/*.cpp
    market_data/
      market_data/*.hpp
      src/*.cpp
    server/
      server/*.hpp
      src/main.cpp
    client/
      src/main.cpp
  test/
    <module>/
  vendor/
    <dependency>/
```

### Library and Application Conventions

Each library lives under `src/<library>/`, with public headers in a stuttering
`<library>/<library>/` subdirectory and implementations in `<library>/src/`.
Consumers include headers as `<library>/foo.hpp`, and the same header tree can
map to `include/<library>/...` if the library is packaged.

Applications follow a library-plus-thin-executable pattern once they contain
testable behavior. The application library owns configuration, composition, and
runtime orchestration; `main.cpp` translates process concerns into that library.

### Consequences

- One dependency declaration for the test tree serves every module beneath it.
- Linters, formatters, and coverage tools can target `src/` and `test/`
  directly.
- Adding a library is additive: create `src/<library>/`, then mirror it in
  `test/<library>/` when coverage is needed.
- A module's source and tests may span sibling trees. The trade is accepted
  because the build and tooling boundaries are clearer.
- The repository root is the only CMake project root.

### Confirmation

The decision is in effect when:

- Production code lives under `src/`.
- Unit tests live under `test/`.
- Third-party copied utility sources live under `vendor/`.
- `CMakeLists.txt` at the repository root owns the project declaration, shared
  options, compiler flags, and CTest setup.
- Each non-trivial application has an application library and a thin executable
  entry point.

## Pros and Cons of the Options

### Flat single-target layout

- Good, because it is simple for a one-file prototype.
- Bad, because it gives the project no durable place for unit tests, reusable
  domain libraries, or alternate applications.
- Bad, because extracting testable behavior later forces a structural rewrite.

### Module-colocated tests

- Good, because all artifacts for one module sit under one directory.
- Good, because deleting a module is mostly local.
- Bad, because test dependencies are repeated per module.
- Bad, because production-only tooling needs exclusions for each nested test
  tree.
- Bad, because project-wide test gating requires conditionals in every module.

### Root-level aggregated trees

- Good, because `src/`, `test/`, and `vendor/` are stable repository-level
  boundaries.
- Good, because test dependencies are declared once for the tree.
- Good, because adding modules and applications is additive.
- Good, because the layout matches the portfolio direction: ordinary clone,
  configure, build, test, and inspect workflows.
- Bad, because a single module may touch up to three sibling trees.
