# 3. Parse CSV as fixed-shape commands

**Status:** accepted

**Date:** 2026-05-14

**Companion:** comparison across fixed-shape splitting, a general tokenizer, and
a parser library lives in
[`0003-parse-csv-as-fixed-shape-commands-matrix.html`](0003-parse-csv-as-fixed-shape-commands-matrix.html).

## Context and Problem Statement

The order-routing stage receives one UDP datagram and turns it into one typed
request: new order, cancel, or flush. The sample wire protocol defines three
record markers, each with a fixed field count.

The scenario fixtures use well-formed command rows. Malformed data is still
represented as a domain rejection at the session boundary, but the CSV decoder
itself should optimize for the documented protocol rather than arbitrary CSV
input.

## Decision Drivers

- The input grammar is closed: `N`, `C`, and `F` are the only record types.
- Each record type has a fixed field count.
- The protocol uses unquoted, comma-separated records with one request per
  datagram.
- The routing stage is on the processing path, so avoidable allocation and
  broad parser dependencies should be avoided.
- Parser failures need a path to domain rejection when the decoder contract
  expands.
- The implementation should be easy to inspect against the protocol.

## Considered Options

1. **Fixed-shape `std::string_view` split.** Switch on the first byte, split the
   known field count for that marker, and parse fields into strong types.
2. **General local tokenizer.** Keep a reusable token stream over
   `std::string_view` and let each message decoder pull fields one by one.
3. **CSV/parser library.** Use Boost.Tokenizer, a header-only CSV parser, or a
   parser combinator.

## Decision Outcome

Chosen option: **fixed-shape `std::string_view` split**.

The parser mirrors the protocol: byte zero selects the request type, and the
message-specific decoder consumes the exact fields that record defines. Numeric
fields use `lab::from_chars`, so integer parsing follows the same result and
error vocabulary as the rest of the project.

Inside the chosen shape, the splitter uses a standard helper to produce
`std::string_view` fields rather than a hand-rolled cursor. The trade is one
small allocation per parse against less local parser code and a decoder that is
easy to review. A benchmark-driven follow-up can switch the splitter to a lazy
tokenizer or stack-resident field buffer without changing the record-level
shape.

Helpers inside the CSV decoder treat the well-formed grammar as a precondition.
Field count, marker validity, side tokens, symbol length, and numeric
parseability are protocol contracts and are asserted with `LAB_ASSERT`. The
decoder boundary keeps its `lab::result<request>` return so a future decoder
can carry structured parse errors without changing the boundary.

### Consequences

- Good, because fields are `std::string_view` slices into the datagram.
- Good, because the code structure matches the sample protocol: dispatch by
  marker, split by fixed arity, construct the typed request variant.
- Good, because no CSV parser dependency is introduced for dialect features the
  protocol does not use.
- Good, because numeric parsing stays inside the project's `lab::from_chars`
  vocabulary.
- Bad, because the decoder is intentionally not a general CSV implementation.
- Bad, because quoted fields, escaped commas, or variable-width records require
  a new parser decision.
- Bad, because precondition violations assert in debug builds; the CSV decoder
  relies on the scenario producer and protocol documentation to keep records
  well-formed.
- Bad, because the current splitter allocates a field vector per parse.

### Confirmation

The decision is in effect when:

- The decoder switches on the first byte of the trimmed payload.
- Each record helper splits the fixed field count for its marker and asserts
  the count against the protocol precondition.
- Field helpers return domain types directly instead of threading
  `lab::result` through every parse step.
- Numeric conversion goes through `lab::from_chars`.
- The decoder boundary keeps a result-returning API for future structured
  rejection paths.

### Follow-ups

- Revisit the splitter after end-to-end benchmarks include order-routing cost.
  If allocation shows on the hot path, replace the field vector with a lazy
  tokenizer or stack-resident field buffer.
- If the protocol grows beyond the closed `N`/`C`/`F` set, lift the helper
  preconditions into structured parse errors.

## Pros and Cons of the Options

### Fixed-shape `std::string_view` split

- Good, because it fits the closed wire grammar exactly.
- Good, because the record shape is visible in the code.
- Good, because it avoids parser-library dependency weight.
- Bad, because it is not suitable for full CSV dialect support.

### General local tokenizer

- Good, because it can stay allocation-free.
- Good, because parser behavior stays in the project.
- Bad, because it models a token stream for a fixed-tuple protocol.
- Bad, because cursor behavior is harder to read than fixed field extraction.

### CSV/parser library

- Good, because it becomes attractive if quoting, escaping, or multiline records
  become required.
- Good, because mature parsers handle dialect edge cases better than local code.
- Bad, because it solves problems outside the sample protocol.
- Bad, because library errors still need translation into domain rejections.

## More Information

- [`docs/engine-specs.md`](../engine-specs.md) -- observable matching-engine
  behavior and sample protocol.
- [`0003-parse-csv-as-fixed-shape-commands-matrix.html`](0003-parse-csv-as-fixed-shape-commands-matrix.html)
  -- comparison of the parser options.
