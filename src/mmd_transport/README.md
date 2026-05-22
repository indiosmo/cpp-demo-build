# mmd_transport

Market-data delivery transport boundaries. Transports consume already encoded
records, so payload format and delivery mechanism can evolve independently.

## Components

- A sink interface for encoded market-data records.
- A capture sink for tests and local wiring spikes.

