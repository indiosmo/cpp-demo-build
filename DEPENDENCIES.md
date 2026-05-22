# Dependencies

Inventory of third-party libraries the build depends on, how they reach
CMake, and the procedure for adding a new one. The mechanism is recorded
in [ADR 0002](docs/adr/0002-use-fetchcontent-for-third-party-dependencies.md); the
`lab::` alias rule and the full vocabulary catalogue live in
[`src/lab/README.md`](src/lab/README.md).

Every third-party dependency enters the build through CMake `FetchContent`
from a public GitHub upstream. Boost and the C++26 standard library are
toolchain-provided and arrive through `BOOST_ROOT` / `Boost_ROOT` path
loading (see [`DEVELOPING.md`](DEVELOPING.md)).

## Third-party (FetchContent)

| Consumed target | Upstream | Pin | Used for |
|---|---|---|---|
| `lab::vendor::NamedType` | [NamedType](https://github.com/joboccara/NamedType) | SHA `76668abe` | `lab::strong_type<T, Tag>`: tagged primitives the compiler rejects when swapped. |
| `lab::vendor::inplace_function` | [SG14 `inplace_function`](https://github.com/WG21-SG14/SG14) | SHA `c9261438` | `lab::inplace_function<Sig, Cap>`: pipeline-stage `on_*` callbacks with no capture allocation. |
| `lab::vendor::readerwriterqueue` | [moodycamel::ReaderWriterQueue](https://github.com/cameron314/readerwriterqueue) | `v1.0.6` | `lab::concurrent_queue<T>`: lock-free SPSC edges between pipeline threads. |
| `fmt::fmt-header-only` | [fmtlib/fmt](https://github.com/fmtlib/fmt) | `11.0.2` | Lab vocabulary formatting; shared ABI with `spdlog`. |
| `nlohmann_json::nlohmann_json` | [nlohmann/json](https://github.com/nlohmann/json) | `v3.12.0` | `lab::json`: JSON parsing and formatting for the UDP command and market-data protocols. |
| `spdlog::spdlog` | [gabime/spdlog](https://github.com/gabime/spdlog) | `v1.14.1` | `market_data::spdlog_sink`. Built with `SPDLOG_FMT_EXTERNAL_HO=ON` so it inlines through the same `fmt` headers as the lab vocabulary. |
| `Catch2::Catch2WithMain` | [catchorg/Catch2](https://github.com/catchorg/Catch2) | `v3.7.1` | Unit tests. `Catch.cmake` is added to `CMAKE_MODULE_PATH` so `catch_discover_tests` is available. |
| `quickfix` | [quickfix/quickfix](https://github.com/quickfix/quickfix) | `v1.16.0` | FIX protocol engine: session management, message parsing, acceptor/initiator wiring. Built static (`QUICKFIX_SHARED_LIBS=OFF`); examples and tests disabled. Upstream provides no `::` alias, so consumers link `quickfix` directly. |

The libraries-of-types tier wraps each upstream behind a thin
`lab::vendor::<name>` INTERFACE target so domain code never depends on the
upstream type directly. The libraries-of-functions tier exposes the upstream
target unchanged because the upstream vocabulary (`fmt::`, `spdlog::`,
`Catch2::`) is already part of the project vocabulary.

All declarations live in [`vendor/CMakeLists.txt`](vendor/CMakeLists.txt).
The first configure populates the `FetchContent` cache from GitHub;
subsequent configures reuse it.

## Toolchain-provided

| Component | Source | Used for |
|---|---|---|
| C++26 standard library | Compiler (GCC 16.1 / Clang 23) | `<expected>`, `<format>`, `<ranges>`, the parallel algorithms, and the rest of the C++26 surface. |
| Boost (header-only components) | Workspace install under `$WORKSPACE_ROOT/boost/...` consumed via `BOOST_ROOT` / `Boost_ROOT` | `leaf` (result/error transport), `container_hash` (`fixed_string` hashing), `pfr` (`lab::auto_hash` reflective field walk), `algorithm` (string replace in `result.hpp`), `stacktrace` (`LAB_ASSERT`), `asio` (`lab_network_asio` UDP receiver), `intrusive`, `pool`, `unordered` (matching engine). |
| `Threads` | Platform pthreads | `server_app` and other targets that need explicit `Threads::Threads`. |

Boost is installed by
[`scripts/dependencies/install_boost.sh`](scripts/dependencies/install_boost.sh)
under `$WORKSPACE_ROOT` and the CMake presets pin `BOOST_ROOT` / `Boost_ROOT`
to the same prefix. The workspace install pattern mirrors the abacus
workspace setup. This carve-out is recorded in
[ADR 0002](docs/adr/0002-use-fetchcontent-for-third-party-dependencies.md).

## Adding a third-party dependency

1. Add a `FetchContent_Declare` block in
   [`vendor/CMakeLists.txt`](vendor/CMakeLists.txt) with
   `GIT_REPOSITORY` pointing at the public GitHub upstream and `GIT_TAG`
   set to either a recent release tag or a commit SHA. Use `GIT_SHALLOW
   TRUE` to keep the fetch small.
2. If the upstream is a *library of types* (headers consumed under a
   `lab::` alias), pass `SOURCE_SUBDIR _lab_skip_add_subdirectory_` so
   the upstream's CMake (usually a test build) is not invoked, then
   declare a `lab_vendor_<name>` INTERFACE library aliased under
   `lab::vendor::<name>` and point
   `target_include_directories(... SYSTEM INTERFACE ${<name>_SOURCE_DIR}/...)`
   at the public headers. Add a `lab::` re-export in
   [`src/lab/lab/`](src/lab/lab/) that includes the upstream header and
   aliases the upstream type into the project vocabulary.
3. If the upstream is a *library of functions* with its own CMake that
   exposes a canonical target (`<name>::<name>`), disable the
   upstream's tests, examples, docs, and install rules through that
   project's own option variables (set them with
   `CACHE BOOL "" FORCE`), then call `FetchContent_MakeAvailable(<name>)`.
   Consumers link the upstream target directly; no `lab::` wrapper is
   added.
4. Add a row to this file with upstream URL, pin, and consumed target
   name. Update [`src/lab/README.md`](src/lab/README.md) if the new
   dependency adds a `lab::` vocabulary entry.

## Updating a pin

Bump the `GIT_TAG` in `vendor/CMakeLists.txt` and reconfigure. CMake will
re-fetch into its `FetchContent` cache on the next configure. Review the
upstream changelog for the chosen range; if the upstream renames or
removes a header, fix the include path or the `lab::` re-export at the
same time.
