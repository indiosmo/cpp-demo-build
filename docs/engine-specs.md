# Engine Specification

The matching engine accepts `order_entry::request` values. It emits
order-entry lifecycle events on `on_order_entry` and market-data events
on `on_market_data`. The runtime guarantees a single thread drives it.

The request variant has four alternatives:

- `new_order_single` -- submit a buy or sell order.
- `replace_order` -- amend a live order by `orig_cl_ord_id`.
- `cancel_order` -- cancel a live order by `orig_cl_ord_id`.
- `flush` -- clear every book silently.

## Wire Protocol

The UDP protocol carries one compact JSON object per datagram. Requests use a
`message_type` discriminator with values `new_order_single`, `replace_order`,
`cancel_order`, and `flush`, plus field names matching the typed
`order_entry` model. The client and server both route through the typed model:
JSON is a boundary codec, not the matching-engine data structure.

The codec stack separates the active JSON workflow from future venue and
transport work:

- `mor` is the normalized order-routing surface that wraps the current
  `order_entry` messages during the migration.
- `morfix`, `ospec`, `quickfix_fix`, and `morfix_quickfix` form the local
  FIX/B3 order-routing stack. The current working slice covers
  new/replace/cancel requests plus execution reports and cancel rejects over an
  in-memory QuickFIX-compatible session boundary.
- `mmd` is the normalized market-data surface that wraps the current
  `market_data` events during the migration.
- `mmd_json`, `mmdfix`, and `mmd_transport` own market-data encoding and
  delivery boundaries.

## Data Structures

### Per-Symbol Order Book

One book is allocated for each configured symbol:

- **Bids** are price-ordered descending; the front is the best bid.
- **Asks** are price-ordered ascending; the front is the best ask.
- Within one price level, orders keep arrival order.

The production book is
[`matching_engine/order_book.hpp`](../src/matching_engine/matching_engine/order_book.hpp):
sorted price-level maps over intrusive lists of pool-allocated nodes.

### Identity Index

A single cross-symbol map from `(client_id, cl_ord_id)` to the resting
node locates live orders for duplicate checks, cancels, and replaces.
The per-symbol books carry no identity index of their own.

## Request Handling

### new_order_single

The engine rejects duplicate keys and unknown symbols. Accepted orders
produce an `order_entry::execution_report` with `exec_type::new_order`
before matching begins.

An aggressing buy crosses the ask side; an aggressing sell crosses the
bid side. Trades execute at the maker's resting price. The engine emits:

- one `market_data::trade` per match;
- one `market_data::execution_summary` when any liquidity is removed;
- one `order_entry::execution_report` with `exec_type::trade` for the
  aggressor when any quantity fills;
- `market_data::mbo_book_update` for the consumed side when liquidity is
  removed;
- `market_data::mbo_book_update` for the order's own side when a residual
  limit order rests at the new best.

`price == 0` is the demo JSON sentinel for a market order. Market
remainders are dropped as IOC. Limit remainders rest.

### replace_order

`replace_order` locates the live order by `(client_id, orig_cl_ord_id)`.
If the original cannot be found, or the replacement `cl_ord_id` would
duplicate another live order, the engine emits `order_entry::cancel_reject`.

A valid replace removes the original resting order, emits any resulting
book update, then processes the replacement terms as a fresh order under
the new `cl_ord_id`.

### cancel_order

`cancel_order` locates the live order by `(client_id, orig_cl_ord_id)`.
If the key is unknown, the engine emits `order_entry::cancel_reject`.
Otherwise it removes the order, erases the identity entry, returns the
node to the pool, emits an `execution_report` with `exec_type::canceled`,
and emits an `mbo_book_update` when the cancelled order was at the best
price for its side.

### flush

`flush` drains every book and emits nothing. The configured symbol set
remains available for later requests.

## Domain Boundaries

Order-entry messages and market-data messages are distinct domains.
Fields such as `price`, `quantity`, `side`, `security_id`, and `symbol`
use separate strong types in each namespace. Conversion happens only at
the matching-engine boundary.
