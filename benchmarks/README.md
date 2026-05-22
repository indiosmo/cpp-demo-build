# Benchmarks

Google Benchmark microbenchmarks for the matching engine. Each suite
has its own README with the workload, the numbers, and the
interpretation.

| Suite | What it measures |
|-------|------------------|
| [`order_book/`](order_book/) | Head-to-head between the order book variants under the operations the engine actually performs (placement, cancel, traversal). Isolates the data-structure choice from [ADR-0004](../../docs/adr/0004-keep-order-book-as-sorted-price-level-maps.md). |
| [`matching_engine/`](matching_engine/) | Component-level cost of the engine across representative workload shapes -- placement, cancel, quote-refresh round-trip, marketable cross. |

## Environment

All numbers in the per-suite READMEs come from the same machine:
Linux 6.6 (WSL2), 18 vCPUs at 3.79 GHz, single-threaded release
build.

## Build

    ./build.sh release <benchmark_target>

Each suite's README names its target and the exact reproduction
command.
