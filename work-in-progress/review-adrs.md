# Review ADRs

## ADR 0002 -- vendoring

Per-agent reports under `adr-0002-review/`:
- `claude-report.md`
- `codex-report.md`

Both reports converge. This section is the consolidated review.

### Where the ADR stands today

ADR 0002 picks copy-vendor as the only mechanism, on the assumption that the
dependency set is "small header-only utilities" with no upstream build system.
The Confirmation block explicitly bans `FetchContent`, `ExternalProject_Add`,
and submodules for those utility headers. `DEPENDENCIES.md` and
`scripts/vendor.sh` codify the copy-vendor flow (ORIGIN.txt, VERSION.txt,
PATHS.txt, sparse checkout, `lab::vendor::<name>` INTERFACE target).

### Where it is wrong under the revised direction

The revised position keeps copy-vendor for header-only utilities and admits
configure-time `FetchContent` from public GitHub for libraries that need
compilation. The ADR needs to encode that split rule instead of a single
winner. Concretely:

- The Context and Problem Statement frames the dependency set as "small
  header-only utilities" and forecloses the compiled case.
- The Decision Outcome picks copy-vendor as the sole mechanism. It should
  pick a two-mechanism rule keyed on dependency shape.
- The Confirmation block forbids `FetchContent` outright. It needs to forbid
  what is actually out of scope (binary drops, tarball downloads, raw
  `file(DOWNLOAD ...)` of upstream sources or CMake helpers,
  `ExternalProject_Add`, system `find_package` fallbacks as the default
  path, git submodules) while permitting `FetchContent` for the compiled
  tier.
- The Decision Drivers overweight offline fresh-checkout behavior, which is
  still right for the header-only tier but not for compiled deps that would
  otherwise mean committing large buildable trees.
- The Pros and Cons treat `FetchContent` as uniformly bad. It needs a
  second pass scoped to the compiled tier.
- The mechanics section (ORIGIN/VERSION/PATHS, sparse checkout,
  `lab::vendor::` INTERFACE targets) is still correct for the header-only
  tier and does not need to change.

### What to borrow from abacus, what to reject

Borrow:

- The per-dep `INTERFACE` wrapper idiom: `add_library(<name> INTERFACE)`,
  `target_include_directories(... SYSTEM INTERFACE ...)`, alias under
  `<name>::<name>`. cpp-demo already follows this with the
  `lab::vendor::<name>` alias.
- The `if(NOT TARGET ...)` guard. Already in cpp-demo. Keep it.
- The conceptual split between "embedded vendors" and "acquired
  dependencies" in a single vendor tree, without inheriting abacus's
  acquisition machinery.

Reject:

- `EXTERNAL_ARTIFACTS_DIR`, `EXTERNAL_SOURCE_PREFIX`, and the requirement
  to set them before configure.
- `External_*.cmake` modules driven by `ExternalProject_Add`
  (`External_fmt.cmake`, `External_spdlog.cmake`, `External_Catch2.cmake`,
  `External_benchmark.cmake`, `External_oneTBB.cmake`).
- The `find_package(<dep>) ... if(NOT <dep>_FOUND) include(External_...)`
  fallback pattern. It encodes a third source (system packages) on top of
  the inline-or-GitHub rule.
- Raw configure-time `file(DOWNLOAD ...)` of CMake helpers from
  `raw.githubusercontent.com` (the Catch2 helpers case).
- Vendor binaries and tarball flows (`External_OnixsB3FeedHandler.cmake`,
  `External_OnixsB3BoeEngine.cmake`, `External_OnixsFixEngine.cmake`,
  `External_tcpdirect.cmake`, `Projects.cmake`).
- Mixing four mechanisms in one `vendor/CMakeLists.txt`. cpp-demo keeps
  the two tiers visibly separated.
- The absence of an audit trail. Keep cpp-demo's ORIGIN/VERSION/PATHS plus
  `scripts/vendor.sh` for the inline tier; abacus has only ad-hoc README
  notes.

### Recommended reframe (one ADR, two mechanisms)

Keep ADR 0002 as a single record. The split rule is the decision.

Suggested title (pick one):

- "Vendor third-party dependencies inline or via `FetchContent`"
- "Use inline vendors for small headers and `FetchContent` for compiled
  GitHub libraries"

Suggested Decision section (sketch):

- The project admits exactly two sources for third-party code: copy-vendor
  bytes committed under `vendor/<name>/upstream/`, and configure-time
  `FetchContent` from a public GitHub repository pinned by SHA (or by
  release tag for established libraries, to be settled below).
- Choice between the two is keyed on dependency shape:
  - Header-only libraries with no upstream build machinery use copy-vendor
    with the existing ORIGIN.txt / VERSION.txt / PATHS.txt metadata,
    `scripts/vendor.sh` workflow, and `lab::vendor::<name>` INTERFACE
    target.
  - Libraries that need compilation use `FetchContent_Declare` /
    `FetchContent_MakeAvailable` against a GitHub URL, with upstream
    options for tests, examples, docs, and install rules turned off
    unless the project needs them.
- Domain code consumes both tiers through the project's `lab::` vocabulary
  where the upstream type would otherwise leak into module interfaces.
- `scripts/vendor.sh` governs only copy-vendored dependencies.

Suggested Decision Drivers (replacing the current list):

- Easy demo setup: a clone and configure should have few moving parts and
  no per-dependency manual steps.
- Proportional machinery: header-only utilities should not need a
  configure-time fetch; compiled libraries should not need committing
  large buildable trees.
- Two acceptable sources: inline bytes and `FetchContent` from GitHub.
  Binary drops, tarballs, raw URL fetches outside `FetchContent`, system
  `find_package` fallbacks, and git submodules are out of scope.
- Audit trail: inline vendors carry ORIGIN/VERSION/PATHS and license text;
  fetched libraries carry an explicit pinned ref in CMake and an entry in
  `DEPENDENCIES.md`.
- Reproducibility: pins are explicit; updates are reviewable.
- Local CMake boundary: project modules see stable targets and `lab::`
  vocabulary, not dependency-acquisition details.

Suggested Confirmation block (replacing the current one):

- Each header-only dependency lives under `vendor/<name>/upstream/` with
  its license text and ORIGIN/VERSION/PATHS metadata, and is exposed as
  `lab::vendor::<name>`.
- Each compiled dependency is brought in by `FetchContent_Declare` with
  `GIT_REPOSITORY` and `GIT_TAG <pin>` from one vendor-owned CMake
  location.
- The build does not use `ExternalProject_Add`, git submodules, raw
  `file(DOWNLOAD ...)` of source or CMake helpers, tarball downloads,
  `EXTERNAL_ARTIFACTS_DIR`-style artifact stores, or `find_package`
  fallbacks as the default acquisition path.
- Domain code consumes third-party dependencies through `lab::` aliases or
  through a stable upstream target where the upstream vocabulary is
  already the project vocabulary.

### Open questions to settle before rewriting

Combined from both reports.

1. Threshold between "header-only" and "compiled." Obvious for the current
   set (NamedType, inplace_function, readerwriterqueue, expected are
   header-only; fmt, spdlog, Catch2, Google Benchmark are compiled). Less
   obvious when a header-only upstream ships CMake helpers that the
   project would otherwise have to recreate (Catch2's `Catch.cmake` is the
   canonical example). Does "needs upstream's CMake machinery" push it
   into the `FetchContent` tier?
2. Scope of the policy. Does the inline-or-GitHub rule apply to every
   third-party dependency listed in `DEPENDENCIES.md` (including Boost,
   fmt, spdlog, Catch2, libbenchmark, currently consumed as system
   packages via `setup.sh`), or only to dependencies governed by ADR 0002
   going forward?
3. Pin form for `FetchContent`. Immutable commit SHAs only, or are
   release tags acceptable for established libraries?
4. Where `FetchContent` declarations live. Options: in
   `vendor/CMakeLists.txt` next to `add_subdirectory(<name>)` calls; or
   per-dep subdirs (`vendor/<name>/CMakeLists.txt` containing
   `FetchContent_*` calls) to mirror the header-only layout.
5. Parallel metadata for fetched deps. CMake already pins the SHA; an
   ORIGIN/VERSION-style file alongside the CMake declaration would be
   redundant. Probably just a comment in CMake plus a row in
   `DEPENDENCIES.md`.
6. `lab::` wrappers over fetched targets. Header-only utilities go behind
   a thin alias header in `src/lab/lab/`. Compiled deps usually expose
   their own canonical target (`fmt::fmt`, `Catch2::Catch2`,
   `benchmark::benchmark`). Does every compiled dep get a
   `lab::vendor::<name>` indirection, or is linking the upstream target
   directly acceptable when its vocabulary is already domain vocabulary?
7. Offline rebuilds. Copy-vendor builds offline. `FetchContent` builds
   offline after the first configure populates its cache. Is
   `FETCHCONTENT_FULLY_DISCONNECTED` a supported mode, and does the ADR
   make a promise about offline rebuilds?
8. `scripts/vendor.sh` over both tiers, or only the inline tier? Listing
   FetchContent pins from CMake would unify `vendor.sh list` and
   `vendor.sh status` across both tiers, but at the cost of CMake-parsing
   in the script.
9. Tests-and-benchmarks-only deps. They influence demo setup as much as
   production deps, but probably do not need `lab::` wrappers. Worth
   stating explicitly.

Loose end picked up while reviewing: `vendor/expected/CMakeLists.txt`
exists, but `vendor/CMakeLists.txt` does not `add_subdirectory(expected)`
and `DEPENDENCIES.md` describes `lab::expected` as toolchain-provided.
Either the directory should be removed or the ADR rewrite should explain
why an inactive vendor tree is kept. Worth resolving as part of the
0002 work.

### Matrix update

The current matrix scores four single mechanisms (copy-vendor, submodules,
`FetchContent`, `ExternalProject`) against header-only-baked criteria. It
no longer matches the decision shape. Two options:

- Two matrices. One scoped to the header-only tier (copy-vendor vs
  submodules vs `FetchContent`); one scoped to the compiled tier
  (`FetchContent` vs `ExternalProject` vs `find_package` vs vendored
  build). Each produces its own winner; together they produce the split
  rule. Easier to read; matches the ADR's split decision shape.
- One matrix with the hybrid as a first-class option. Compare
  "inline-plus-FetchContent" against "copy-vendor everything",
  "FetchContent everything", submodules, and `ExternalProject` / mixed
  artifact strategy. The hybrid should score best for demo setup, fit per
  tier, and host-state coupling; copy-vendor-everything stays strong on
  offline reproducibility but weak on compiled-library size and noise;
  `FetchContent`-everything stays strong on update ergonomics but weak on
  fresh-checkout behavior for tiny headers.

The two-matrix shape is structurally cleaner. The single-matrix shape
keeps the companion file count down. Either way, the assumptions block
needs to restate the project's two-source constraint so the matrix is
explicitly comparing within that constraint.

## ADR 0004

Reframe as list+pool to match v3 implementation. Drop v1/v2 and lift v3
to the library directly.
