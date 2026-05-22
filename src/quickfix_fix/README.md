# quickfix_fix

Local QuickFIX-compatible FIX engine boundary. It gives the codec glue a typed
message, text codec, and in-memory session interface through a package-free
implementation for the local FIX loop.

## Components

- A simple FIX message abstraction with `MsgType` and ordered fields.
- A text codec for FIX-style `tag=value` records with either SOH or printable
  delimiters.
- A session interface with inbound message, reject, and outbound send hooks.
- A connected in-memory initiator/acceptor session pair for deterministic
  codec tests.
