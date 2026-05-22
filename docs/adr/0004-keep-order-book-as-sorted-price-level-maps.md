# 4. Stage the order-book data structure by iteration

**Status:** accepted

**Date:** 2026-05-15

**Companion:** comparison across the order-book data-structure options lives in [`0004-keep-order-book-as-sorted-price-level-maps-matrix.html`](0004-keep-order-book-as-sorted-price-level-maps-matrix.html).

## Context and Problem Statement

The matching engine needs a per-symbol limit order book that preserves price-time priority, exposes best-price aggregates for market-data updates, matches marketable orders against the opposite side, and removes resting orders by `(user, user_order_id)` on cancel.

The data-structure decision is staged. The first implementation needed to pass the suite quickly and keep the matching contract easy to inspect. Now that the suite is passing, the next optimization pass can spend complexity on the hot-path properties the first iteration deferred: no per-placement allocation and direct cancellation.

## Decision Drivers

- Preserve price-time priority and the top-of-book semantics documented in [`docs/engine-specs.md`](../engine-specs.md).
- Keep the matching loop readable enough for an evaluator to verify against the exercise contract.
- Fit the first iteration timeline: prefer standard containers and focused tests until the behavior suite is green.
- In the optimization iteration, remove avoidable hot-path allocation and make cancellation direct.
- Handle sparse and unknown price levels without requiring fixed tick-range reservations.
- Keep the data-structure swap local if cancellation or allocation costs become material.
- Prefer Boost-backed storage machinery over a custom allocator or bespoke linked structure when optimizing.

## Considered Options

1. **Sorted maps of vectors.** Store bids and asks in side-specific `std::map<price, std::vector<order>>` containers, ordered so `begin()` is the best price. Append residual orders to the vector for the price level and scan for cancels.
2. **Intrusive lists plus object pool.** Store each price level as a `boost::intrusive::list<order>` backed by a preallocated pool, with a `(user, user_order_id) -> order*` index for direct cancellation.
3. **Preallocated vectors with tombstones.** Reserve bounded contiguous storage for each price level, mark cancelled entries inactive, and skip tombstones during matching.
4. **Paged order blocks.** Store each price level as a chain of fixed-size blocks allocated from a pool, with tombstones and block reclamation.

## Decision Outcome

Chosen path: **sorted maps of vectors for the first iteration; intrusive lists plus object pool for the optimization iteration**. The book that ships in `kraken_submission` is the optimization-iteration shape.

The first iteration uses sorted maps of vectors because that structure is the smallest implementation that gives the submission the required semantics. The side maps keep price levels ordered, `begin()` reads the best price, and each level's vector preserves arrival order. The matching loop can walk levels in price-time priority without also explaining a custom allocator, intrusive hooks, tombstone compaction, or block-reclamation rules.

The optimization iteration moves to intrusive lists plus an object pool. That option targets the costs the first iteration deliberately accepted: cancellation scans resting orders because the wire cancel command does not include symbol, side, or price, and placement can allocate when a new price level or vector capacity appears. A pool-backed intrusive level representation, paired with a `(user, user_order_id) -> order*` index, gives direct cancellation and removes per-order heap allocation while keeping sparse price levels natural.

### Consequences

- Good, because the first iteration got behavior correct with standard containers and focused tests.
- Good, because the staged approach avoids mixing behavior bring-up with custom storage machinery.
- Good, because the optimization target is already scoped: replace per-level vectors with intrusive lists backed by a pool and add a direct cancel index.
- Good, because sparse and unknown price levels remain natural in both stages.
- Bad, because the first iteration still has `O(R)` worst-case cancellation and can allocate on placement.
- Bad, because the optimization iteration adds pool sizing, pointer lifetime, hook safety, and index consistency concerns that need dedicated tests.
- Bad, because intrusive lists trade vector sweep locality for direct cancellation and stable pooled nodes.

### Confirmation

The first-iteration decision is in effect when:

- Each book stores bids and asks as side-specific `std::map<price, std::vector<order>>` containers.
- The bid map is ordered descending and the ask map is ordered ascending, so `begin()` is the top price for each side.
- Placement appends to the price-level vector when the matching loop leaves a non-zero residual.
- Cancel searches resting orders, erases the matching vector entry, removes empty price levels, and returns the removed order for the cancel acknowledgement.
- Top-of-book observers compute best price and aggregate quantity from the first map level.

The optimization decision is in effect when:

- Resting orders live in pool-owned nodes.
- Each price level stores orders in an intrusive list that preserves arrival order.
- A direct `(user, user_order_id)`-to-node index drives cancellation without scanning books and levels.
- Cancellation unlinks the node from its level, erases the index entry, and returns the node to the pool.
- Tests cover partial fills, full level removal, cancellation of front, middle, and back nodes, unknown cancels, and pool/index consistency.

### Follow-ups

- Implement the intrusive-list plus pool shape after preserving the current behavior suite.
- Add focused microbenchmarks around placement, cancellation, and deep sweeps to quantify the trade.
- Reconsider paged order blocks only if sweep locality dominates after the list+pool implementation is measured.

## Pros and Cons of the Options

### Sorted maps of vectors (first iteration)

- Good, because it matches the exercise semantics with standard containers.
- Good, because map ordering and vector append make price-time priority visible in the code.
- Good, because it handles arbitrary sparse prices without fixed-range allocation.
- Bad, because cancellation scans resting orders.
- Bad, because insertions can allocate and vector erasure can move surviving entries.

### Intrusive lists plus object pool

- Good, because direct order pointers and intrusive unlinking give `O(1)` cancellation.
- Good, because a preallocated pool can remove trading-hour allocation.
- Good, because this is the selected optimization path once the suite is passing.
- Bad, because it introduces pool sizing, lifetime, hook safety, and index consistency concerns that need dedicated tests.
- Bad, because pointer chasing hurts sweep locality compared with contiguous storage.

### Preallocated vectors with tombstones

- Good, because contiguous storage is cache-friendly during sweeps.
- Good, because cancellation can become a cheap tombstone write when every cancel has a direct handle.
- Bad, because unknown price ranges and per-level order density make safe reservation hard.
- Bad, because tombstones accumulate unless another compaction or reclamation mechanism is added.
- Bad, because exceeding a reserved bound either reallocates on the hot path or rejects otherwise valid orders.

### Paged order blocks

- Good, because blocks preserve most of the sweep locality of arrays while avoiding one huge reservation per level.
- Good, because a pool can bound allocation once the maximum active block count is known.
- Bad, because block chaining, tombstone accounting, iterators, and reclamation create a large local bug surface.
- Bad, because the implementation is harder to explain than the behavior it supports in the current exercise.
- Bad, because it still needs benchmark evidence and sizing assumptions before it should replace the simpler book.

## More Information

- [`DESIGN.md`](../../DESIGN.md) -- data-structure summary and improvement list.
- [`docs/engine-specs.md`](../engine-specs.md) -- observable matching-engine behavior.
- [`0004-keep-order-book-as-sorted-price-level-maps-matrix.html`](0004-keep-order-book-as-sorted-price-level-maps-matrix.html) -- comparative matrix for the options.
