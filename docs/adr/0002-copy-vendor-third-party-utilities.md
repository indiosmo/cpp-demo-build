# 2. Copy-vendor third-party utility headers

**Status:** accepted

**Date:** 2026-05-14

**Companion:** comparative scoring across copy-vendor, git submodules, CMake
`FetchContent`, and `ExternalProject` lives in
[`0002-copy-vendor-third-party-utilities-matrix.html`](0002-copy-vendor-third-party-utilities-matrix.html).

## Context and Problem Statement

`matching-engine-lab` uses small header-only utilities for strong typedefs,
fixed-capacity callbacks, expected-style value-or-error APIs, and SPSC queues.
These utilities support the local C++ design rules without turning the project
into a dependency-management showcase.

The repository should be easy to clone, configure, audit, and build in normal
local environments, including restricted or intermittent network environments.
The choice is how third-party utility headers reach the build: copy the sources
into the tree, track them as submodules, fetch them with CMake, or wrap them in
an external superproject.

## Decision Drivers

- A fresh checkout should build against committed project utility sources.
- The repository should carry the exact third-party bytes it builds against.
- Setup cost should match the size of the dependencies: a few header-only
  libraries with no upstream build systems.
- License texts and origin metadata should travel with the copied headers.
- CMake integration should look like the rest of the project: one `INTERFACE`
  target per utility and one project-facing alias header where the dependency is
  used through `lab::` vocabulary.
- Updating a utility should be explicit and auditable.

## Considered Options

1. **Copy-vendor.** Drop each upstream's public headers under
   `vendor/<name>/upstream/`; commit origin, version, selected path, and license
   metadata alongside them.
2. **Git submodules.** Track each upstream as a submodule pinned by SHA.
3. **CMake `FetchContent`.** Declare each dependency in CMake and fetch it at
   configure time.
4. **`ExternalProject` superproject.** Use CMake external projects to download,
   configure, build, and expose dependencies to consumers.

## Decision Outcome

Chosen option: **copy-vendor**.

The dependencies are small enough that committing their headers is cheaper than
maintaining a fetching layer. Copy-vendor makes the checkout self-contained,
keeps CMake simple, and leaves a clear audit trail from the copied bytes back to
the upstream commit.

Submodules are rejected because they add a second checkout step and make cold
clones easier to misconfigure. `FetchContent` is a good fit for larger projects
with an update cadence, but each configure-time fetch adds network and cache
state to a project that benefits from deterministic local setup. `ExternalProject`
is useful for libraries with real build/install machinery; it is too heavy for
header-only utility headers.

### Mechanics

Per dependency:

1. Clone the upstream into a scratch directory and choose a tag or commit SHA.
2. Copy the required public headers into `vendor/<name>/upstream/`.
3. Keep the upstream license text with the copied bytes.
4. Write `ORIGIN.txt`, `VERSION.txt`, and `PATHS.txt` under `vendor/<name>/`.
5. Declare one `INTERFACE` library for the dependency in the vendor tree.
6. Re-export the project-facing vocabulary through a `lab::` header under
   `src/lab/lab/` when domain code should not include the upstream header
   directly.

### Consequences

- Fresh local builds do not need network access for project utility sources.
- Reproducibility is direct: the exact bytes used by the build are committed.
- License review is local because copied headers and license texts live
  together.
- Updates are manual. Bumping a utility means recopying selected headers,
  updating metadata, and reviewing the diff.
- Repository size grows by the vendored headers. The accepted cost is small for
  the utility set used here.

### Confirmation

The decision is in effect when:

- Each vendored utility lives under `vendor/<name>/upstream/` with its license
  text and metadata files.
- The vendor tree declares one `INTERFACE` target per utility.
- Project code consumes vendored utilities through `lab::` wrappers or aliases
  unless the upstream type is intentionally part of that module's local
  implementation.
- The project CMake files do not use `FetchContent`, `ExternalProject_Add`, or
  submodule paths for these utility headers.

## Pros and Cons of the Options

### Copy-vendor

- Good, because the dependency sources are in the checkout.
- Good, because the exact bytes are reviewable and reproducible.
- Good, because CMake integration is one `INTERFACE` target per utility.
- Good, because license files can be kept beside the copied sources.
- Bad, because updates are manual recopy operations.
- Bad, because third-party bytes increase repository size.

### Git submodules

- Good, because the upstream SHA is explicit.
- Good, because updates are clean gitlink changes.
- Bad, because cold-clone setup needs `--recurse-submodules` or a follow-up
  `submodule update --init`.
- Bad, because source review spans multiple repositories.

### CMake `FetchContent`

- Good, because the repository carries only CMake declarations.
- Good, because updates can be one SHA change.
- Bad, because first configure needs network and cache state.
- Bad, because each dependency needs careful CMake flags to avoid broadening
  warning, install, or build surfaces.

### `ExternalProject` superproject

- Good, because it handles dependencies with their own build systems.
- Bad, because the project utilities are header-only and do not need external
  configure/build/install steps.
- Bad, because it adds dependency-edge and exported-include plumbing for little
  benefit.

## More Information

- [`DEPENDENCIES.md`](../../DEPENDENCIES.md) -- utility rationale and update
  procedure.
- [`0002-copy-vendor-third-party-utilities-matrix.html`](0002-copy-vendor-third-party-utilities-matrix.html)
  -- comparative scoring across the dependency-management options.
- [ADR 0001](0001-cpp-project-layout.md) -- the root-level layout that contains
  the vendor tree.
