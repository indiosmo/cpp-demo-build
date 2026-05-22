# Run Local Matching Engine

The local run environment is two host processes on loopback. The `server`
loads its UDP endpoint, matching symbols, logger, and thread settings from a
JSON config file, decodes JSON order-entry commands, runs the matching engine,
and writes logs plus market-data JSON records to stdout. The `client` loads
its target endpoint and input source from a JSON config file, then sends each
JSONL command as one UDP datagram.

Run `./setup.sh` once after cloning. Build outputs live under `_build/<preset>/`,
and direct binary runs should source `scripts/setenv.sh` so the local C++
toolchain libraries are on the runtime path.

## Build The Binaries

1. Build the server and client targets:

   ```bash
   ./build.sh debug server
   ./build.sh debug client
   ```

2. Source the toolchain environment in each shell that runs a binary directly:

   ```bash
   source scripts/setenv.sh
   ```

## Run In Two Terminals

1. Start the server in the first terminal:

   ```bash
   source scripts/setenv.sh
   ./_build/debug/server examples/configs/server.json
   ```

   The server logs startup messages and waits for UDP datagrams.

2. Send the bundled crossing-orders scenario from a second terminal:

   ```bash
   source scripts/setenv.sh
   ./_build/debug/client examples/configs/client.json
   ```

3. Watch the first terminal. The server prints market-data JSON records for
   book updates, a trade, and an execution summary.

4. Stop the server with `Ctrl-C`.

## Run A Repeatable Smoke Test

1. Run the smoke test from the repository root:

   ```bash
   source scripts/setenv.sh

   server_log=_build/debug/local-smoke.log
   market_data_log=_build/debug/local-smoke.market-data.jsonl

   ./_build/debug/server examples/configs/server.json > "$server_log" 2>&1 &
   server_pid=$!

   sleep 1
   ./_build/debug/client examples/configs/client.json
   sleep 1

   kill -TERM "$server_pid"
   wait "$server_pid" || true

   grep '^{' "$server_log" > "$market_data_log"
   diff -u examples/expected/crossing-orders-market-data.jsonl "$market_data_log"
   ```

2. Confirm `diff` exits without output. That means the client reached the
   server and the market-data records match the bundled expected sequence.

3. Inspect `_build/debug/local-smoke.log` when the comparison fails. Lines that
   start with `{` are market-data records; other lines are runtime logs.

## Send Commands From Stdin

1. Start the server:

   ```bash
   source scripts/setenv.sh
   ./_build/debug/server examples/configs/server.json
   ```

2. Create a stdin client config in the build tree:

   ```bash
   cat > _build/debug/client-stdin.json <<'JSON'
   {
     "order_client": {
       "sender": {
         "endpoint": {
           "address": "127.0.0.1",
           "port": 1234
         },
         "max_datagram_size": 65535
       }
     },
     "input": {
       "type": "stdin"
     }
   }
   JSON
   ```

3. Pipe JSONL commands to the client from another terminal:

   ```bash
   source scripts/setenv.sh
   printf '{"message_type":"new_order_single","client_id":1,"cl_ord_id":1,"symbol":"IBM","side":"buy","order_qty":100,"price":10}\n{"message_type":"flush"}\n' \
     | ./_build/debug/client _build/debug/client-stdin.json
   ```

4. Stop the server with `Ctrl-C`.

## Troubleshooting

- `GLIBCXX_... not found`: run `source scripts/setenv.sh` in the shell before
  starting `server` or `client`.
- `Address already in use`: choose another UDP port and put the same value in
  the server and client config files.
- No market-data records: confirm the client and server use the same host and
  port, and confirm the scenario uses a symbol configured by the server.

## Files

- Scenario input: `examples/scenarios/crossing-orders.jsonl`
- Expected market data: `examples/expected/crossing-orders-market-data.jsonl`
- Server config: `examples/configs/server.json`
- Client config: `examples/configs/client.json`
- Server binary: `_build/debug/server`
- Client binary: `_build/debug/client`
