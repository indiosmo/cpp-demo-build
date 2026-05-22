# 5. Layer codecs behind normalized routing and market-data domains

**Status:** accepted

**Date:** 2026-05-22

## Context and Problem Statement

The active demo path uses JSON over UDP for order-entry commands and JSONL on
stdout for market-data records. That is a useful local workflow, but the
portfolio direction needs room for FIX, B3-specific normalization, and future
transport choices without pushing venue or wire-format decisions into the
matching engine.

The matching engine already has separate order-entry and market-data domains.
The next boundary is a codec stack that keeps normalized business messages
separate from canonical FIX-shaped messages, venue-specific tag/value rules,
the concrete FIX engine boundary, and delivery transports.

## Decision Drivers

- Preserve the current JSON demo path while new codec modules land.
- Keep the matching engine on typed domain values rather than wire records.
- Let B3-specific tags, enum values, and field naming live in one venue profile.
- Keep the scaffold package-free while the QuickFIX-facing boundary takes
  shape.
- Make future FIX, JSON, binary, stdout, WebSocket, and FIX-session work add
  modules instead of changing matching behavior.
- Keep request correlation and FIX lifecycle bookkeeping out of the matching
  engine.

## Considered Options

| Option | Complexity | Blast radius | Maintenance | Result |
|---|---:|---:|---:|---|
| Extend current JSON modules in place | Low | Low | Poor | Keeps demo simple but mixes codec evolution into existing domains. |
| Add one shared codec module | Medium | Medium | Medium | Centralizes codecs but couples order routing, market data, venue rules, and transport. |
| Layer normalized domains, canonical FIX, venue specs, engine boundary, and glue | Medium | Medium | Good | Chosen: clear ownership with buildable migration points. |

## Decision Outcome

Chosen path: **layer codecs behind normalized order-routing and market-data
domains**.

Order routing uses:

- `mor` for normalized Matching Engine Order Routing messages, interfaces, and
  compatibility conversions from `order_entry`;
- `morfix` for canonical FIX-shaped order-routing messages and lifecycle
  scaffolding;
- `ospec` for B3 tag constants, value normalization, and reference anchors;
- `quickfix_fix` for a package-free local QuickFIX-compatible message/session
  boundary;
- `morfix_quickfix` for B3 codec glue between `morfix`, `ospec`, and the FIX
  engine boundary.

Market data uses:

- `mmd` for normalized Matching Engine Market Data events and compatibility
  conversions from `market_data`;
- `mmd_json` for JSON rendering that preserves the current JSONL record shape;
- `mmdfix` for FIX-shaped market-data records;
- `mmd_transport` for encoded-record delivery sinks.

The B3 profile starts from the fields already exercised by the lab and uses
`b3-entrypoint-messages-8.4.2.xml` and
`b3-market-data-messages-2.2.0.xml` at the repository root as reference inputs.

### Consequences

- Good, because the matching engine continues to consume and emit typed values.
- Good, because JSON, FIX, and future binary codecs have separate ownership.
- Good, because B3 normalization is testable without a FIX engine runtime.
- Good, because the QuickFIX-facing boundary supports a package-free local
  message/session loop.
- Good, because the current phase 5 JSON workflow remains buildable while
  compatibility adapters carry the new names.
- Bad, because the repository temporarily contains both current domain names
  (`order_entry`, `market_data`) and normalized target names (`mor`, `mmd`).
- Bad, because scaffold modules add surface area before every adapter has full
  production behavior.

### Confirmation

The decision is in effect when:

- `mor` and `mmd` compile as normalized compatibility domains.
- `morfix`, `mmdfix`, and `ospec::b3` compile and have focused unit coverage.
- `quickfix_fix` and `morfix_quickfix` expose typed interfaces and support a
  local B3 new/replace/cancel request and execution-report/cancel-reject loop.
- `mmd_json` preserves the current JSON market-data record shape through the
  normalized market-data boundary.
- The existing server and client still use the phase 5 JSON path and the full
  debug test suite remains green.

### Follow-ups

- Move the active order-entry JSON command codec behind `mor`.
- Move the active market-data JSON publisher behind `mmd_json`.
- Broaden B3 FIX field mapping in `morfix_quickfix` beyond the first
  order-entry request/response slice.
- Add response-channel support for order-entry lifecycle events when the client
  becomes interactive.
