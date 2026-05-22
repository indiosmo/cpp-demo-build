# morfix_quickfix

Glue layer between `morfix`, `ospec`, and the local QuickFIX-compatible engine
boundary. The B3 codecs compile and return typed scaffold failures until the
real FIX field mapping lands.

## Components

- Initiator and acceptor codec interfaces.
- B3 initiator and acceptor codec stubs.
- Structured error codes for unsupported and unimplemented codec operations.

