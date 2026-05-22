# AGENTS.md

Guidance for agents working in this repository.

This is the senior order book submission for the Kraken C++ interview
exercise. The brief is [`EXERCISE.md`](EXERCISE.md): a multi-threaded
UDP-driven matching engine that publishes CSV records to stdout. The
pipeline shape and threading model are in [`README.md`](README.md);
the per-library responsibilities are in the READMEs under
[`submission/src/`](submission/src/); the rationale behind the major
choices is in [`DESIGN.md`](DESIGN.md). Sources live under
`submission/src/`, tests under
[`submission/test/`](submission/test/) (per-module Catch2 unit tests)
and [`test/`](test/) (workspace-level black-box scenarios). Library
layout follows [ADR 0001](docs/adr/0001-cpp-project-layout.md).

## Documentation hierarchy

```
README.md                       Top-level orientation: architecture diagram, threading
                                model, component map, "where to read next",
                                quick start.
submission/README.md            Source-tree map; links to per-library READMEs.
submission/src/<lib>/README.md  Per-library responsibility + reading order.
docs/cpp-design-principles.md   How this codebase approaches C++. Other docs link to it.
docs/showcase.md                Where each principle lands as concrete code.
docs/engine-specs.md            Procedural + observable spec of the matching engine.
docs/runtime-startup.md         Chronological application wiring path.
docs/event-loop.md              Event loop primitive each thread runs.
docs/performance.md             Current benchmark snapshot and methodology.
docs/adr/                       Architecture decisions, numbered and dated.
DESIGN.md                       Decision narrative: constraints, ADR summary,
                                localized decision pointers, improvements.
DEPENDENCIES.md                 Dependency inventory and `kraken::` alias rule.
DEVELOPING.md                   Build presets, setup, and repository conventions.
EXERCISE.md                     The problem statement (do not modify).
AGENTS.md                       You are here. Agent operational rules + short anchors
                                that point into the above.
work-in-progress/               Working notes that have not been promoted to docs/ yet.
.claude/skills/                 Procedural workflows for specific tasks
                                (for example, tracing call graphs).
```

## Fast checklist

- At session start, read the required files below.
- Before implementing, search for existing code that solves the same problem.
- Before code edits, read [`DEVELOPING.md#conventions`](DEVELOPING.md#conventions),
  especially the namespace-alias rules.
- Before building or testing, use `./build.sh` (presets: `debug`, `release`, `asan`, `tsan`, `clang`).
- Before external library or API lookup, use the searching-docs skill.
- Let the README's "Where to read next" table drive task-specific
  context. Runtime work usually needs `docs/runtime-startup.md` and
  `docs/event-loop.md`; dependency work needs `DEPENDENCIES.md`;
  performance work needs `docs/performance.md` and the benchmark READMEs.
- Touch [`docs/cpp-design-principles.md`](docs/cpp-design-principles.md)
  when a principle changes; touch the relevant per-library README when
  the public surface of a library changes.

## Required reading

Before working on the codebase you **MUST** read, in order:

1. [`README.md`](README.md) -- the architecture diagram, the
   threading model, the component map.
2. [`docs/cpp-design-principles.md`](docs/cpp-design-principles.md)
   -- how this codebase approaches C++. The principles below quote
   it; the doc is the source of truth for the rationale.
3. [`DESIGN.md`](DESIGN.md) -- the major design choices, the
   constraints they were made under, and the ADR summary that
   indexes the full reasoning.
4. [`docs/engine-specs.md`](docs/engine-specs.md) -- the procedural and
   observable spec of the matching engine (read before touching
   `matching_engine`).

Read all four before any other action -- including searches, edits,
and test runs. There are no exceptions.

## Submission context: prefer simple over clever

This is an interview submission on a deadline. The goal is to
showcase senior judgement and a production-shaped architecture, not
to hit the last drop of performance. When two approaches both fit
the architecture, prefer the simpler one even if it pays a
measurable performance cost.

Concretely:

- Reach for a standard or vendored helper before writing a custom
  one. `kraken::split_fields` is preferred over a hand-rolled tokenizer
  loop, even though its current implementation allocates through
  `boost::algorithm::split`. `kraken::from_chars` over a hand-rolled parser.
  Range-based for loops over manual iterator dances.
- Do not pre-optimize the hot path. Allocation-free CSV parsing,
  bespoke containers, custom string types, and lock-free tricks are
  only justified when a profiler or a clear architectural requirement
  asks for them. A `std::vector` in the receive loop is fine until
  benchmarks say otherwise.
- Performance idioms that the design docs already commit to
  (`kraken::fixed_string`, `kraken::inplace_function`, pool-backed
  intrusive order nodes, startup reservation, the three-thread layout,
  the domain/runtime split) stay -- they are part of the showcase.
  The constraint applies to *new* code: do not invent more of them.
- If a simpler approach loses a property the design relies on (type
  safety, exhaustiveness, error propagation), keep the property and
  pay the small ceremony cost. The constraint is "simple over fast",
  not "simple over correct".

When in doubt, write the obvious version, leave a one-line comment
if the trade-off is non-obvious, and move on.

## Operating in the repo

### Always explore before implementing

Before writing new code -- especially utilities -- search the
codebase for existing abstractions that solve the same problem. Grep
for the types/concepts involved and read how existing code handles
them. Prefer composing project libraries over reimplementing. This
applies even when asked for "standalone" tools -- push back and
suggest project libraries if they exist.

The general-purpose utilities (`result`, `expected`, `error`,
`fixed_string`, `strong_type`, `inplace_function`, `match`,
`split_fields`, `from_chars`, `concurrent_queue`, `event_loop`) live
under [`submission/src/kraken/kraken/`](submission/src/kraken/kraken/).
Some are thin re-exports of vendored upstream libraries, and others
are in-house vocabulary helpers -- see [`DEPENDENCIES.md`](DEPENDENCIES.md)
for the list and the aliasing rule.

### Building and testing

`./build.sh [preset] [target]`. Presets: `debug` (default),
`release`, `asan`, `tsan`, `clang`. Build artefacts land under
`_build/<preset>/`. Tests run after a successful build.

`./build.sh release kraken_submission` is the closest local
approximation of what the Docker pipeline does for grading.

### Documentation lookup

Always use the searching-docs skill for external library and API
documentation.

### Code navigation

Use clangd/LSP when available: go-to-definition, references, call
hierarchy, hover. Fall back to focused `ag`/`grep` and targeted file
reads when LSP is unavailable or hits blind spots (template- and
callback-heavy code).

### Search tools

`ag` for the codebase (respects `.gitignore`); add `--unrestricted`
to include ignored files. `grep` for single files and piped input.

### Tests, sandboxes, and `kraken::inplace_function`

Many pipeline-stage classes use non-defaulted `kraken::inplace_function`
callbacks for their `on_*` fields. In debug builds, leaving them
unwired fails late -- typically during teardown -- with an
"uninitialized `kraken::inplace_function`" assertion.

For **tests, sandboxes, spikes, and PoCs only**: wire explicit noop
lambdas to callbacks irrelevant to the scenario before the object is
destroyed. Check base classes too. Search for an existing
`wire_noop_callbacks(...)` helper before adding local noops.

For production code, every non-defaulted callback is part of the
wiring contract.

## Conventions at a glance

Short anchors for the conventions most likely to bite. Each entry
points at the full discussion in
[`docs/cpp-design-principles.md`](docs/cpp-design-principles.md).

### Directory layout

Stuttering: `<component>/<component>/` for headers,
`<component>/src/` for implementation. Tests under
`submission/test/<component>/`. The general-purpose `kraken`
library may be depended on by every domain. The two leaf domains
(`order_routing` and `market_data`) do not depend on each other;
`matching_engine` is the deliberate composition point and consumes
`order_routing` requests and emits `market_data` messages directly.
See [ADR 0001](docs/adr/0001-cpp-project-layout.md).

### Strong typing

`kraken::strong_type<T, Tag>` for any primitive that can be confused
with another. One `types.hpp` per domain; domain type aliases live in
the domain's nested `types` namespace, not in the enclosing domain
namespace.

```cpp
namespace order_routing::types {
using user_id = kraken::strong_type<std::uint64_t, struct UserIdTag>;
using user_order_id = kraken::strong_type<std::uint64_t, struct UserOrderIdTag>;
using symbol = kraken::strong_type<kraken::fixed_string<8>, struct SymbolTag>;
} // namespace order_routing::types

void place_order(order_routing::types::user_id,
                 order_routing::types::symbol,
                 ...); // compile error if swapped
```

Within a domain's own namespace, keep the `types::` qualifier. Do
not pull individual types into the enclosing namespace with `using`.

```cpp
namespace order_routing {

auto price = types::price{100}; // Good.

using types::price;             // Bad: hides the ownership boundary.

} // namespace order_routing
```

Use `kraken::strong_type` ergonomically. The project skill bundle
supports implicit const/reference call-through, same-type
comparison, same-type arithmetic that preserves the strong type,
hashing, streaming, and `fmt` formatting. Do not unwrap and rewrap
when the operation can stay in the strong type.

```cpp
if (remaining_quantity > 0 && limit_price >= level_price) { /* ... */ }

if (remaining_quantity.get() > 0 && limit_price.get() >= level_price.get()) {
  /* Bad: redundant unwrapping. */
}

remaining_quantity -= fill_quantity;          // Good.
remaining_quantity = remaining_quantity - fill_quantity; // Also good.
remaining_quantity = types::quantity{remaining_quantity - fill_quantity}; // Bad.
```

```cpp
auto total = types::quantity{0}; // Good accumulator.
for (const auto& order : orders) {
  total += order.remaining_quantity;
}
return total;
```

```cpp
std::uint64_t total = 0; // Bad: unwraps the domain too early.
for (const auto& order : orders) {
  total += order.remaining_quantity.get();
}
return types::quantity{total};
```

This applies to order-book aggregate helpers such as
`total_at_best_bid()` and `total_at_best_ask()`: use a
`types::quantity` accumulator directly when summing quantities.

```cpp
fmt::format("{}", order_id);                  // Good.
fmt::format("{}", order_id.get());            // Bad.
```

Only unwrap with `.get()` at a real boundary that lacks a strong-type
overload, such as an API that requires the exact primitive type. A
common legitimate example is `std::to_string(value.get())`.

See [`cpp-design-principles.md#compile-time-correctness`](docs/cpp-design-principles.md#compile-time-correctness).

### Namespace aliases

Follow [`DEVELOPING.md#namespace-aliases`](DEVELOPING.md#namespace-aliases).
When a `.cpp` or test file repeatedly mixes domain vocabularies, use
the standard aliases:

```cpp
namespace md = market_data;
namespace me = matching_engine;
namespace rt = order_routing;

auto price = rt::types::price{100};
auto trade = md::trade{.trade_price = md::types::price{100}};
```

Do not invent alternate aliases such as `mkt`, `ord`, `rtt`, or
`ort`. Do not add namespace aliases or `using` declarations to
header files; they leak into includers and make name lookup depend
on include order.

### Exhaustiveness

`enum class` switches have no `default`; the compiler enforces
coverage under `-Werror`. Constant switches (the CSV record-type
char, etc.) and if/else chains keep a `default` / final `else`
returning `kraken::make_leaf_error(...)`.

### Const placement

West const everywhere: `const T&`, `const T*`, `const auto&`. Never
`T const&` or `auto const`. The qualifier sits to the left of the
type it applies to.

```cpp
void handle(const rt::new_order& request);            // Good.
for (const auto& instrument : valid_symbols) { ... }  // Good.

void handle(rt::new_order const& request);            // Bad.
for (auto const& instrument : valid_symbols) { ... }  // Bad.
```

Top-level const on a local pointer or value (`order_node* const`,
`const int x`) is unrelated and stays as-is.

### Designated initializers

Always. Trailing comma on multi-line. Brace elision for non-scalar
strong types (`.symbol{"XBT"}` rather than
`.symbol = types::symbol{"XBT"}`); `=` for scalars.

```cpp
auto config = server_config{
  .listen_address = "0.0.0.0",
  .listen_port = 1234,
  .reuse_address = true,
  .recv_buffer_bytes = 1 << 20,
};
```

### Error handling

`kraken::result<T>` (alias for `boost::leaf::result<T>`) is the
**interior** vocabulary; it never escapes a public function. At a
domain or module boundary, consume the result with
`boost::leaf::try_handle_*` and return
`kraken::expected`/`std::optional`/`void`. `kraken::expected` is the
C++20 backport used where a boundary wants value-or-error vocabulary
without depending on C++23 `std::expected`. Errors live in the
domain that produces them (`<domain>/error_code.hpp` +
`<domain>/errors.hpp`).

The one exception is a utility library whose contract *is* "this
algorithm can fail" -- e.g. helpers in `kraken/algorithm.hpp` --
where the algorithm itself is the boundary and `kraken::result`
is a legitimate return type.

Unrecoverable runtime failures (allocation failure on a cross-thread
post, for instance) are not error-handling territory: they assert
and abort. `kraken::event_loop::post` is `noexcept` and returns
`void`.

```cpp
kraken::result<void> process() {
  BOOST_LEAF_ASSIGN(const auto& order, orders_.find(id));
  BOOST_LEAF_CHECK(validate(order));
  return {};
}

// Domain-specific structured error
return kraken::make_leaf_error(
  order_routing::errors::invalid_field{.field_name = "side", .field_value = token});

// Generic error -- use the convenience overload, do not build generic_error by hand
return kraken::make_leaf_error(kraken::error_code::configuration_error,
                               "udp_port must not be zero");
```

See [`cpp-design-principles.md#error-handling`](docs/cpp-design-principles.md#error-handling).

### Result pipelines over `if`-cascades

When a function strings together **three or more** sequential
fallible steps -- container lookups, validation checks, parsing,
resource acquisition -- prefer extracting each step into a
`kraken::result`-returning helper and composing them with
`BOOST_LEAF_ASSIGN` / `BOOST_LEAF_CHECK` instead of an inline
cascade of `if (!x) { log; return; }` guards.

Before -- the cascade interleaves business logic with error
plumbing, and the reader has to skip every other block to follow
the success path:

```cpp
void engine::handle(const rt::cancel_order& request) {
  const auto order_it = resting_index_.find(
      order_key{request.user_id, request.user_order_id});
  if (order_it == resting_index_.end()) { log_warn("cancel miss"); return; }

  const auto book_it = books_.find(order_it->second->resting.instrument);
  if (book_it == books_.end()) { log_error("book gone"); return; }

  // ... cancel logic over order_it, book_it
}
```

After -- failure plumbing lives in the helpers, the body reads as
a sequence of named steps, and the leaf handler at the boundary is
the one place that maps errors back to log lines:

```cpp
kraken::result<void> engine::handle_cancel_impl(const rt::cancel_order& request) {
  BOOST_LEAF_ASSIGN(auto* node, find_resting(request));
  BOOST_LEAF_ASSIGN(auto& book, find_book(node->resting.instrument));
  // ... cancel logic over node, book
  return {};
}

void engine::handle(const rt::cancel_order& request) {
  boost::leaf::try_handle_all(
    [&] { return handle_cancel_impl(request); },
    [&](const errors::unknown_order&) { log_warn("cancel miss"); },
    [&](const errors::missing_book&)  { log_error("book gone"); });
}
```

The helpers (`find_resting`, `find_book`) are container-aware
lookups shaped like the `mil::find_*` family -- they hide the
`it == end()` check and surface the failure as a leaf error. The
canonical reference set lives at
`/home/msi/abacus_workspace/abacus/src/mil/mil/algorithm.hpp`
(`find_iterator`, `find_element`, `find_if`, `require_contains_if`,
`try_emplace_unique`, `at`). When a candidate site benefits, write
the helper inline as a private member or a free function near the
container -- do not add a generic library unless the same shape
appears in three or more places.

**Convert when all of these hold:**

- Three or more sequential fallible steps in one function body.
- Each step's failure is reported the same way (log + drop, or log
  + reject), so one leaf handler at the boundary covers them all.
- The function is internal to a domain. A `kraken::result` return
  at a public domain boundary still converts to
  `kraken::expected` / `std::optional` / `void`.
- The structured errors already exist in `<domain>/errors.hpp`, or
  at most one new variant covers the new sites.

**Skip when any of these hold:**

- One or two fallible steps. The linear `if`-cascade is the
  obvious version; the pipeline does not pay for itself.
- The function already reads top-to-bottom with named intermediates
  and no nesting. Restructuring for the pattern's sake is clever,
  not simple.
- Converting would require two or more new structured error types
  per site. The scaffolding swamps the gain.
- Hot-path code where the `if` is a prediction
  (`if (likely_path) { ... }`) rather than an error guard.

The same pattern applies symmetrically to extraction inside an
already-`kraken::result`-returning function: if a function's body
contains three or more `BOOST_LEAF_ASSIGN` lines that share a
theme (all "resolve inputs", all "build outputs"), group them into
a named helper so the outer function reads as the top-level
sequence.

The working note `work-in-progress/if-cascades.md` is a self-
contained sweep prompt for finding candidates across the codebase.

### Declarative style

Decompose, stage data upfront, name predicates, use ranges and
`kraken::match` for variants. Prefer existing `kraken::` helpers
(`split_fields`, `trim`, `from_chars`, `KRAKEN_PLUCK`) and standard
library member helpers such as `.contains(...)` over open-coded
parsing, variant dispatch, and iterator checks.

### Hot-path allocation

`kraken::fixed_string<N>` over `std::string` for bounded hot-path text,
`kraken::inplace_function` for stored callbacks, pool-backed intrusive
nodes for resting orders, and `.reserve(...)` at startup for indexes
sized from config. `boost::container::static_vector` is currently only
a recorded follow-up for the CSV splitter if profiling justifies it. See
[`cpp-design-principles.md#performance-discipline`](docs/cpp-design-principles.md#performance-discipline).

### Runtime layer split

The domain libraries (`order_routing`, `matching_engine`, `market_data`)
are pure synchronous code over value types. Threading, the UDP
source, the stdout sink, and the three event loops live in
`kraken_submission` -- the wiring shell near `main`. Unit tests
exercise the domain libraries without spinning threads. See
[`cpp-design-principles.md#functional-core-imperative-shell`](docs/cpp-design-principles.md#functional-core-imperative-shell)
and [`DESIGN.md`](DESIGN.md).

### Vendored utilities

Production code depends on `kraken::` aliases, not on the upstream
headers directly. Swapping a vendored library out is a single header
change in `submission/src/kraken/kraken/` rather than a sweep. The
vendored list and the aliasing rule live in
[`DEPENDENCIES.md`](DEPENDENCIES.md); the mechanism that brings them
into the build is recorded in
[ADR 0002](docs/adr/0002-copy-vendor-third-party-utilities.md).

## Comments

`/* ... */` for class- and struct-level documentation; `//` for
inline notes inside function bodies. Comments describe the present
code and the **why** behind non-obvious choices; they must never
describe what the code used to do, what it no longer does, or what
the caller is expected to handle (the negative-documentation rules
in your global `CLAUDE.md` are the source of truth).

### Block-level flow comments

Lean toward commenting blocks with brief plain-language summaries so
a reviewer can follow a function's flow without parsing C++. A
"block" is anything from a multi-line group of related statements
up to a whole `if` / `for` / lambda body. Read consecutively, the
flow comments inside a function should read as a procedural
walkthrough.

The model is `handle_new_order_impl` in
[`submission/src/matching_engine/src/v3/engine.cpp`](submission/src/matching_engine/src/v3/engine.cpp).

Skip the comment when the statement is a **single line, short, and
free of syntax noise** -- a clean `kraken::match` dispatcher, a
switch on a self-describing enum, or an `if (cond) return x;` guard
does not need one. Conversely, even a short block deserves a comment
when it has dense punctuation, designated initializers across domain
boundaries, or chained method calls that obscure intent.

Good -- one flow line over a 3-statement block with syntax noise:

```cpp
// drop the identity entry, unlink the FIFO head, return the slot to the pool.
resting_index_.erase(types::order_key{.user = node.data.user, .order_id = node.data.order_id});
level.orders.pop_front();
release_node(&node);
```

Good -- block summary above a multi-step conditional:

```cpp
// rest the residual: pool-allocate a node, link it into the book,
// register its identity for future cancels and duplicate checks.
order_node* node = allocate_node(order);
book.place(node);
resting_index_.emplace(incoming_key, node);
```

Good -- a why-comment sits alongside the flow comment when a step
carries a non-obvious ordering or invariant:

```cpp
// ack precedes any trade so receivers see the order id before fills on it.
on_event(md::order_ack{
  .user = md::types::user_id{request.user},
  .order_id = md::types::user_order_id{request.order_id},
});
```

Bad -- narrating an obvious dispatcher one-liner:

```cpp
// dispatch to the typed handle() overload.
kraken::match(cmd, [this](const auto& cmd) { handle(cmd); });
```

Better -- delete the comment; the dispatcher pattern is self-evident:

```cpp
kraken::match(cmd, [this](const auto& cmd) { handle(cmd); });
```

Bad -- restating what a single clean line plainly does:

```cpp
// increment the counter.
++counter;
```

Better -- delete the comment.

Bad -- delta framing for absent behavior (see the negative-doc rule):

```cpp
// dedup moved to the upstream stage
for (const auto& order : orders) {
  process(order);
}
```

Better -- if the precondition is genuinely load-bearing, phrase it
as a positive statement about the present code; otherwise delete it:

```cpp
// precondition: orders has been deduped upstream; we do not re-check here.
for (const auto& order : orders) {
  process(order);
}
```
