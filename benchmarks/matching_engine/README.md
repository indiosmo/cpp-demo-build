# Matching engine benchmarks

Component-level measurements for `matching_engine::engine`. These sit
one layer above the [order_book microbenchmarks](../order_book/):
every operation goes through the engine's variant dispatch, the
per-symbol book lookup, the cross-symbol `resting_index_` map, and
any `top_of_book` emit the wire contract triggers.

**Headline.** A cancel costs ~85 ns and is essentially the same
whether it emits a `top_of_book` (cancel at the best bid, 82 ns/op)
or skips the emit (cancel at the deepest bid, 87 ns/op) -- the
level walk under the emit is small relative to the rest of the
cancel path. A place + cancel round-trip costs 170 ns at the seeded
depth. The matching loop runs at ~23 ns per consumed order on a
wide cross. The raw place path (no fill) costs ~176 ns/place
including the post-place `top_of_book` emit.

## What each benchmark measures

The workload is one symbol with 256 levels per side, ~24 orders per
level (Normal, clamped to `[1, 60]`), ~6,000 resting orders per side.
A fixed seed (`0xC0FFEE`) makes every iteration of every benchmark
start from the same canonical book.

| Benchmark                  | Probes                                                       | Why the shape matters                                                                                                                                                                              |
| -------------------------- | ------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `BM_PlaceLimitNoFill`      | Place at the existing best bid, no cross                     | Full `new_order` path including the post-place `top_of_book` emit and its level walk. The placement-side counterpart to the cancel-at-top measurement.                                             |
| `BM_PlaceCancelRoundTrip`  | Place + cancel pair at the seeded best bid, both timed       | The shape a market maker pays on every quote refresh. Per-iteration cost is the round-trip cost directly.                                                                                          |
| `BM_CancelAtTopOfBook`     | Cancel at the best bid level                                 | `top_before == best_bid`, so the engine emits a `top_of_book` whose total walks the surviving level. The cancel hot path top-of-book churn pays.                                                  |
| `BM_CancelDeepInBook`      | Cancel at the deepest seeded bid level                       | The cancelled price never matches `best_bid`, so the engine skips the TOB emit. The engine's minimum-cost cancel; paired with the row above, isolates the cost of the emit and its level walk.    |
| `BM_CrossManyOrders`       | One marketable buy that consumes ~200 asks across whole levels | The matching loop -- `best_ask` -> `top_ask_front` -> trade -> `fill_top_ask_front` -- is the engine's tightest hot path. Per-trade cost dominates the iteration.                                |

The cancel and cross benchmarks use the Setup-per-repetition pattern
(`Iterations(K) -> Repetitions(R) -> Setup`), so the cost of seeding
the engine and pre-building the command vector lives outside the
timed loop. The cancel benchmarks place a slab of orders behind the
cancellable batch so the level depth stays in the seeded regime
throughout the timed run. Position within a level does not affect
cancel cost (the book unlinks intrusively in O(1)), so this is
observationally equivalent to cancelling at any position.

The two non-Setup benchmarks (`BM_PlaceLimitNoFill`,
`BM_PlaceCancelRoundTrip`) use google-benchmark's default auto-iter
loop. `BM_PlaceLimitNoFill` runs a batch of K=64 places followed by
K=64 cancels inside each iteration so the timed segment dominates
loop-framework overhead; `BM_PlaceCancelRoundTrip` is small enough
per iteration that the auto-iter count is high and the per-iter
overhead is negligible.

## Results

CPU time is the meaningful column for the Setup-based benchmarks:
each repetition is a short window (K iterations of ~50-200 ns each),
so the real-time column picks up some per-repetition framework
overhead that the CPU time column factors out. Read the `_median`
or `_mean` row for the per-op number; `_stddev` and `_cv` describe
the spread across the 30 repetitions.

| Benchmark                  | Per-iteration (CPU) | Per-op                     | items/s |
| -------------------------- | -------------------:| -------------------------- | -------:|
| `BM_PlaceLimitNoFill`      |    11,257 ns        | ~176 ns per place          |   5.7 M |
| `BM_PlaceCancelRoundTrip`  |       170 ns        |  170 ns per round-trip     |   5.9 M |
| `BM_CancelAtTopOfBook`     |        82 ns        |   82 ns per cancel         |  12.2 M |
| `BM_CancelDeepInBook`      |        87 ns        |   87 ns per cancel         |  11.5 M |
| `BM_CrossManyOrders`       |     4,900 ns        | ~23 ns per consumed order  |  42.9 M |

The two cancel benchmarks land within ~5 ns of each other, which
puts the cost of the post-cancel `top_of_book` emit and its level
walk in the noise band of the cancel itself.

## How to reproduce

    ./build.sh release matching_engine_benchmark
    ./_build/release/benchmarks/matching_engine/matching_engine_benchmark

Useful filters:

    --benchmark_filter=BM_Place
    --benchmark_filter=BM_Cancel
    --benchmark_filter=BM_Cross
