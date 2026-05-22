# client

Command-line UDP sender for scenario inputs and local demos.

```bash
printf 'N, 1, IBM, 10, 100, B, 1\nF\n' | ./_build/debug/client --host 127.0.0.1 --port 1234
```

If `--input` is omitted, the process reads commands from stdin. Each non-empty
line is decoded through `order_routing::csv_decoder`, sent through
`order_client::client`, and emitted as one UDP datagram to the configured
server endpoint.
