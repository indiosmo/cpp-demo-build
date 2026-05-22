# 2. Copy-vendor third-party utilities into the submission tree

**Status:** accepted

**Date:** 2026-05-14

**Companion:** comparative scoring across the four dependency-management mechanisms (copy-vendor, git submodules, CMake `FetchContent`, `ExternalProject` superproject) lives in [`0002-copy-vendor-third-party-utilities-matrix.html`](0002-copy-vendor-third-party-utilities-matrix.html).

## Context and Problem Statement

The submission needs four small header-only utilities -- a strong-typedef helper, a fixed-capacity callable, an SPSC queue, and an `expected` vocabulary type -- to satisfy the project design principles (see [`DEPENDENCIES.md`](../../DEPENDENCIES.md) for the per-utility rationale). How those utilities reach the build is a separate question with several plausible answers: copy the sources into the tree, reference them as git submodules, fetch them at configure time with CMake `FetchContent`, or stand up an `ExternalProject` superproject.

Two constraints from the grading environment shape the choice. First, the graded harness runs `docker build` then `docker run` against the submission; the Dockerfile copies the `submission/` directory into the image and builds from there, and a transient network failure during that one-shot build is an outright loss. Second, delivery is by `git bundle` per [`EXERCISE.md`](../../EXERCISE.md), and a `git bundle` packs only the parent repo's history -- anything tracked outside that history (submodule contents, for instance) does not travel.

Network is in fact available during `docker build` (the existing Dockerfile already runs `apt-get` for the toolchain), but availability and reliability are different properties. The project has an interview-shaped horizon: dependencies are pinned once, there is no update cadence to amortise, and there is no benefit to optimising for the "swap a SHA monthly" workflow.

## Decision Drivers

- The graded `docker build` must not depend on a configure-time or build-time network fetch of project dependencies. A flaky GitHub or a stale mirror should not be able to fail the grade.
- The `git bundle` delivery format must carry the dependency sources. Anything outside the parent repo's history is lost.
- Setup cost should be proportional to the size of what we are bringing in: four header-only files with no upstream build systems.
- The CMake plumbing should stay close to what every other library in the tree already does (an `INTERFACE` target with `target_include_directories`), so that the cost of consuming a vendored utility is one `target_link_libraries` line.
- Reproducibility: a reader cloning the bundle a year from now should build the exact bytes the grader saw, without depending on whether the upstream URL still resolves.

## Considered Options

1. **Copy-vendor.** Drop each upstream's public headers into `submission/vendor/<name>/upstream/`; commit small `ORIGIN.txt` (upstream URL) and `VERSION.txt` (tag and commit SHA) files alongside, so the audit trail back to the upstream lives in the tree.
2. **Git submodules.** Each upstream lives at `submission/vendor/<name>/` as a submodule pinned by SHA.
3. **CMake `FetchContent`.** One `FetchContent_Declare` per dependency in the top-level CMake; sources fetched at configure time, pinned by `GIT_TAG`.
4. **`ExternalProject` superproject.** A custom CMake module orchestrates per-dependency `ExternalProject_Add` blocks with download / configure / build / install steps and dependency edges into the consumer targets.

## Decision Outcome

Chosen option: **copy-vendor**.

The two hard constraints -- offline-tolerant Docker grade and self-contained `git bundle` delivery -- both point at copy-vendor, and they are the only criteria in the comparison that can independently fail the submission. Every other option fails at least one of them. The wins the alternatives offer (smaller repo, easier SHA bumps) carry no weight on an interview timeline where dependencies are pinned once and never revisited.

Submodules are rejected because `git bundle` packs only the parent repo's history. Submodule gitlinks travel, contents do not -- the grader unbundles and gets dangling references. Fixing either end (host-side `submodule update --init` before bundling, or a Dockerfile rewrite that runs recursive checkout) adds more moving parts than the alternative removes. `FetchContent` is the option a longer-lived project would pick: bump-a-SHA updates are pleasant, and the build environment does have network. But every configure-time fetch is a transient-failure axis during a one-shot grade, and copy-vendor removes that axis entirely. `ExternalProject` is the right tool for libraries that bring their own build systems and need custom configure/install integration; it is the wrong tool for four header-only utilities.

### Mechanics

Per dependency:

1. `git clone --depth=1` the upstream into a scratch directory and pick a commit SHA.
2. Copy the public headers into `submission/vendor/<name>/upstream/` and keep the upstream `LICENSE` file alongside them, so the licensed bytes and the licence terms travel together.
3. Write two small text files at `submission/vendor/<name>/`: `ORIGIN.txt` with the upstream URL and `VERSION.txt` with the upstream tag and commit SHA. The pair is the audit trail -- a reader can match the vendored bytes to an upstream commit without git archaeology.
4. Declare one `add_library(<name> INTERFACE)` in `submission/vendor/CMakeLists.txt` with `target_include_directories` pointing at the vendored headers.
5. Add a `kraken::` re-export under `submission/src/kraken/kraken/<role>.hpp` per the alias rule in [`DEPENDENCIES.md`](../../DEPENDENCIES.md). Domain libraries depend on the alias, not the vendored header.

### Consequences

- The graded `docker build` makes no network calls for project dependencies. The only network surface left is the `apt-get` line that installs the toolchain, which is independent of this decision.
- The `git bundle` carries everything the build needs. A grader who unbundles, builds, and runs the Docker image has no dependency on external hosts being reachable.
- Per-dep setup is "clone once, copy headers, write `VERSION.txt`, add one CMake line" -- minutes of work, no per-dep boilerplate beyond that.
- Reproducibility is by construction: the exact bytes are committed. There is no SHA to re-verify against an upstream that may have moved or disappeared.
- Updates are manual: bumping a vendored library to a newer upstream is a re-copy. This is the cost we pay for "no fetch at build time", and it is the right side of the trade-off for a pin-once project.
- Repository size grows by the size of the vendored headers. Four header-only libraries together are well under a megabyte -- not a meaningful cost.
- The vendored headers are accompanied by their upstream `LICENSE` files. The MIT, BSL-1.0, and BSD-2-Clause licenses on the four upstreams all require this; copy-vendor makes it a tree-level invariant rather than a build-time concern.

### Confirmation

The decision is in effect when:

- Every utility in the `kraken::` alias set has its sources committed under `submission/vendor/<name>/upstream/`, alongside the upstream `LICENSE`. The parent `submission/vendor/<name>/` directory carries an `ORIGIN.txt` (upstream URL) and a `VERSION.txt` (upstream tag and commit SHA).
- `submission/vendor/CMakeLists.txt` declares one `INTERFACE` library per vendored utility, and no other CMake file under `submission/` references `FetchContent`, `ExternalProject_Add`, or a submodule path for these utilities.
- A fresh `git clone` of the submission builds and tests with no network access required after the toolchain is in place.

## Pros and Cons of the Options

### Copy-vendor (chosen)

- Good, because the sources are bytes in the tree by the time the Dockerfile `COPY`s; no fetch step can fail during the grade.
- Good, because files in the tree pack into a `git bundle` natively; the grader unbundles and has everything.
- Good, because per-dep setup is minimal: clone, copy headers, write `VERSION.txt`, add one `add_library(... INTERFACE)`.
- Good, because the exact bytes are committed, so reproducibility does not depend on the upstream URL still resolving.
- Good, because the CMake surface is one `INTERFACE` target per dep -- the same shape every other library in the tree already uses.
- Bad, because updating to a newer upstream is a manual re-copy rather than a SHA bump.
- Bad, because the source bytes live in the repo (a non-issue at the scale of four header-only libraries, but a real cost for larger upstreams).

### Git submodules

- Good, because the SHA is pinned by the gitlink with no source bytes in the parent repo.
- Good, because updates are a clean `git -C vendor/<name> checkout <sha>` plus a gitlink bump.
- Bad, because `git bundle` packs only the parent repo's history; submodule gitlinks travel but contents do not. The standard `COPY`-then-`cmake` Dockerfile sees empty submodule directories and fails.
- Bad, because fixing the bundle problem requires either a host-side pre-init step before bundling or a Dockerfile rework with `git` plus recursive checkout, each of which adds its own failure surface.
- Bad, because cold-clone local development needs `--recurse-submodules` or a post-clone `submodule update --init`; easy to forget.

### CMake `FetchContent`

- Good, because only CMake declarations live in the tree; they travel through `git bundle` fine.
- Good, because updates are a one-line SHA bump and a reconfigure.
- Good, because it is the modern, well-supported CMake idiom for header-only deps.
- Bad, because every configure-time fetch is a transient-failure axis during a one-shot grading run. Network being "available" is not the same as network being reliable.
- Bad, because each `FetchContent_Declare` needs the right flags (`GIT_SHALLOW`, `SYSTEM`, `EXCLUDE_FROM_ALL`) to behave well, multiplied across four declarations.
- Bad, because cold-clone local development needs network for the first configure.

### `ExternalProject` superproject

- Good, because only CMake declarations live in the tree.
- Good, because it is the right tool for libraries that bring their own build systems and need custom configure/install integration.
- Bad, because none of the utilities here have build systems to integrate with; the superproject machinery pays a cost for no benefit on header-only libs.
- Bad, because per-dep boilerplate (download + install + glue to expose headers + dependency edges into the consumer) is real engineering that has to be maintained.
- Bad, because build-time fetch touches every clean rebuild and slows incremental work on a fresh build tree.
- Bad, because the failure surface is the same as `FetchContent`, just at build time rather than configure time.

## More Information

- [`DEPENDENCIES.md`](../../DEPENDENCIES.md) -- per-utility rationale, the `kraken::` alias rule, and the procedure for adding a new vendored utility.
- [`0002-copy-vendor-third-party-utilities-matrix.html`](0002-copy-vendor-third-party-utilities-matrix.html) -- comparative scoring across all four mechanisms.
- [ADR 0001](0001-cpp-project-layout.md) -- the project layout that `submission/vendor/<name>/` slots into.
