# morfix_quickfix

Glue layer between `morfix`, `ospec`, and the local QuickFIX-compatible engine
boundary. The B3 codecs map the first order-entry request and response slice
between canonical `morfix` messages and local `quickfix_fix::message` records.

## Components

- Initiator and acceptor codec interfaces.
- B3 initiator request encoding and event decoding.
- B3 acceptor request decoding and event encoding.
- Structured error codes for unsupported codec operations.
