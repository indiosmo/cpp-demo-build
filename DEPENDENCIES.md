# Dependencies

Inventory of vendored utilities, the system packages the build links
against, and the procedure for adding a new dependency. The
copy-vendor mechanism is recorded in
[ADR 0002](docs/adr/0002-copy-vendor-third-party-utilities.md);
the `kraken::` alias rule and the full vocabulary catalogue live in
[`submission/src/kraken/README.md`](submission/src/kraken/README.md).

## Vendored

| `kraken::` alias | Upstream | License | Rationale |
|---|---|---|---|
| `kraken::strong_type<T, Tag>` | [NamedType](https://github.com/joboccara/NamedType) | MIT | [Compile-time correctness](docs/cpp-design-principles.md#compile-time-correctness): tagged primitives the compiler rejects when swapped. |
| `kraken::inplace_function<Sig, Cap>` | [SG14 `inplace_function`](https://github.com/WG21-SG14/SG14) | BSL-1.0 | Pipeline-stage `on_*` callbacks: no capture allocation, inline-visible call sites, capture-size regressions caught at compile time. |
| `kraken::concurrent_queue<T>` | [moodycamel::ReaderWriterQueue](https://github.com/cameron314/readerwriterqueue) | BSD-2-Clause | Lock-free SPSC edges between the three pipeline threads. |
| `kraken::expected<T, E>` | [TartanLlama `expected`](https://github.com/TartanLlama/expected) | CC0-1.0 | C++20 value-or-error vocabulary at domain boundaries; collapses to `std::expected` once the project moves to C++23. |

## System packages

Installed by the grading container's `Dockerfile`:

| Package | Linked for |
|---|---|
| `libboost-all-dev` | header-only: `leaf`, `container_hash`, `pfr`, `algorithm`, `stacktrace` (kraken vocabulary); `asio` (network adapter); `intrusive`, `pool`, `unordered` (matching engine) |
| `libfmt-dev` (fmt 9.1) | `fmt::fmt-header-only` across the kraken vocabulary; same ABI backs `libspdlog-dev` |
| `libspdlog-dev` | `spdlog::spdlog` behind `market_data::spdlog_sink` |
| `catch2` | unit tests |
| `libbenchmark-dev` | `submission/benchmarks/` |

`nlohmann-json3-dev`, `libgtest-dev`, and `libgmock-dev` are in the
apt manifest but unused by the submission.

## Vendor tree

```
submission/vendor/<name>/
  ORIGIN.txt       upstream git URL (one line)
  VERSION.txt      line 1: pinned ref (tag, branch, or SHA)
                   line 2: resolved SHA (written by 'sync')
  PATHS.txt        optional: git sparse-checkout patterns, one per line
  CMakeLists.txt   INTERFACE target wrapping upstream/
  upstream/        upstream repo at the pinned ref, .git stripped
```

Blank lines and `#`-prefixed lines in the three text files are ignored.

## `scripts/vendor.sh`

```
scripts/vendor.sh list                  show pinned versions
scripts/vendor.sh sync   [<name>...]    clone upstream into <name>/upstream/
scripts/vendor.sh check  [<name>...]    re-clone at the recorded SHA and byte-compare
scripts/vendor.sh status [<name>...]    diff the recorded SHA against upstream HEAD
```

`sync` and `check` are the only commands that touch the network. `sync`
shallow-clones at the ref on line 1 of `VERSION.txt` (honouring
`PATHS.txt` when present), strips `.git`, and writes the resolved SHA
back as line 2. `check` confirms the committed `upstream/` bytes still
match what the recorded SHA serves; tampering inside the parent repo
shows up in `git diff` regardless.

## Adding a dependency

1. Create `submission/vendor/<name>/` with `ORIGIN.txt`, `VERSION.txt`
   (the ref to pin), and a `CMakeLists.txt` declaring an `INTERFACE`
   library `kraken_vendor_<name>` aliased to `kraken::vendor::<name>`,
   with `target_include_directories(... SYSTEM INTERFACE ...)` pointing
   at the upstream's public headers. Copy from an existing vendor.
2. For monorepo upstreams, add `PATHS.txt` with one sparse-checkout
   pattern per line (see `submission/vendor/inplace_function/PATHS.txt`).
3. Run `scripts/vendor.sh sync <name>`. If the pinned ref was a moving
   target, copy line 2 of `VERSION.txt` over line 1 so the pin is
   immutable.
4. Adjust the include path in the per-vendor `CMakeLists.txt` if the
   upstream layout differs from your guess.
5. Add `add_subdirectory(<name>)` to `submission/vendor/CMakeLists.txt`.
6. Add the `kraken::` re-export at
   `submission/src/kraken/kraken/<role>.hpp` (one `using` alias plus
   the upstream `#include`) and link the kraken target to
   `kraken::vendor::<name>`.

Domain libraries consume the `kraken::` alias from step 6, never the
upstream header.

## Caveats

- **The full upstream tree rides along.** README, examples, tests, and
  upstream build files sit under `upstream/` even though only the
  headers are consumed. Use `PATHS.txt` to narrow the checkout for
  larger upstreams.
- **Layout shifts on version bumps.** The per-vendor `CMakeLists.txt`
  hard-codes which subdirectory of `upstream/` holds the public
  headers. A reorganised upstream forces an include-path edit
  alongside the `VERSION.txt` bump.
- **No schema for the pin files.** A typo in `ORIGIN.txt` surfaces as
  a clone failure on the next `sync`; a typo in `PATHS.txt` surfaces
  as a missing-file error in the consumer.
