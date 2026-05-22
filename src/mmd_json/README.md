# mmd_json

JSON rendering for normalized `mmd` market-data events. It preserves the phase
5 JSONL record shape while the market-data codecs move behind the modular
boundary.

## Components

- A JSON encoder for every current `mmd::message` alternative.
- Enum token helpers matching the existing market-data JSON contract.

