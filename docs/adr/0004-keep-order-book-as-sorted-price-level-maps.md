# 4. Keep the order book as sorted flat price-level maps with intrusive pooled orders

**Status:** accepted

**Date:** 2026-05-22

**Companion:** comparison across the order-book data-structure options lives in
[`0004-keep-order-book-as-sorted-price-level-maps-matrix.html`](0004-keep-order-book-as-sorted-price-level-maps-matrix.html).

## Context and Problem Statement

The matching engine needs a per-symbol limit order book that preserves
price-time priority, exposes top-of-book aggregates for market-data updates,
matches marketable orders against the opposite side, and removes resting orders
by `(client_id, cl_ord_id)` on cancel.

The shape must support direct cancellation through an identity index, stable
resting-order addresses for that index, an O(1) read of top-of-book aggregate
quantity for the market-data path, and sparse price levels created only when
liquidity rests at that price.

## Decision Drivers

- Preserve the price-time priority and top-of-book semantics documented in
  [`docs/engine-specs.md`](../engine-specs.md).
- Make cancellation direct from the cancel request's order identity.
- Handle sparse and unknown price levels without fixed tick-range reservation.
- Read top-of-book aggregate quantity in O(1) for market-data updates.
- Erase the fully consumed prefix of the opposing side map with one range erase
  after matching walks outward from top-of-book.
- Reuse one resting-order pool across symbols so resident memory tracks
  aggregate depth rather than the sum of per-symbol caps.
- Preserve book references while matching callbacks fire.
- Keep resting node addresses stable so the identity index can store direct
  pointers.
- Reclaim resting memory at shutdown through the pool.
- Prefer Boost-backed storage machinery over bespoke allocator and list code.

## Considered Options

1. **Sorted maps of FIFO vectors.** Store bids and asks in side-specific sorted
   maps from price to FIFO vectors of resting orders.
2. **Sorted flat side maps with intrusive lists and an object pool.** Store
   each price level as an intrusive FIFO of pool-owned order nodes, with a
   `(client_id, cl_ord_id) -> order_node*` identity index.
3. **Preallocated vectors with tombstones.** Reserve bounded contiguous storage
   for each price level, mark cancelled entries inactive, and skip tombstones
   during matching.
4. **Paged order blocks.** Store each price level as a chain of fixed-size
   blocks allocated from a pool, with tombstones and block reclamation.

## Decision Outcome

Chosen path: **sorted `boost::container::flat_map` side maps over intrusive
per-level FIFOs of pool-owned order nodes**.

Each book stores bids and asks in side-specific
`boost::container::flat_map<price, price_level, comparator>` maps. The bid side
sorts descending and the ask side sorts ascending, so `begin()` is top-of-book
on either side.

Each `price_level` stores same-price liquidity as a
`boost::intrusive::list<order_node>` in arrival order and maintains a running
`total_remaining` quantity on every place, cancel, and fill. The market-data
path reads top-of-book aggregate quantity in O(1).

Resting `order_node`s live in a single `boost::pool<>` owned by the engine and
shared across every book, so a freed slot in a shallow symbol funds a deep one.
Books themselves live in `boost::unordered_node_map<symbol, order_book>` because
the matching loop holds a book reference across callbacks and the registry must
preserve that reference under rehash.

Cancellation and replacement go through a cross-symbol
`boost::unordered_flat_map<order_key, order_node*>` identity index keyed by
`(client_id, cl_ord_id)`. The index is flat because its value is already a
stable pointer. Direct cancel resolves the index, unlinks the node from its
level, decrements `total_remaining`, erases the level if it became empty,
returns the node to the pool, and erases the index entry.

Matching walks the opposing side from `begin()` outward, drains each crossing
level through the intrusive list, and erases the fully consumed prefix with one
`side_map.erase(side_map.begin(), it)` at the end of the walk. Per-element erase
on `flat_map` would shift the tail on every iteration; bulk range erase shifts
once.

### Consequences

- Good, because side-map ordering and per-level FIFO order express price-time
  priority directly.
- Good, because sparse prices fall out: a level is created only when liquidity
  rests at that price.
- Good, because maintained per-level totals make top-of-book aggregate quantity
  a single read.
- Good, because pooled nodes plus the identity index give O(1) cancellation by
  `(client_id, cl_ord_id)` regardless of side or price.
- Good, because one shared pool puts resting memory under a single budget that
  tracks aggregate depth across symbols.
- Good, because matching erases fully consumed flat-map levels with one range
  erase rather than per-element erase.
- Good, because shutdown reclaims resting memory through the pool in one shot:
  the payload is trivially destructible, and the intrusive hook uses
  `normal_link`.
- Bad, because the pool needs sizing: under-provisioning forces growth
  allocation on the hot path, and over-provisioning wastes resident memory.
- Bad, because intrusive hooks plus raw `order_node*` values in the identity
  index put lifetime correctness on the engine: every place a node leaves the
  book must also leave the index, and vice versa.
- Bad, because pointer-chase through intrusive lists costs cache locality on
  deep sweeps compared with a contiguous-storage layout; accepted as the price
  of direct cancellation and stable node addresses.
- Bad, because the book registry and the identity index intentionally use
  different unordered-map flavors for different reasons: reference stability
  for books and cache density for stable pointers.

### Confirmation

The decision is in effect when:

- Each book stores bids as
  `boost::container::flat_map<price, price_level, std::greater<>>` and asks as
  `flat_map<price, price_level, std::less<>>`, so `begin()` is top-of-book for
  either side.
- Each `price_level` is a `boost::intrusive::list<order_node>` plus a
  `total_remaining` quantity updated on every place, cancel, and fill.
- Resting `order_node`s are allocated from one engine-wide `boost::pool<>` that
  outlives every book.
- The engine indexes resting orders by `(client_id, cl_ord_id)` in a
  `boost::unordered_flat_map<order_key, order_node*>`; books live in a
  `boost::unordered_node_map<symbol, order_book>` so matching can hold a book
  reference across callbacks.
- Matching walks the opposing side from `begin()` outward, drains crossing
  levels through their intrusive lists, releases fully consumed maker nodes
  back to the pool, and removes the consumed prefix with one
  `erase(begin(), it)` rather than per-element erase.
- Cancellation resolves the identity index, unlinks the node, decrements the
  level total, erases the level if empty, erases the index entry, and returns
  the node to the pool.
- Tests cover price ordering, FIFO preservation, aggregate quantity updates,
  partial fills, full level removal, cancellation at front, middle, and back
  positions, unknown cancels, pool and index consistency, and book-reference
  stability across callbacks.

### Follow-ups

- Require focused latency and locality evidence before changing the order-book
  storage shape.
- Reconsider paged order blocks only if evidence shows sweep locality dominates
  direct cancellation and stable node addresses at representative book depths.
- Keep pool sizing tied to configured expected resting depth, and revisit the
  growth policy if production-like scenarios show burst allocation on the
  matching path.

## Pros and Cons of the Options

### Sorted maps of FIFO vectors

- Good, because it matches the matching semantics with standard containers.
- Good, because map ordering and vector append make price-time priority visible.
- Good, because it handles arbitrary sparse prices.
- Bad, because cancellation scans resting orders.
- Bad, because insertions can allocate and vector erasure can move surviving
  entries.

### Sorted flat side maps with intrusive lists and object pool

- Chosen.
- Good, because direct order pointers and intrusive unlinking give direct
  cancellation.
- Good, because a shared pool removes per-order heap allocation after warm-up
  and lets freed slots from one symbol fund another.
- Good, because flat side maps keep sparse price handling simple.
- Good, because matching erases the fully consumed prefix of flat-map levels
  with one range erase.
- Good, because one shared pool gives stable resting-order addresses and
  amortizes capacity across symbols.
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
- Bad, because block accounting competes with direct cancellation and
  `total_remaining` maintenance for the same mutation path.
- Bad, because it needs evidence and sizing assumptions before it should
  replace the simpler book.

## More Information

- [`docs/engine-specs.md`](../engine-specs.md) -- observable matching-engine
  behavior.
- [`0004-keep-order-book-as-sorted-price-level-maps-matrix.html`](0004-keep-order-book-as-sorted-price-level-maps-matrix.html)
  -- comparative matrix for the options.
