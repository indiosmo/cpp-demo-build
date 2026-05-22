# ospec

Venue-specific order-routing and market-data specifications. The first profile
is `ospec::b3`, using the B3 entrypoint and market-data XML files at the
repository root as reference inputs for names, tags, and value mappings.

## Components

- B3 tag constants for the simplified order-routing and market-data surface.
- B3 value normalization for order side, order type, time-in-force, lifecycle
  status, reject reason, book side, update action, and trade condition.
- Reference anchors for `b3-entrypoint-messages-8.4.2.xml` and
  `b3-market-data-messages-2.2.0.xml`.

