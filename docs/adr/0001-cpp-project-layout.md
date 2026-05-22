# 1. Organize the C++ project tree for production-style growth

**Status:** accepted

**Date:** 2026-05-13

**Companion:** comparative scoring for the layout choice and its sub-decisions (tooling and build configuration location, Docker pipeline handling, `project()` placement) lives in [`0001-cpp-project-layout-matrix.html`](0001-cpp-project-layout-matrix.html).

## Context and Problem Statement

This project is a small interview submission, but the brief calls for organizing it the way a production codebase would be organized. The supplied sample arrives with a single `submission/src/main.cpp` and a Docker pipeline that compiles it -- no library separation, no tests, no benchmarks, no shared CMake conventions. Any structure beyond "compile one source file" is a deliberate choice.

The goal is a layout that stays comfortable as the number of libraries and applications grows from one of each to many, without forcing a structural reorganization later.

The Docker pipeline that the grader runs copies only `submission/` and builds from there. That constraint shapes where the CMake entry points live, but it should not dictate how the source tree inside `submission/` is organized.

## Decision Drivers

- The layout should scale to multiple libraries and multiple applications without structural changes. Adding a new module should be additive, not require moving existing files.
- Tooling that operates on a class of files (linters, formatters, coverage scopes) should be able to express its target as a single top-level path rather than per-module globs.
- CMake plumbing for unit-test and benchmark dependencies should be declared once, not repeated per module.
- Public headers should be includable with a stable, conflict-free path, and the same layout should map cleanly onto an install prefix if the library is ever packaged.
- Applications should be testable, not only runnable. The executable target should be a thin wrapper rather than the home of the application logic.
- The Docker entry point must continue to work, copying only `submission/`.

## Considered Options

1. **Keep the flat single-target layout (status quo).** Leave `submission/CMakeLists.txt` as a single `project()` plus one `add_executable(...)` over `src/main.cpp`. No libraries, no test or benchmark trees, no shared CMake conventions.
2. **Module-colocated tests and benchmarks.** Declare `submission/` as the project root and standardize each module as `<lib>/{src,test,benchmarks}/...`. Each module owns its test and benchmark sources next to the code they exercise, and declares its own test and benchmark dependencies.
3. **Aggregated test and benchmark trees.** Declare `submission/` as the project root and split the tree into three sibling roots -- `src/` for libraries and applications, `test/` for unit tests organized by module, `benchmarks/` for microbenchmarks organized by module.

## Decision Outcome

Chosen option: **aggregated layout**.

The aggregated tree gives three properties that module-colocation cannot. There is a single declaration site for unit-test and benchmark dependencies, so the Catch2 and Google Benchmark `find_package` dance happens once each rather than once per module. The three sibling roots are stable top-level paths that tooling can target directly, without filtering test and benchmark code out of source globs. And the whole test subtree or benchmark subtree can be gated on or off by a single CMake option at the root of its tree, without per-module conditionals.

A consequence we accepted: changing a single module now touches up to three sibling directories (`src/<lib>/`, `test/<lib>/`, `benchmarks/<lib>/`) instead of one. IDE navigation absorbs this trivially, and it is the same shape that codebases of this style settle on once they reach a handful of modules.

### Resulting shape

```
sample-cpp/                          (workspace root)
  CMakeLists.txt                     (project, options, CompilerFlags, CTest, delegates into submission/)
  CMakePresets.json
  cmake/                             (shared CMake modules)
  .clangd, .clang-format             (workspace-rooted so IDEs and tooling find them)
  build.sh
  test/                              (black-box integration harness, orthogonal to unit tests)
  submission/                        (C++ project root; also the Docker copy boundary)
    CMakeLists.txt
    src/
      <lib>/
        <lib>/*.hpp                  (public headers; stuttering directory enables <lib>/foo.hpp includes)
        src/*.cpp                    (implementations)
        CMakeLists.txt
      <app>/
        <app_lib>/                   (application library, when the app is non-trivial)
        src/main.cpp                 (thin executable wrapper)
        CMakeLists.txt
    test/
      CMakeLists.txt                 (one find_package(Catch2) for the whole tree)
      <module>/
        *.cpp
        CMakeLists.txt               (add_executable plus catch_discover_tests)
    benchmarks/
      CMakeLists.txt                 (one find_package(benchmark) for the whole tree)
      <module>/
        *.cpp
        CMakeLists.txt               (add_executable)
```

Workspace-root tooling configs stay at the repository root because that is where IDEs and editor integrations look for them. The C++ project root inside `submission/` is what the build system and the Docker pipeline both treat as the project; the workspace `CMakeLists.txt` delegates into it with `add_subdirectory(submission)`.

### Library and application conventions

Each library lives under `src/<lib>/`, with public headers in a stuttering `<lib>/<lib>/` subdirectory and implementations in `<lib>/src/`. The stutter is deliberate. It means consumers write `#include <foo/bar.hpp>` without risk of header-name collisions across modules, and the public header tree maps directly to an install prefix (`include/foo/...`) if the library is ever packaged. This is the same shape that Boost uses.

Applications follow a "library plus thin executable" pattern when they are non-trivial. The application logic lives in an application-specific library under `src/<app>/<app_lib>/`, and the executable target is a small `main.cpp` that wires the library together and acts as the process entry point. This keeps the application logic unit-testable with the same tooling as any other library. A trivial application that is genuinely just a `main.cpp` may skip the library split, but the expectation as the application grows is to factor logic out into an app-specific library before the executable accumulates testable behavior.

### Consequences

- One `find_package` declaration per dependency at each tree root serves every module beneath it. Per-module CMakeLists for tests and benchmarks shrink to the executable definition.
- Linters, formatters, and coverage scopes can target `submission/src/` to scope themselves to production code without globbing around interleaved test or benchmark trees.
- A single CMake option gates each subtree end-to-end. Disabling tests or benchmarks skips a whole subtree without per-module conditionals.
- Adding a new library is additive: a new `src/<lib>/` directory with a `CMakeLists.txt` and headers in `<lib>/<lib>/`, optionally mirrored under `test/<lib>/` and `benchmarks/<lib>/`. No existing files move.
- The header tree is install-ready: copying `src/<lib>/<lib>/` to a system include prefix produces the same `#include <lib/foo.hpp>` semantics consumers already use.
- Application logic is unit-testable through the application library; the executable target stays small enough to be uninteresting to test directly.
- `submission/CMakeLists.txt` remains dual-mode (`if(NOT PROJECT_NAME)` branch) because Docker copies only `submission/`. The duplication is one `project()` call -- structurally required by the Docker boundary, not a code smell.
- A module's source, tests, and benchmarks span up to three sibling directories rather than one. The cost was accepted because the tooling-scope and CMake-dryness wins are larger and the navigation cost is absorbed by the editor.
- The workspace-root `CMakeLists.txt` is load-bearing: it owns shared options, the `cmake/` module path, the `CompilerFlags` include, and `include(CTest)`. It is not a candidate for removal.

### Confirmation

The decision is in effect when:

- Every C++ source file under `submission/` sits beneath exactly one of `src/`, `test/`, or `benchmarks/`.
- `find_package(Catch2)` appears exactly once under `submission/test/`, and `find_package(benchmark)` appears exactly once under `submission/benchmarks/`.
- Each library under `src/` carries its public headers in a `<lib>/<lib>/` subdirectory and its implementations in `<lib>/src/`.
- New non-trivial applications under `src/` factor their logic into an application library and keep `main.cpp` as a thin entry point.

## Pros and Cons of the Options

### Flat single-target layout (status quo)

- Good, because it is as simple as it gets: one `project()`, one `add_executable()`, no abstraction overhead.
- Good, because for a one-file sample the cognitive load is essentially zero -- a reader sees the whole build in seven lines of CMake.
- Bad, because the brief explicitly asks for production-style organization, and a single flat target signals "throwaway sample" rather than "codebase that will grow".
- Bad, because there is nowhere to put unit tests or benchmarks; adding either forces one of the structural choices below.
- Bad, because adding a second library or pulling logic out of `main.cpp` to make it testable triggers a full restructure rather than an additive change.

### Module-colocated tests and benchmarks

- Good, because all artifacts for a module are co-located, so a module change touches one directory.
- Good, because deleting a module is a single `rm -r src/<lib>/`.
- Bad, because every module repeats `find_package(Catch2)` and `find_package(benchmark)`; growth multiplies the duplication.
- Bad, because tooling cannot target "production code only" with a simple top-level path -- every script needs an exclusion for each module's nested test and benchmark trees.
- Bad, because gating tests or benchmarks across the project requires conditionals scattered through every module's CMakeLists.

### Aggregated test and benchmark trees (chosen)

- Good, because test and benchmark dependencies are declared once each, at the root of their respective trees.
- Good, because `src/`, `test/`, and `benchmarks/` are stable top-level paths for linters, formatters, coverage, and any future static-analysis tooling.
- Good, because each kind of artifact is gated by one CMake option at the top of its tree.
- Good, because the structure communicates the growth path for new modules without further conventions to remember.
- Bad, because a single module spans up to three sibling directories.
- Bad, because the aggregation pattern is visibly heavier than necessary at one library and one application; the layout pays off as the project grows.
