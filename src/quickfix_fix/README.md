# quickfix_fix

Local QuickFIX-compatible FIX engine boundary. It gives the codec glue a typed
message and session interface through a package-free scaffold for this phase.

## Components

- A simple FIX message abstraction with `MsgType` and ordered fields.
- A session interface with inbound message, reject, and outbound send hooks.
