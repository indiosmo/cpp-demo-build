# Order book benchmarks

Microbenchmarks for the production order book exposed through
`matching_engine::order_book`.

Historical implementations (`v1`, `v2`, `v3::std_order_book`) live in
the tree for design review, but this benchmark binary only drives the
production book today. The pre-refactor comparison numbers across the
four variants are preserved at the bottom of this file for reference.

## What Each Benchmark Measures

The heavy fixture seeds 512 price levels x 64 orders per level, a
depth that matches a realistic per-tick book.

| Benchmark                  | Probes                                                       | Why the shape matters |
| -------------------------- | ------------------------------------------------------------ | --------------------- |
| `BM_PlaceExistingPrice`    | One placement onto an existing level                         | Isolates per-call placement cost when the side-map level already exists. |
| `BM_PlaceNewPrice`         | One placement at a previously unseen price                   | Forces a fresh side-map entry per iteration. |
| `BM_PlaceGrowingLevel`     | Place into the same level, swept at depth = 64/256/1024      | Average insert cost across a level that grows from 0 to `depth` during a repetition. |
| `BM_CancelDeepLevel`       | Cancel a target placed at the deepest seeded level           | Confirms handle-based cancellation cost stays independent of book depth. |
| `BM_TraverseThreePrices`   | Sweep three full levels with an aggressing order             | Models the inner loop of marketable-order matching. |
| `BM_LookupByPrice`         | Find existing price levels across a deterministic schedule   | Measures side-map point lookup across swept depth. |
| `BM_IterateFullSide`       | Walk every price level on one side                           | Measures side-map iteration across swept depth. |

The mutating benchmarks use the Setup-per-repetition pattern: Setup
builds a fresh `boost::pool`-backed fixture, pre-builds the operation
inputs (order states to place, or pointers to pre-placed nodes to
cancel) and -- for the growing-level fixture -- cycles the workload
through the book once to warm `boost::pool`'s slab and avoid first-
touch costs leaking into the first timed iteration. The timed loop
then walks a cursor and performs one operation per iteration, so per-
iteration CPU time is the per-op cost directly.

The percentile group runs `BM_PlaceGrowingLevel` and
`BM_CancelDrainingLevel` at depth 200 with 30 repetitions, reporting
p50/p90/p95/p99 over the per-repetition CPU time samples. The
draining fixture pre-places 200 orders on an empty level in Setup and
the timed loop cancels them front-to-back.

## Results

CPU time is the meaningful column for the Setup-based benchmarks (real
time picks up per-repetition framework overhead on the short K=64
windows). Read the `_median` row for the headline per-op number;
`_stddev` and `_cv` describe the spread across 30 repetitions.

### Heavy fixture (current production book)

| Benchmark                              | per-op (CPU, median) | cv  |
| -------------------------------------- | --------------------:| ---:|
| `BM_PlaceExistingPrice`                |               34 ns  | 17% |
| `BM_PlaceNewPrice`                     |              705 ns  | 20% |
| `BM_PlaceGrowingLevel` (depth = 64)    |               16 ns  |  7% |
| `BM_PlaceGrowingLevel` (depth = 256)   |              7.8 ns  |  6% |
| `BM_PlaceGrowingLevel` (depth = 1024)  |              6.2 ns  | 28% |
| `BM_CancelDeepLevel`                   |               38 ns  |  7% |
| `BM_TraverseThreePrices`               |              226 ns  |   - |

`BM_PlaceNewPrice` is ~20x more expensive than `BM_PlaceExistingPrice`
because it pays a `flat_map` insert on every iteration; the existing-
price case only appends to the intrusive list at the level the seed
already created. `BM_PlaceGrowingLevel` numbers drop with depth: at
K=64 the per-repetition window (~1 us) carries enough loop-framework
overhead per op to inflate the apparent cost; the K=1024 number
(~6 ns/op) is the closer approximation of the steady-state insert
cost into a populated level.

### Side-map sweep (current production book)

| N    | `BM_LookupByPrice` | `BM_IterateFullSide` |
| ----:| ------------------:| --------------------:|
|   10 |             5.0 ns |               4.9 ns |
|   50 |             8.9 ns |              21  ns  |
|  100 |              11 ns |              43  ns  |
|  250 |              13 ns |             101  ns  |
|  500 |              16 ns |             395  ns  |
| 1000 |              47 ns |             794  ns  |

### Latency percentiles at depth 200 (current production book)

`BM_PlaceGrowingLevel` and `BM_CancelDrainingLevel` registered with
`Iterations(200) -> Repetitions(30) -> ComputeStatistics(p50/p90/p95/p99)`.

| Percentile | Insert (ns/op) | Cancel (ns/op) |
| ---------- | -------------: | -------------: |
| p50        |            8.5 |           7.75 |
| p90        |            8.5 |            8.0 |
| p95        |            9.9 |            8.0 |
| p99        |           12.1 |            9.8 |

p99 stays within ~25% of p50 on both operations, which matches what
the order book's data structures predict: an intrusive list append /
unlink is O(1), and the side-map insert / erase the percentile run
exercises only at the end of the drain is amortised to a 1/200th
contribution per iteration.

## How To Reproduce

    ./build.sh release matching_engine_order_book_benchmark
    ./_build/release/benchmarks/order_book/matching_engine_order_book_benchmark

Useful filters:

    --benchmark_filter=current_heavy
    --benchmark_filter=current_sweep
    --benchmark_filter=current_latency_percentiles

## Historical comparison: v1 / v2 / v3 (pre-refactor)

The benchmark binary previously ran each scenario against four book
implementations (`v1`, `v2`, `v3::std_order_book`, `v3::flat_order_book`)
templated over the book type. The current binary only drives the
production book, so these numbers cannot be reproduced from this tree.
The historical run used `--benchmark_min_time=2s` per case and the
pre-refactor `state.PauseTiming()` / `state.ResumeTiming()` shape, so
the absolute values are not directly comparable to the production
table above. They are preserved for the relative comparison between
implementations.

| Benchmark                              | v1        | v2       | v3_std    | v3_flat   |
| -------------------------------------- | --------- | -------- | --------- | --------- |
| `BM_PlaceExistingPrice` (heavy)        |   505 ns  |   516 ns |   501 ns  |   508 ns  |
| `BM_PlaceNewPrice` (heavy)             |   553 ns  |   545 ns |   535 ns  |   540 ns  |
| `BM_PlaceGrowingLevel` (heavy, N=64)   |  1193 ns  |  1100 ns |  1475 ns  |  1506 ns  |
| `BM_PlaceGrowingLevel` (heavy, N=256)  |  2909 ns  |  2894 ns |  4425 ns  |  4390 ns  |
| `BM_PlaceGrowingLevel` (heavy, N=1024) |  9641 ns  | 10229 ns | 16393 ns  | 16168 ns  |
| `BM_CancelWorstCase` (heavy)           | 24438 ns  |   507 ns |   512 ns  |   507 ns  |
| `BM_TraverseThreePrices` (heavy)       |  78.5 ns  |   132 ns |   133 ns  |   136 ns  |

`BM_PlaceGrowingLevel` per-order figures (ns/op), historical:

| Depth | v1   | v2   | v3_std | v3_flat |
| ----- | ---- | ---- | ------ | ------- |
| 64    | 18.6 | 17.2 | 23.0   | 23.5    |
| 256   | 11.4 | 11.3 | 17.3   | 17.2    |
| 1024  |  9.4 | 10.0 | 16.0   | 15.8    |

What the historical comparison showed:

- `BM_CancelWorstCase` for v1 cost ~24 us because v1's cancel scans
  every resting order across both sides until `(user, id)` matches.
  v2 and v3 cancel by handle in constant time -- the ~50x cost gap
  was the load-bearing argument for moving to handle-keyed cancels.
- Placement was within noise across v1/v2 and ~5-7 ns/op slower on
  v3 (the per-place cost of moving node allocation out of the book
  itself, in exchange for v3's cleaner separation between structure
  and storage).
- The two v3 side-map variants (`std::map`, `boost::container::flat_map`)
  were indistinguishable on the heavy fixture. The decision to ship
  `flat_order_book` came from the side-map sweep, where lookup and
  iteration on small maps (the realistic per-symbol shape) favour
  the contiguous `flat_map` layout; see
  [ADR 0004](../../../docs/adr/0004-keep-order-book-as-sorted-price-level-maps.md).
