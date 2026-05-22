# client

Command-line UDP sender for scenario inputs and local demos.

```bash
printf '{"message_type":"new_order_single","client_id":1,"cl_ord_id":1,"symbol":"IBM","side":"buy","order_qty":100,"price":10}\n{"message_type":"flush"}\n' | ./_build/debug/client --host 127.0.0.1 --port 1234
```

If `--input` is omitted, the process reads commands from stdin. Each non-empty
line is decoded through `order_entry::json_decoder`, sent through
`order_client::client`, and emitted as one UDP datagram to the configured
server endpoint.
