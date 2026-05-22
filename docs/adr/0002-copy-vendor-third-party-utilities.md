# 2. Vendor third-party code via CMake `FetchContent`

**Status:** accepted

**Date:** 2026-05-22

**Companion:** comparative scoring across `FetchContent`, copy-vendor, git
submodules, and `ExternalProject` lives in
[`0002-copy-vendor-third-party-utilities-matrix.html`](0002-copy-vendor-third-party-utilities-matrix.html).

## Context and Problem Statement

`matching-engine-lab` depends on a small set of third-party libraries spanning
two shapes: header-only utilities used for the lab vocabulary (strong typedefs,
fixed-capacity callbacks, SPSC queues, decimal types) and compiled libraries
used by domain code, tests, and benchmarks (`fmt`, `spdlog`, `Catch2`, Google
Benchmark, and similar). Every third-party dependency in this layer is in
scope; nothing in it is taken from system packages.

Two pieces stand outside that layer and are treated as toolchain-provided:
the C++26 standard library (`<expected>`, `<format>`, the parallel algorithms,
etc.) and Boost. Boost is installed under `$WORKSPACE_ROOT` by
`scripts/dependencies/install_boost.sh` and consumed through `BOOST_ROOT` /
`Boost_ROOT` path loading from `scripts/setpath.sh` and the CMake presets,
matching the pattern used by the abacus workspace. Boost is not pulled by
`FetchContent` because the superproject is large enough that fetching it from
GitHub at configure time would dominate cold-clone setup; the workspace-install
path keeps configure fast and matches the rest of the toolchain (GCC, Clang,
CMake, ninja, ccache, mold, lcov).

The repository should be easy to clone, configure, audit, and build. The
choice is how third-party code reaches the build. The viable mechanisms differ
both in what they handle (header-only vs. compiled) and in where the source of
truth lives (CMake vs. an out-of-band tool).

An earlier version of this ADR split the decision: copy-vendor for
header-only utilities and `FetchContent` for compiled libraries. That split
required two coexisting acquisition systems -- a CMake path for compiled deps
and a shell-script-plus-metadata path (`scripts/vendor.sh`,
`ORIGIN.txt`/`VERSION.txt`/`PATHS.txt`, committed `upstream/` trees) for
header-only ones. Maintaining two systems is the cost; the benefit was
offline fresh-checkout behavior for tiny headers, which is not a goal the
project commits to.

## Decision Drivers

- One source of truth for dependency acquisition. CMake already drives the
  build, so dependency declarations should live in CMake too.
- Proportional machinery. A single declarative mechanism should handle both
  header-only and compiled libraries without per-tier shell scripts or
  committed upstream trees.
- Two acceptable sources for third-party code: `FetchContent` from public
  GitHub, and code written inside this repository. No system packages, no
  binary drops, no tarball downloads, no `ExternalProject_Add`, no git
  submodules, no raw `file(DOWNLOAD ...)` of source or CMake helpers, no
  `find_package` fallbacks. The toolchain (compilers, build tools, Boost,
  the C++26 stdlib) is a separate layer and is not subject to this rule.
- Reproducibility. Each dependency is pinned by an explicit, immutable
  reference visible in CMake.
- Auditable updates. Changing a dependency is a CMake diff against a public
  upstream.
- Local CMake boundary. Project modules see stable targets and -- where
  warranted -- a thin `lab::` indirection over upstream types.
- Online builds. Configure and build run with network access available; the
  project does not promise an offline fresh-checkout experience.

## Considered Options

1. **CMake `FetchContent` for everything.** Declare every third-party
   dependency in CMake under `vendor/CMakeLists.txt` with
   `FetchContent_Declare` (`GIT_REPOSITORY` plus `GIT_TAG`) and
   `FetchContent_MakeAvailable`.
2. **Copy-vendor for everything.** Commit selected upstream sources under
   `vendor/<name>/upstream/` with metadata files (`ORIGIN.txt`,
   `VERSION.txt`, `PATHS.txt`), one `INTERFACE` library per dep, and a
   refresh script.
3. **Split: copy-vendor for header-only, `FetchContent` for compiled.** Use
   committed bytes for header-only utilities and CMake-driven fetch for
   compiled libraries.
4. **Git submodules.** Track each upstream as a submodule pinned by SHA.
5. **`ExternalProject` superproject.** Drive downloads, configure, build,
   and install through `ExternalProject_Add`.

## Decision Outcome

Chosen option: **CMake `FetchContent` for everything**.

`FetchContent` is the only mechanism in the four CMake supports that handles
both header-only utilities and compiled libraries uniformly, expresses pins
inside CMake, and avoids committing third-party trees into this repo. Picking
it for both tiers removes the need for a parallel acquisition path
(`scripts/vendor.sh` plus metadata files plus committed `upstream/`
directories), which is the larger maintenance cost than first-configure
network use.

Copy-vendor for everything is rejected because it requires committing
buildable trees for compiled libraries (`fmt`, `Catch2`, Google Benchmark),
which inflates the repository and turns updates into multi-step recopy
operations driven by an out-of-band script. The split arrangement is rejected
because it makes two acquisition systems coexist for marginal gain on
header-only fresh-checkout behavior; the project does not need that gain. Git
submodules are rejected because cold clones need explicit `--recurse-submodules`
or follow-up `submodule update --init`, and review spans multiple repositories.
`ExternalProject_Add` is rejected because it duplicates what `FetchContent`
does with extra plumbing, runs at build time rather than configure time, and
loses the in-tree consumption model.

### Mechanics

Per dependency:

1. Add a `FetchContent_Declare` block under `vendor/CMakeLists.txt` with
   `GIT_REPOSITORY` pointing at the public GitHub upstream and `GIT_TAG`
   set to either a release tag (when recent and immutable in practice) or
   a commit SHA.
2. Disable upstream tests, examples, docs, and install rules through the
   library's own options before `FetchContent_MakeAvailable`.
3. Call `FetchContent_MakeAvailable(<name>)`.
4. Add a row to `DEPENDENCIES.md` recording the upstream, the chosen
   target name, and the pin form.
5. For libraries of types whose vocabulary should not leak into module
   interfaces (for example `NamedType`-backed strong typedefs, a decimal
   type), add a `lab::` re-export header under `src/lab/lab/` mapping
   the upstream type to a project-vocabulary alias. Domain code includes
   only the `lab::` header.
6. For libraries of functions whose canonical target name is already
   domain vocabulary (`fmt::fmt`, `Catch2::Catch2WithMain`,
   `benchmark::benchmark`, `spdlog::spdlog`), no `lab::` wrapper is
   added; consumers link the upstream target directly.

### Consequences

- A single mechanism governs every third-party dependency. There is no
  shell script, no committed `upstream/` tree, no per-dep metadata
  alongside CMake.
- Pins are expressed in CMake and reviewed as ordinary CMake diffs.
- The first configure on a fresh checkout populates the `FetchContent`
  cache from GitHub. Subsequent configures use the cache. The project
  treats network as available at configure time.
- Compiled-library options (tests, examples, install rules) must be
  explicitly disabled per dep to avoid widening the project's build or
  install surface.
- Tag pins must be chosen with care. The decision admits tags only when
  the upstream's tagging practice is stable; SHA pins are the fallback
  when a tag is moving or when the chosen reference is not a release.
- Removing copy-vendor removes the `scripts/vendor.sh` workflow and the
  `vendor/<name>/upstream/` trees from this repository. That cleanup is
  follow-on work tracked separately.

### Confirmation

The decision is in effect when:

- Every third-party dependency is brought in by `FetchContent_Declare`
  with `GIT_REPOSITORY` and `GIT_TAG` in `vendor/CMakeLists.txt`.
- No third-party dependency is committed under `vendor/<name>/upstream/`,
  tracked as a git submodule, downloaded via raw `file(DOWNLOAD ...)`,
  brought in through `ExternalProject_Add`, or resolved through
  `find_package` as the default acquisition path.
- No third-party dependency is taken from a system package.
- The only `find_package` calls in the build resolve toolchain-provided
  components: Boost (from `BOOST_ROOT`), `Threads`, and the like.
- Libraries of types are consumed through a `lab::` re-export header.
  Libraries of functions are consumed through their upstream target
  directly.
- `DEPENDENCIES.md` lists every dependency with its upstream URL, pin,
  and consumed target name, and names Boost and the C++26 stdlib in a
  separate toolchain-provided section.

## Pros and Cons of the Options

### CMake `FetchContent` for everything

- Good, because one mechanism handles both header-only and compiled
  dependencies.
- Good, because pins live in CMake alongside the build that uses them.
- Good, because no third-party bytes are committed to this repository.
- Good, because updates are CMake diffs that point at a public upstream.
- Bad, because first-configure needs network and a populated cache.
- Bad, because compiled-library options must be set per dep to keep
  tests, examples, and install rules out of the project's build.

### Copy-vendor for everything

- Good, because a fresh checkout carries every byte it builds.
- Good, because review can be done without leaving the repository.
- Bad, because compiled libraries become large committed trees.
- Bad, because updates need an out-of-band refresh tool and metadata
  files maintained outside CMake.
- Bad, because the dependency source of truth diverges from CMake.

### Split: copy-vendor for header-only, `FetchContent` for compiled

- Good, because header-only fresh checkouts stay self-contained.
- Bad, because two acquisition systems coexist for marginal gain.
- Bad, because the shell-script tier (`vendor.sh` plus metadata files)
  has no representation in CMake, so the build cannot see or validate
  what it depends on for that tier.
- Bad, because the rule for which tier a given dep belongs to needs
  case-by-case judgement.

### Git submodules

- Good, because the upstream SHA is explicit.
- Bad, because cold-clone setup requires `--recurse-submodules` or a
  follow-up `submodule update --init`.
- Bad, because source review spans multiple repositories.
- Bad, because the pin lives in a `.gitmodules` file rather than in
  CMake.

### `ExternalProject` superproject

- Good, because it handles dependencies with their own build systems.
- Bad, because `FetchContent` already does the same with less plumbing
  for the consumption model this project uses.
- Bad, because the in-tree target model `FetchContent_MakeAvailable`
  exposes is lost.
- Bad, because dependencies become build-time, not configure-time,
  citizens.

## More Information

- [`DEPENDENCIES.md`](../../DEPENDENCIES.md) -- per-dependency rationale and
  update procedure.
- [`0002-copy-vendor-third-party-utilities-matrix.html`](0002-copy-vendor-third-party-utilities-matrix.html)
  -- comparative scoring across the dependency-acquisition options.
- [ADR 0001](0001-cpp-project-layout.md) -- the root-level layout that
  contains the vendor tree.
