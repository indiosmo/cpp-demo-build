# order_client

Typed UDP client library for order-entry commands. It accepts
`order_routing::new_order`, `order_routing::cancel_order`, and
`order_routing::flush`, encodes them into the inbound CSV command protocol, and
sends each command as one UDP datagram.

## Components

- `order_client::csv_encoder` maps typed routing requests to CSV command
  records.
- `order_client::udp_sender` owns the Boost.Asio UDP socket and endpoint.
- `order_client::client` is the public API that combines encoding and sending.

The library is intentionally small: it gives examples and local tools a typed
way to drive the server without depending on shell-specific UDP behavior.
