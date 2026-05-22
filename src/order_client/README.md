# order_client

Typed UDP client library for order-entry commands. It accepts
`order_entry::new_order_single`, `order_entry::replace_order`,
`order_entry::cancel_order`, and `order_entry::flush`, encodes them into
the inbound JSON command protocol, and sends each command as one UDP datagram.

## Components

- `order_client::json_encoder` maps typed order-entry requests to JSON command
  records.
- `order_client::udp_sender` owns the Boost.Asio UDP socket, endpoint, and
  datagram-size guard.
- `order_client::client` is the public API that combines encoding and sending.

The library is intentionally small: it gives examples and local tools a typed
way to drive the server without depending on shell-specific UDP behavior.
