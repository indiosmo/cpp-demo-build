# client

Command-line UDP sender for scenario inputs and local demos.

```bash
./_build/debug/client examples/configs/client.json
```

The config file selects the UDP endpoint and input source. Each non-empty line
is decoded through `order_entry::json_decoder`, sent through
`order_client::client`, and emitted as one UDP datagram to the configured
server endpoint.
