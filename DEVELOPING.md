# Developing

This document covers working on the repository itself: setting up a local
environment, building, running tests, and the conventions the project follows.

## First-time setup

Run the setup script once after cloning:

```bash
./setup.sh
```

This initializes submodules, installs the shared local C++ toolchain, installs
`uv`, installs the [prek](https://github.com/j178/prek)-managed pre-commit hook
that runs `clang-format` on staged C++ files, and writes
`CMakeUserPresets.json` for editor integrations.

The local C++ toolchain installs under `$WORKSPACE_ROOT` (default:
`~/cpp_workspace`):

- GCC 16.1.0
- CMake 4.3.2
- LLVM/Clang 23 tools
- binutils 2.46.0, mold 2.41.0, ccache 4.13.5, lcov 2.3.2
- Boost 1.91.0 built with the custom GCC

If `uv` or `prek` are missing, the script offers to install them. The hook
configuration lives in `.pre-commit-config.yaml`.

`./setup.sh` runs `apt` by default so a clean machine gets the OS package set
before the local toolchain is checked or built. On a machine where the system
packages are already present and `apt update` is slow or blocked, run:

```bash
./setup.sh --skip-system-packages
```

`uv` is shared by developer scripts such as `scripts/render_performance_charts.py`;
run `./scripts/install_uv.sh` directly when you only need that toolchain.

Source the toolchain environment before running CMake directly:

```bash
source scripts/setenv.sh
```

`build.sh` sources that environment automatically.

## Building and testing

`build.sh` wraps the CMake configure, build, and `ctest` steps so the common
workflow is a single command:

```bash
./build.sh                       # debug preset, all targets, run tests
./build.sh release               # release preset
./build.sh asan server           # named preset and target
./build.sh asan client           # named preset and target
./build.sh debug -DLAB_BUILD_BENCHMARKS=OFF  # forward extra cmake options
```

Anything after the optional target that starts with `-` is forwarded to the
configure step. Tests run automatically after a successful build.

Build outputs live under `_build/<preset>/`. The script also refreshes a
top-level `compile_commands.json` so clangd works in the editor without a
separate build.

### Presets

The repo ships several CMake presets covering different compilers and
sanitizers (Address/Leak/UB, Thread, Clang+ASan, plus plain debug and
release). The authoritative list is `CMakePresets.json`; pass the preset name
as the first argument to `build.sh`, or invoke `cmake --preset=<name>`
directly after sourcing `scripts/setenv.sh`.

Use the sanitizer presets when chasing memory or threading bugs locally and
before submitting changes -- they catch issues the default debug build misses.

### Running CMake directly

If you prefer not to use `build.sh`:

```bash
source scripts/setenv.sh
cmake --preset=debug
cmake --build _build/debug
ctest --test-dir _build/debug --output-on-failure
```

## Project layout

- `src/` -- libraries and the `client` and `server` applications.
- `test/` -- module unit tests.
- `benchmarks/` -- Google Benchmark microbenchmarks.
- `vendor/` -- copy-vendored third-party utilities.
- `cmake/` -- shared CMake modules (compiler flags, sanitizer wiring).
- `scripts/` -- developer scripts invoked by `setup.sh` and the pre-commit
  hook.
- `docs/adr/` -- architecture decision records. Start a new ADR when making a
  decision worth recording; see existing entries for the format.

## Code style

C++ source is formatted with `clang-format` using the rules in `.clang-format`.
The pre-commit hook formats changed files automatically; to reformat the whole
tree manually, run `./scripts/format.sh --all`.

## Conventions

### Const placement

West const everywhere: `const T&`, `const T*`, `const auto&`. The qualifier
sits to the left of the type it applies to. Avoid the east-const form
`T const&` / `auto const`.

### Namespace aliases

Use the standard aliases when a source or test file reads several domain
namespaces together often enough that the abbreviation improves clarity:

```cpp
namespace md = market_data;
namespace me = matching_engine;
namespace rt = order_routing;
```

These aliases keep nested domain vocabulary short enough to read, for example
`md::types::price`.

Within a domain's own namespace, keep the `types::` qualifier on domain types.
Do not bring individual types into scope:

```cpp
auto price = types::price{100};
```

```cpp
using types::price; // Do not do this.
```

Never add namespace aliases or `using` declarations to header files. They leak
into includers and make name lookup depend on include order.

In `.cpp` and test files, use judgment. A single invocation in a file does not
warrant a namespace alias; repeated cross-domain vocabulary usually does.

## Refreshing the performance charts

`docs/performance/` holds the rendered SVG that [`docs/performance.md`](docs/performance.md)
links to. Rendering is split in two so iterating on chart aesthetics does
not re-run benchmarks:

1. **Collect** -- runs the order-book microbench under the
   `current_latency_percentiles` filter and writes the google-benchmark
   JSON under `docs/performance/data/`:

   ```bash
   ./build.sh release matching_engine_order_book_benchmark
   ./scripts/collect_performance_data.sh
   ```

   The benchmark registration carries its own `Iterations(...)`,
   `Repetitions(...)`, and `ComputeStatistics(...)` for p50/p90/p95/p99,
   so the script passes `--benchmark_min_time` for parity with other
   drivers but the value has no effect on per-repetition iteration
   count. Wall time is a few seconds. The resulting `benchmark.json` is
   committed so the doc rebuilds on a fresh clone without re-running
   the bench.

2. **Render** -- reads only `docs/performance/data/benchmark.json` and
   writes the percentile SVG:

   ```bash
   ./scripts/render_performance_charts.py
   ```

## Running the server

Build the server and client targets, then run the binaries from the selected
preset:

```bash
./build.sh debug server
./build.sh debug client
./_build/debug/server --host 127.0.0.1 --port 1234
printf 'N, 1, IBM, 10, 100, B, 1\nF\n' | ./_build/debug/client --host 127.0.0.1 --port 1234
```

The process listens for UDP order commands and writes market data records to
stdout. Use `ctest --test-dir _build/debug --output-on-failure` to rerun the
unit tests for an existing build tree.
