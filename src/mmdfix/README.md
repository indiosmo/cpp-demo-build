# mmdfix

Canonical FIX-shaped market-data messages. The first scaffold covers the
engine's active trade and MBO book-update events so B3-specific normalization
can be added without changing the matching-engine domain.

## Components

- FIX-shaped incremental refresh records for book updates.
- FIX-shaped trade capture records for trades.
- Conversion helpers from normalized `mmd` events.

