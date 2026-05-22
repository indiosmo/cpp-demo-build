# ADR 0002 review report

Review of `docs/adr/0002-copy-vendor-third-party-utilities.md` and its
companion matrix against the user's revised direction for the
`matching-engine-lab` demonstration project, with stylistic comparison
to the `abacus` vendor tree.

## 1. Current ADR summary

ADR 0002 decides that every third-party dependency in the project enters
the build via copy-vendor: the upstream's selected public headers live
under `vendor/<name>/upstream/`, accompanied by `ORIGIN.txt`,
`VERSION.txt`, `PATHS.txt`, and the upstream license text; one
`INTERFACE` target per dependency is declared in the vendor tree and
re-exported through a `lab::` alias header in `src/lab/`. The decision
is justified on assumptions that the dependency set is exclusively small
header-only utilities, that a fresh checkout must build without network,
and that an explicit audit trail beats update ergonomics. `FetchContent`,
git submodules, and `ExternalProject` are all rejected, and the
"Confirmation" block explicitly forbids `FetchContent`,
`ExternalProject_Add`, and submodules for the utility headers.

## 2. Gap with the user's revised direction

The revised direction keeps copy-vendor for the small header-only cases
but accepts configure-time `FetchContent` as a lower-friction path for
larger libraries that need compilation. The current ADR is misaligned in
five concrete ways.

- The "Context and Problem Statement" frames the dependency set as
  "small header-only utilities," which forecloses the larger compiled
  case the user now wants to admit. The premise needs to be widened.
- The "Decision Outcome" picks copy-vendor as the single mechanism for
  the project. Under the revised direction the decision is a split rule
  keyed on dependency shape, not a single winner.
- The "Confirmation" block forbids `FetchContent` outright for utility
  headers. That clause needs to be rewritten so it forbids the
  mechanisms that are actually out of scope (binary drops, tarball
  downloads, `ExternalProject`, raw `file(DOWNLOAD ...)`, system
  `find_package` fallbacks) while permitting `FetchContent` for the
  compiled tier.
- The "Decision Drivers" list assumes header-only upstreams ("a few
  header-only libraries with no upstream build systems"). The drivers
  need to add "ease of build for a demonstration project" and
  "tolerance for configure-time network for the compiled tier" as
  first-class drivers, and to drop or qualify the assumption about
  upstream build systems.
- The "Pros and Cons" treat `FetchContent` as a uniformly bad fit. Under
  the split rule, `FetchContent` is the recommended path for the
  compiled tier, so its pros and cons need a second pass scoped to
  that tier rather than to header-only utilities.

The mechanics section (`ORIGIN.txt` / `VERSION.txt` / `PATHS.txt` /
sparse checkout / `lab::vendor::` interface targets) is still correct
for the header-only tier. It does not need to change.

## 3. What to borrow from abacus, what to reject

The abacus vendor tree is worth comparing for its embedded subdirs
only. Its external-project layer is exactly the baggage the user wants
to avoid.

Borrow from abacus:

- The per-subdir layout for vendored headers: a small subdirectory with
  its own `CMakeLists.txt` and an `include/` directory holding the
  upstream headers. The current cpp-demo `vendor/<name>/upstream/`
  shape is functionally equivalent; abacus's `include/` is a cleaner
  name when the upstream is genuinely header-only because it states
  the role of the bytes rather than their provenance. This is a small
  stylistic call; either name is defensible.
- The simple `INTERFACE` wrapper idiom: declare a target, set
  `SYSTEM INTERFACE` include directories, alias under
  `<name>::<name>`. The cpp-demo wrappers already match this idiom
  with the `lab::vendor::<name>` alias.
- The `if(NOT TARGET ...)` guard, which cpp-demo already uses; keep it.

Reject from abacus:

- `EXTERNAL_ARTIFACTS_DIR`, `EXTERNAL_SOURCE_PREFIX`,
  `EXTERNAL_INSTALL_PREFIX`, and the requirement to set them before
  configure. They turn a clone-and-build flow into a multi-step setup.
- The `External_*.cmake` modules driven by `ExternalProject_Add`. They
  build dependencies as separate CMake projects with their own
  install prefixes; that machinery is out of scope for a demonstration
  project.
- The `find_package(<dep>) ... if NOT <dep>_FOUND ... include(External_...)`
  fallback pattern. It encodes a third source (system packages) on top
  of the inline-or-github rule the user wants.
- The raw `file(DOWNLOAD ...)` of Catch2 CMake helpers from
  `raw.githubusercontent.com`. Configure-time downloads outside
  `FetchContent` are exactly the "tarball/raw URL" path the user
  explicitly excludes.
- Vendor binaries (`tcpdirect`, OnixS engines) pulled in by external
  project modules. The user rules out depending on third-party
  binaries.
- The mixing of `find_package`, `FetchContent`, `ExternalProject`, and
  embedded vendor `add_subdirectory` calls in a single
  `vendor/CMakeLists.txt`. cpp-demo should pick two sources (inline
  and `FetchContent`) and keep them visibly separated.

Also reject as a stylistic choice the absence of an audit trail. The
cpp-demo `ORIGIN.txt` / `VERSION.txt` / `PATHS.txt` files and the
`scripts/vendor.sh` flow give the project a clear story for
"where did these bytes come from and how would I update them?"; that is
worth keeping for the header-only tier even though abacus does not have
it. The cost is small and the payoff for a portfolio demo is high.

## 4. Recommended reframe

Keep ADR 0002 as a single ADR with two mechanisms. Splitting into
0002a/0002b would suggest the two halves are independent decisions; in
practice the split rule itself is the decision, so it belongs in one
record. The "two mechanisms" framing also matches abacus's mental
model (embedded vendors plus pulled deps) without inheriting its
machinery.

Suggested title:

> "Vendor third-party dependencies inline or via `FetchContent`"

Suggested Decision section (sketch, not finished prose):

- The project admits exactly two sources for third-party code:
  copy-vendor (bytes committed under `vendor/<name>/upstream/`) and
  configure-time `FetchContent` from a public git repository.
- The choice between the two is keyed on dependency shape:
  - Header-only libraries with no upstream build system: copy-vendor,
    using the existing `ORIGIN.txt` / `VERSION.txt` / `PATHS.txt`
    metadata, `scripts/vendor.sh` workflow, and `INTERFACE` target
    aliased under `lab::vendor::<name>`.
  - Larger libraries that need compilation: `FetchContent_Declare` /
    `FetchContent_MakeAvailable` against an upstream git URL pinned by
    SHA, with whatever options the upstream needs to disable tests,
    docs, and install rules.
- Both tiers expose the dependency to domain code through the project's
  `lab::` vocabulary where the upstream type would otherwise leak into
  module interfaces.

Suggested Decision Drivers (replacing the current list):

- The project is a demonstration; cloning and building must work with
  the documented toolchain and no further per-dependency setup.
- The only acceptable sources are inline bytes in the tree and
  configure-time `FetchContent` from public git. Binary drops, tarball
  downloads, raw URL fetches outside `FetchContent`, and system
  `find_package` fallbacks are out of scope.
- Setup cost should match dependency shape: header-only utilities pay
  the smaller cost of inline bytes; compiled libraries pay the larger
  cost of a configure-time fetch and build.
- License texts and origin metadata travel with copy-vendored bytes.
- For `FetchContent` dependencies, the pin lives in CMake itself
  (`GIT_TAG <sha>`); no parallel metadata file.
- CMake integration stays uniform: one consumable target per
  dependency, exposed through `lab::` aliases where the upstream
  vocabulary would otherwise leak.

Suggested Confirmation block (replacing the current one):

- Each header-only dependency lives under `vendor/<name>/upstream/`
  with its license text and `ORIGIN.txt` / `VERSION.txt` / `PATHS.txt`
  metadata, and is exposed as `lab::vendor::<name>`.
- Each compiled dependency is brought in by `FetchContent_Declare`
  with `GIT_REPOSITORY` and `GIT_TAG <sha>` (full commit SHA, not a
  moving ref) in a single CMake file under the vendor tree.
- The build does not use `ExternalProject_Add`, git submodules, raw
  `file(DOWNLOAD ...)` of source or CMake helpers, tarball
  downloads, or `find_package` fallbacks for project dependencies.
- Domain code consumes third-party dependencies through `lab::`
  aliases or the upstream target a `FetchContent` provides, never
  through ad-hoc include paths.

## 5. Open questions

- Threshold between "small header-only" and "compiled." The split is
  obvious for the current set (NamedType, inplace_function,
  readerwriterqueue, expected are header-only; the candidate compiled
  deps would be things like fmt, spdlog, Catch2, Google Benchmark). It
  is less obvious for header-only libraries that ship a non-trivial
  CMake configuration the project would otherwise have to recreate
  (Catch2's `Catch.cmake` helpers being the canonical example). Worth
  deciding whether "needs upstream's CMake machinery" pushes a
  nominally header-only library into the `FetchContent` tier.
- Where `FetchContent` declarations live. Options: keep them in
  `vendor/CMakeLists.txt` next to `add_subdirectory(<name>)` calls; or
  give them their own subdir per dep (`vendor/<name>/CMakeLists.txt`
  with `FetchContent_*` calls in it) to mirror the header-only layout.
  The latter keeps the vendor tree uniform; the former keeps the
  declarations together for at-a-glance review.
- Whether to vendor a `VERSION.txt`-style file alongside
  `FetchContent` declarations. The CMake call already pins the SHA, so
  a parallel file is redundant. A short comment giving the upstream
  ref name (tag) next to the SHA in the CMake call is probably
  sufficient; should be confirmed.
- How `lab::` aliases extend to `FetchContent`-pulled targets. For
  header-only utilities the project already wraps the upstream behind
  a thin alias header in `src/lab/lab/`. For compiled deps the
  upstream usually exposes its own canonical target name (`fmt::fmt`,
  `Catch2::Catch2`, `benchmark::benchmark`). Worth deciding whether
  every compiled dep gets a `lab::vendor::<name>` indirection too, or
  whether domain code is allowed to link the upstream target directly
  when the upstream vocabulary is already the domain vocabulary.
- Offline-build story for the compiled tier. The copy-vendor tier
  builds offline; `FetchContent` does not, after the first configure
  populates its cache. Worth saying explicitly in the ADR whether the
  project promises offline rebuilds after a single online configure,
  and whether `FETCHCONTENT_FULLY_DISCONNECTED` is a supported mode.
- Whether `scripts/vendor.sh` needs a companion mode that lists
  `FetchContent` pins from CMake, so `vendor.sh list` and `status`
  cover both tiers, or whether `FetchContent` is left to CMake
  entirely.

## 6. Suggested matrix update

The current matrix scores four options (copy-vendor, submodules,
`FetchContent`, `ExternalProject`) against one set of criteria, with
header-only assumptions baked in. Under the revised direction that
single one-shot scoring no longer reflects the decision being made.

Two reasonable shapes:

- Two matrices. One scoped to the header-only tier (copy-vendor,
  submodules, `FetchContent`, with `ExternalProject` dropped as
  obviously out of scope), one scoped to the compiled tier
  (`FetchContent`, `ExternalProject`, system `find_package`, copy-vendor
  with a vendored build, with the two reject categories acting as
  contrast). Each matrix produces its own winner; together they
  produce the split rule.
- One matrix with split criteria. Keep the four options as columns and
  add an explicit "dependency shape" axis: rows are scored separately
  for "header-only" and "compiled," producing two sub-decisions in one
  table. Cells stay short; the reader sees both regimes in one place.

The two-matrix shape is easier to read and matches the ADR's split
decision shape more cleanly; the single-matrix shape keeps the
companion file count down. Either way, the assumptions block needs to
state that the project admits exactly two sources (inline and
`FetchContent`) and that the matrix is comparing within that
constraint, not over the full space of dependency-management tools.
