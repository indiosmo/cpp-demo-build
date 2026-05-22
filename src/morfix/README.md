# morfix

Canonical FIX-shaped rendering of `mor` order-routing messages. This module
owns FIX order lifecycle vocabulary that does not belong in the matching
engine, such as request correlation and `ExecID` allocation.

## Components

- FIX-shaped requests for new, replace, and cancel order flow.
- FIX-shaped lifecycle events for execution reports and cancel rejects.
- Conversion helpers from normalized `mor` messages.
- A small lifecycle-state scaffold for request correlation.

