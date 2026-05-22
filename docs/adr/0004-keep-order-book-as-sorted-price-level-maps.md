# 4. Stage the order-book data structure by iteration

**Status:** accepted

**Date:** 2026-05-15

**Companion:** comparison across the order-book data-structure options lives in
[`0004-keep-order-book-as-sorted-price-level-maps-matrix.html`](0004-keep-order-book-as-sorted-price-level-maps-matrix.html).

## Context and Problem Statement

The matching engine needs a per-symbol limit order book that preserves
price-time priority, exposes best-price aggregates for market-data updates,
matches marketable orders against the opposite side, and removes resting orders
by `(user, user_order_id)` on cancel.

The data-structure decision is staged. The first implementation should make the
matching contract easy to inspect and test. The optimized implementation should
spend complexity on hot-path properties: direct cancellation, stable resting
order addresses, and no per-order heap allocation.

## Decision Drivers

- Preserve the price-time priority and top-of-book semantics documented in
  [`docs/engine-specs.md`](../engine-specs.md).
- Keep the behavior-first implementation readable.
- Remove avoidable placement allocation in the optimized implementation.
- Make cancellation direct even when the cancel request does not carry symbol,
  side, or price.
- Handle sparse and unknown price levels without fixed tick-range reservation.
- Keep the data-structure swap local to the matching-engine module.
- Prefer Boost-backed storage machinery over bespoke allocator and list code.

## Considered Options

1. **Sorted maps of vectors.** Store bids and asks in side-specific sorted maps
   from price to FIFO vectors of resting orders.
2. **Sorted side maps with intrusive lists and an object pool.** Store each
   price level as an intrusive FIFO of pool-owned order nodes, with a
   `(user, user_order_id) -> order_node*` identity index.
3. **Preallocated vectors with tombstones.** Reserve bounded contiguous storage
   for each price level, mark cancelled entries inactive, and skip tombstones
   during matching.
4. **Paged order blocks.** Store each price level as a chain of fixed-size
   blocks allocated from a pool, with tombstones and block reclamation.

## Decision Outcome

Chosen path: **sorted maps of vectors for the behavior-first iteration;
intrusive lists plus object pool for the optimized iteration**.

The first iteration uses sorted maps of vectors because that structure is the
smallest implementation that expresses the required semantics. Side maps keep
price levels ordered, `begin()` reads the best price, and each level's vector
preserves arrival order.

The optimized iteration moves resting orders into pool-owned nodes linked into
per-price-level intrusive lists. A direct identity index maps
`(user, user_order_id)` to the resting node, so cancellation does not scan the
book. The side map still owns price-level ordering, while each level owns FIFO
order through its intrusive list and a running quantity total for top-of-book
aggregation.

### Consequences

- Good, because the staged approach keeps behavior bring-up separate from
  storage mechanics.
- Good, because price-time priority stays visible in both shapes: side-map
  ordering provides price priority, and per-level FIFO order provides time
  priority.
- Good, because sparse and unknown prices stay natural.
- Good, because the optimized shape gives direct cancellation and stable order
  addresses for the identity index.
- Bad, because the behavior-first shape has worst-case cancellation scans and
  can allocate during placement.
- Bad, because the optimized shape introduces pool sizing, pointer lifetime,
  hook safety, and index consistency concerns.
- Bad, because intrusive lists trade vector sweep locality for direct
  cancellation and stable pooled nodes.

### Confirmation

The behavior-first decision is in effect when:

- Each book stores bids and asks as side-specific sorted maps from price to a
  FIFO container of resting orders.
- The bid side sorts descending and the ask side sorts ascending, so `begin()`
  is the top price for each side.
- Placement appends residual orders to the price-level FIFO.
- Cancel searches resting orders, erases the matching entry, removes empty
  price levels, and returns the removed order for acknowledgement.
- Top-of-book observers compute best price and aggregate quantity from the
  first side-map level.

The optimized decision is in effect when:

- Resting orders live in pool-owned nodes.
- Each price level stores orders in an intrusive list that preserves arrival
  order.
- A direct `(user, user_order_id)` identity index drives cancellation.
- Cancellation unlinks the node from its level, erases the index entry, and
  returns the node to the pool.
- Tests cover partial fills, full level removal, cancellation of front, middle,
  and back nodes, unknown cancels, and pool/index consistency.

### Follow-ups

- Keep microbenchmarks around placement, cancellation, and deep sweeps as the
  evidence surface for future storage changes.
- Reconsider paged order blocks only if benchmarks show sweep locality dominates
  after list-plus-pool optimization.

## Pros and Cons of the Options

### Sorted maps of vectors

- Good, because it matches the matching semantics with standard containers.
- Good, because map ordering and vector append make price-time priority visible.
- Good, because it handles arbitrary sparse prices.
- Bad, because cancellation scans resting orders.
- Bad, because insertions can allocate and vector erasure can move surviving
  entries.

### Sorted side maps with intrusive lists and object pool

- Good, because direct order pointers and intrusive unlinking give direct
  cancellation.
- Good, because a preallocated pool can remove per-order heap allocation.
- Good, because side maps keep sparse price handling simple.
- Bad, because it introduces pool sizing, lifetime, hook safety, and index
  consistency concerns that need dedicated tests.
- Bad, because pointer chasing hurts sweep locality compared with contiguous
  storage.

### Preallocated vectors with tombstones

- Good, because contiguous storage is cache-friendly during sweeps.
- Good, because cancellation can become a cheap tombstone write with a direct
  handle.
- Bad, because unknown price ranges and per-level order density make safe
  reservation hard.
- Bad, because tombstones accumulate unless compaction or reclamation is added.
- Bad, because exceeding a reserved bound either reallocates on the hot path or
  rejects otherwise valid orders.

### Paged order blocks

- Good, because blocks preserve most array sweep locality while avoiding one
  huge reservation per level.
- Good, because a pool can bound allocation once maximum active block count is
  known.
- Bad, because block chaining, tombstone accounting, iterators, and reclamation
  create a large local bug surface.
- Bad, because it needs benchmark evidence and sizing assumptions before it
  should replace the simpler optimized book.

## More Information

- [`docs/engine-specs.md`](../engine-specs.md) -- observable matching-engine
  behavior.
- [`0004-keep-order-book-as-sorted-price-level-maps-matrix.html`](0004-keep-order-book-as-sorted-price-level-maps-matrix.html)
  -- comparative matrix for the options.
