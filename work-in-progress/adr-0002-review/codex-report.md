# ADR 0002 review: copy-vendor third-party utilities

## 1. Current ADR summary

ADR 0002 currently decides that `matching-engine-lab` should copy-vendor small header-only utility libraries into `vendor/<name>/upstream/`, carry `ORIGIN.txt`, `VERSION.txt`, `PATHS.txt`, and license text beside those bytes, expose each dependency as one `INTERFACE` target under `lab::vendor::<name>`, and consume the utility through `lab::` vocabulary where appropriate. The companion matrix compares copy-vendor, git submodules, CMake `FetchContent`, and `ExternalProject`, then selects copy-vendor because the current utility set is small, header-only, auditable in-tree, and cheap to wire without configure-time network access.

## 2. Gap with user's revised direction

The ADR is too narrow and too absolute for the revised direction. It treats copy-vendor as the only acceptable dependency mechanism and makes `FetchContent` a rejected alternative rather than the accepted path for larger libraries that need compilation.

Specific gaps:

- The title and framing say "utility headers", but the revised policy is a dependency-source policy for a demonstration project: inline bytes for small header-only libraries, GitHub `FetchContent` for compiled libraries.
- The Decision Outcome locks in copy-vendor alone. It does not encode the small-vs-large split.
- The Confirmation section says project CMake files do not use `FetchContent`, `ExternalProject_Add`, or submodule paths for these utility headers. That is valid for current utility headers, but it would conflict with the revised policy if read as a repository-wide ban on `FetchContent`.
- The Decision Drivers over-optimize for offline fresh checkouts. That remains important for small header-only utilities, but compiled dependencies can accept configure-time fetch when the alternative is committing large buildable third-party trees or adding heavier machinery.
- The matrix compares four single mechanisms. It does not evaluate the user's proposed hybrid strategy as the primary option.
- The current `DEPENDENCIES.md` and `scripts/vendor.sh` sections are copy-vendor-specific. They remain useful for inline vendors, but the ADR should make clear that `scripts/vendor.sh` governs only copy-vendored dependencies.
- The current vendor tree shows a good inline pattern for `readerwriterqueue`, `NamedType`, and `inplace_function`. It also contains `vendor/expected/CMakeLists.txt`, while `vendor/CMakeLists.txt` does not add `expected` and `DEPENDENCIES.md` describes `lab::expected` as toolchain-provided. The reframe should avoid broad wording that assumes every directory under `vendor/` is an active copy-vendored dependency.

## 3. What to borrow from abacus vs what to reject

Borrow these abacus idioms:

- The inlined header-only vendor shape in `/home/msi/abacus_workspace/abacus/vendor/readerwriterqueue/`, `vendor/magic_enum/`, and `vendor/decimal_for_cpp/`: a small checked-in include tree plus a local `CMakeLists.txt`.
- The simple per-library CMake wrapper pattern from those directories: `add_library(... INTERFACE)`, `target_include_directories(... SYSTEM INTERFACE ...)`, and an alias target such as `readerwriterqueue::readerwriterqueue`.
- The root vendor orchestrator idea from `/home/msi/abacus_workspace/abacus/vendor/CMakeLists.txt`: centralize third-party wiring in the vendor tree so application modules do not each invent dependency acquisition.
- The distinction between embedded vendors and acquired dependencies. Abacus has an "embedded vendors" section with `add_subdirectory(...)`; cpp-demo can use the same conceptual split, but with simpler source rules.

Prefer cpp-demo's existing improvements over abacus where they are already better:

- Keep cpp-demo's `ORIGIN.txt`, `VERSION.txt`, `PATHS.txt`, and license text audit trail for copy-vendor. Abacus's header-only vendor READMEs record URL and version, but cpp-demo's metadata is more structured and supports `scripts/vendor.sh check`.
- Keep cpp-demo's `lab::vendor::<name>` namespace and `lab::` project-facing wrapper rule. Abacus exposes upstream-style aliases directly; cpp-demo has a stronger local vocabulary boundary.

Reject these abacus mechanisms for cpp-demo:

- The `EXTERNAL_ARTIFACTS_DIR` requirement at the top of abacus `vendor/CMakeLists.txt`. It makes sense for external artifact stores, but it works against easy demo setup.
- The `ExternalProject_Add` modules such as `cmake/External_fmt.cmake`, `cmake/External_spdlog.cmake`, `cmake/External_Catch2.cmake`, `cmake/External_benchmark.cmake`, and `cmake/External_oneTBB.cmake`. They add install directories, byproducts, cache prepopulation, imported shared targets, and dependency edges that are heavier than this project needs.
- The system `find_package` fallback-first flow in abacus `vendor/CMakeLists.txt`. The revised policy says the default path should be inline or GitHub, not whatever happens to be installed on the host.
- The prebuilt binary and tarball flow in `cmake/External_OnixsB3FeedHandler.cmake`, `cmake/External_OnixsB3BoeEngine.cmake`, `cmake/External_OnixsFixEngine.cmake`, `cmake/External_tcpdirect.cmake`, and `cmake/Projects.cmake`. Those use local artifact paths, hashes, extracted binaries, and imported targets, which are explicit non-goals here.
- The raw configure-time `file(DOWNLOAD)` of Catch2 CMake modules in abacus `vendor/CMakeLists.txt`. That creates an ad-hoc source outside the two accepted sources and should not be copied.

## 4. Recommended reframe of ADR 0002

Keep this as one ADR. The decision is one dependency-source policy with two mechanisms, not two unrelated architectural decisions. Splitting it would make the policy harder to read because the small-vs-large boundary is the core rule.

Suggested title:

`Use inline vendors for small headers and FetchContent for compiled GitHub libraries`

Suggested Decision outline:

- `matching-engine-lab` uses only two third-party source mechanisms by default: inline vendored bytes committed under `vendor/`, and CMake `FetchContent` from GitHub at configure time.
- Small header-only libraries use copy-vendor. They live under `vendor/<name>/upstream/`, carry `ORIGIN.txt`, `VERSION.txt`, `PATHS.txt` when useful, and license text, and are exposed through one `SYSTEM INTERFACE` target under `lab::vendor::<name>`.
- Libraries that need compilation use CMake `FetchContent` from GitHub. They are pinned by tag or commit, configured with examples, tests, installs, and broad optional features disabled unless the project needs them, and linked through a narrow CMake target at the vendor boundary.
- `scripts/vendor.sh` remains the audit and update tool for copy-vendored dependencies only.
- The project does not use git submodules, `ExternalProject_Add`, third-party binary packages, tarball artifact stores, raw GitHub URL downloads for ad-hoc CMake modules, or system-package `find_package` fallbacks as the default dependency path.

Suggested Decision Drivers outline:

- Easy demo setup: a normal clone and configure should have few moving parts.
- Proportional machinery: small header-only utilities should not require configure-time fetch; compiled libraries should not require committing large third-party source trees.
- Clear audit trail: inline vendors carry origin, selected paths, version, and license text; fetched libraries carry pinned GitHub refs and visible CMake options.
- Local CMake boundary: project modules should see stable targets and `lab::` vocabulary where appropriate, not dependency-acquisition details.
- Reproducibility: pins must be explicit, and updates must be reviewable.
- License review: license text for inline vendors lives beside copied bytes; fetched dependencies must have an explicit license review pointer in dependency documentation.
- Low maintenance cost: avoid external artifact stores, binary package plumbing, and fallback paths that make the build depend on host state.

Suggested Confirmation outline:

- Each copy-vendored dependency has `ORIGIN.txt`, `VERSION.txt`, optional `PATHS.txt`, license text, copied upstream bytes under `upstream/`, and one `lab::vendor::<name>` `INTERFACE` target.
- `scripts/vendor.sh check <name>` can verify copy-vendored bytes against the recorded upstream commit when network access is available.
- Each `FetchContent` dependency is declared from a GitHub repository URL, pinned to a tag or commit, and configured in one vendor-owned CMake location.
- No project dependency path uses `ExternalProject_Add`, git submodules, `EXTERNAL_ARTIFACTS_DIR`, third-party binary archives, raw `file(DOWNLOAD)` from GitHub for CMake modules, or default system `find_package` fallback behavior.
- Project code consumes dependency concepts through local wrappers or stable targets so dependency acquisition stays in the vendor tree.

## 5. Open questions for the user

- Does the revised "only inline and GitHub" rule apply to all third-party dependencies listed in `DEPENDENCIES.md`, including Boost, fmt, spdlog, Catch2, and benchmark, or only to dependencies governed by ADR 0002 going forward?
- For GitHub `FetchContent`, should pins be immutable commit SHAs only, or are release tags acceptable for established libraries?
- Should fetched compiled dependencies have a small metadata file similar to `ORIGIN.txt` and `VERSION.txt`, or is the CMake declaration plus `DEPENDENCIES.md` enough audit trail?
- Should `vendor/CMakeLists.txt` own both inline vendors and `FetchContent` declarations, or should compiled dependency fetch declarations live under a separate CMake module included from the vendor tree?
- Should the ADR require a `lab::` wrapper for fetched compiled libraries, or is linking the upstream target acceptable when the upstream type is not part of domain vocabulary?
- What is the policy for dependencies used only by tests and benchmarks? They can affect demo setup as much as production dependencies, but they may not need the same wrapper vocabulary.

## 6. Suggested matrix update

Update the matrix so the proposed policy is an option, not an unstated compromise. The leading option should be "Inline small header-only vendors plus GitHub FetchContent for compiled libraries." Compare it against "Copy-vendor every dependency", "FetchContent every dependency", "Git submodules", and "ExternalProject or mixed external artifact strategy."

Revise the assumptions to state that this is a demonstration project, build setup should stay easy, and the only default third-party sources are inline committed bytes and GitHub configure-time fetches.

Add or adjust criteria:

- Demo setup simplicity.
- Fit for small header-only libraries.
- Fit for compiled libraries.
- Fresh checkout behavior.
- Configure-time network scope.
- Audit trail and license text.
- CMake complexity.
- Update ergonomics.
- Host-state coupling.
- Reversibility.

Expected scoring direction:

- The hybrid inline-plus-FetchContent option should score best overall because it keeps tiny utilities self-contained while avoiding large checked-in third-party source trees for compiled libraries.
- Copy-vendor-everything should score well for offline reproducibility and audit trail, but poorly for compiled library size, maintenance cost, and repository noise.
- FetchContent-everything should score well for update ergonomics and compiled libraries, but poorly for fresh checkout behavior and unnecessary network use for tiny headers.
- Git submodules should remain weaker because clone and checkout discipline hurt demo ergonomics.
- ExternalProject or abacus-style mixed artifact strategy should remain weakest for this project because it introduces external artifact stores, binary package plumbing, imported-target ceremony, and broad host-state coupling.
