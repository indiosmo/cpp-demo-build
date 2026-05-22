# 3. Parse CSV as fixed-shape commands

**Status:** accepted

**Date:** 2026-05-14

**Companion:** comparison across fixed-shape splitting, a general tokenizer, and a parser library lives in [`0003-parse-csv-as-fixed-shape-commands-matrix.html`](0003-parse-csv-as-fixed-shape-commands-matrix.html).

## Context and Problem Statement

The order routing stage receives one UDP datagram and turns it into one typed command: new order, cancel, or flush. `EXERCISE.md` defines three CSV command shapes, each with a marker and a fixed field count, and says hidden tests use the same format as the provided scenarios.

The provided `test/*/in.csv` files do not contain malformed command rows after ignoring blank trailing lines. That matters: malformed data is not a primary design target for this submission. The session still has a rejection path for defensive handling, but the parser should optimize for the documented exercise grammar rather than arbitrary CSV input.

## Decision Drivers

- The input grammar is closed: `N`, `C`, and `F` are the only record types.
- Each record type has a fixed field count.
- The exercise does not require quoted fields, escaped commas, multiline records, or recovery from hostile input.
- The routing stage is on the processing path, so avoidable allocations and broad parser dependencies should be avoided.
- Parser failures still need to surface as domain rejections when they happen.
- **Delivery time is constrained.** The submission window is tight; parser complexity that does not pay back inside the exercise scope is deferred. Where a stock library helper short-cuts the implementation, the implementation accepts the helper -- even at a measurable performance cost -- and flags the call site for a perf-driven follow-up.

## Considered Options

1. **Fixed-shape `std::string_view` split.** Switch on the first byte, split the known field count for that marker, parse fields into strong types.
2. **General local tokenizer.** Keep a reusable token stream over `std::string_view` and let each message decoder pull fields one by one.
3. **CSV/parser library.** Use Boost.Tokenizer, a header-only CSV parser, or a parser combinator.

## Decision Outcome

Chosen option: **fixed-shape `std::string_view` split**.

The parser mirrors the protocol: byte zero selects the command type, and the message-specific decoder consumes the exact number of fields that record defines. Numeric fields use `kraken::from_chars`, ported into the local utility library so integer parsing follows the same `kraken::result` error vocabulary as the rest of the code.

Inside the chosen shape, the splitter is `boost::algorithm::split` into a `std::vector<std::string_view>` rather than a hand-rolled cursor or a non-allocating `boost::tokenizer`. The trade is one heap allocation per parse against fewer lines, fewer edge cases to argue about, and faster sign-off under the submission deadline. The call site carries an `IMPROVEMENT:` comment so a perf-driven follow-up can swap in `boost::tokenizer` (lazy, non-allocating) or a stack-resident `boost::container::static_vector<N+1>` when -- or if -- benchmarks indicate the alloc matters. This sub-decision is the place the time-pressure driver actually bites; the surrounding shape is unchanged.

Every helper inside `csv_decoder.cpp` treats the well-formed grammar as a precondition. Field count, marker validity, numeric parseability, `B`/`S` side tokens, and symbol length are documented contracts and asserted with `KRAKEN_ASSERT` rather than threaded through `kraken::result`. The decoder boundary keeps its `kraken::result<command>` return so a future non-CSV decoder can still carry richer errors; the CSV implementation simply never produces them under this contract. If the protocol later grows into real CSV with quoting or escaping, this ADR should be superseded and the helpers re-promoted to result-returning forms.

### Consequences

- Good, because fields are `std::string_view` slices into the datagram and integers are parsed through `kraken::from_chars`, so no per-field copying happens.
- Good, because the code structure matches the exercise grammar directly: dispatch by marker, split by fixed arity, construct the typed command variant.
- Good, because the helpers return concrete types instead of `kraken::result<T>`; the well-formed precondition removes the per-step error plumbing and the matching parse-error formatting.
- Good, because no CSV parser dependency is introduced for dialect features the protocol does not use.
- Bad, because the parser is intentionally not a general CSV implementation.
- Bad, because adding quoted fields, escaped commas, or variable-width records would require both a new ADR and re-introducing the result-typed helpers the precondition collapsed.
- Bad, because precondition violations abort in debug (`KRAKEN_ASSERT`) and propagate as undefined behaviour in `NDEBUG`; protocol drift now surfaces as a crash or wrong output rather than a structured rejection. The CSV-only contract pays for this, but the structured rejection path on the session boundary stays in place so a future producer can re-enable structured errors without redesign.
- Bad, because the splitter heap-allocates a `std::vector<std::string_view>` per parse. Acceptable under the well-formed input and time-constrained delivery assumptions; flagged in code for revisit and listed as an open question below.

### Confirmation

The decision is in effect when:

- The decoder entry point switches on the first byte of the trimmed payload and dispatches to a message-specific helper for each record type.
- Each helper splits on the fixed field count declared by its record type and asserts the count against the well-formed precondition. The split goes through a standard library helper rather than a hand-rolled tokenizer, and the call site carries an `IMPROVEMENT:` marker naming the lazy follow-ups.
- The per-field helpers (numeric parse, side token, symbol token, per-marker decoders) return their domain types directly under the precondition; they do not thread `kraken::result` through every step.
- Numeric conversion goes through the project's `kraken::from_chars` alias.
- The provided `test/*/in.csv` scan remains consistent with the documented command shapes.

### Follow-ups

- Revisit the splitter when end-to-end benchmarks are wired up. If the per-parse allocation shows on the routing-stage hot path, swap `boost::algorithm::split` for `boost::tokenizer` (lazy, non-allocating) or splice tokens into a stack-resident `boost::container::static_vector<N+1>`. Until then, the `IMPROVEMENT:` marker in `csv_decoder.cpp` is the audit trail.
- If the grammar grows beyond the closed `N`/`C`/`F` set, or if a non-test transport starts feeding the decoder, lift the helpers back to `kraken::result<T>` (or move the result wrapping up to `csv_decoder::decode`) so precondition violations translate into a structured rejection on the session boundary again.

## Pros and Cons of the Options

### Fixed-shape `std::string_view` split

- Good, because it fits the closed exercise grammar exactly.
- Good, because the shape itself is allocation-free; only the splitter sub-choice introduces an allocation, and the option is open to swap to `boost::tokenizer` or a `static_vector<N+1>` later.
- Good, because it avoids parser-library dependency weight.
- Good, because it is easy to read against `EXERCISE.md`.
- Bad, because it is not suitable for full CSV dialect support.

### General local tokenizer

- Good, because it can stay allocation-free.
- Good, because it keeps parser behavior inside the project.
- Bad, because it models a token stream for a protocol that is really fixed tuples.
- Bad, because cursor behavior obscures the record shape more than a fixed split does.

### CSV/parser library

- Good, because it would be the right direction if quoting, escaping, or multiline records became required.
- Good, because mature parsers handle dialect edge cases better than local code.
- Bad, because it solves problems outside the exercise contract.
- Bad, because available tokenizer-style libraries may materialize owned strings or require domain-error translation anyway.
- Bad, because it broadens the dependency surface for little benefit under the well-formed input assumption.

## More Information

- [`EXERCISE.md`](../../EXERCISE.md) -- input command shapes and the hidden-test format statement.
- [`DESIGN.md`](../../DESIGN.md) -- routing-stage architecture and parser decision summary.
- [`0003-parse-csv-as-fixed-shape-commands-matrix.html`](0003-parse-csv-as-fixed-shape-commands-matrix.html) -- comparison of the parser options.
