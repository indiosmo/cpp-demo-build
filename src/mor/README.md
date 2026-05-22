# mor

Normalized Matching Engine Order Routing domain. It is the stable order-routing
surface that future clients, risk stages, FIX codecs, and matching-engine
adapters can share.

## Components

- Request messages: `new_order_single`, `replace_request`, `cancel_request`,
  and `flush_request`.
- Event messages: `execution_report`, `cancel_reject`, and `parser_reject`.
- Source, sink, and pipeline-stage callback interfaces for bidirectional
  routing.
- Compatibility conversions to and from the current `order_entry` messages.

