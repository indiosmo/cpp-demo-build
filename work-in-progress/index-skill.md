# INDEX.md skill -- design notes

A skill that walks a C++ / CMake codebase and produces a single
`INDEX.md` at the project root: a navigation map for AI agents (and
humans) that points to *where things live*, with enough context per
file that a reader can pick the right one without opening five
wrong ones first.

Scope:

- C++ only. CMake-based projects only. Any non-C++ trees (Python
  tooling, scripts, generated artefacts) are ignored.
- One language per run. Multi-language repos are out of scope.

Deliberately narrower than the `PROJECT.md` / `AGENTS.md` pattern:

- No conventions, no coding rules, no architecture narrative, no
  agent directives. Those belong in `AGENTS.md` / per-module
  READMEs.
- No promises about being a system prompt. This is a directory.
- One source of truth per concern: this file is the index,
  `AGENTS.md` is the rulebook, READMEs are the per-module guides.

The skill should produce `INDEX.md` plus a companion process report
and stop. It should not invent conventions, restate architecture,
or duplicate content that already exists elsewhere.

### Maintenance

`INDEX.md` is living documentation. The agent updates it whenever a
change in the working session would make the index drift:

- A header is added, removed, renamed, or moved.
- A header's intent or central role changes substantially (a thin
  wrapper grows into the main API; a module's entry point shifts).
- A module is added, removed, renamed, or restructured.
- The directory tree gains or loses a top-level subtree.

Trivial changes (renaming a parameter, fixing a typo in a comment,
adding a private helper that doesn't change the file's role) do not
trigger an update. When in doubt, update -- a stale index is worse
than a touched-too-often one.

---

## What `INDEX.md` contains

Four sections, in this order.

### 1. System overview

Two to three sentences. What the project is, what it does, what
shape it has (library, service, CLI, multi-binary workspace). No
history, no rationale -- just orientation.

### 2. Tech stack

Inventory only:

- C++ standard / version (e.g. C++23).
- Build system (CMake, with presets if any).
- External libraries actually used in the source. Name + origin URL
  per entry. The URL resolves ambiguity for libraries that share a
  name (e.g. there are multiple `expected` implementations) and
  gives the agent a documentation anchor.
- Test framework.

Bullets, no prose. Example:

- C++23
- CMake (presets: debug, release, asan, tsan, clang)
- Boost.LEAF -- https://github.com/boostorg/leaf
- fmt -- https://github.com/fmtlib/fmt
- Catch2 (tests) -- https://github.com/catchorg/Catch2

### 3. Directory structure

`tree -d`-style output, but annotated. One line per directory with
a short purpose tag. Depth up to 5 levels so nested submodules show
up (the `kraken/kraken/` stutter, then a `network/` submodule under
it, etc.).

Per-directory depth exceptions:

- `test/` -- list once, no recursion. The test tree mirrors the
  source tree by convention; spelling it out wastes lines.
- `libs/` (or `third_party/`, `vendor/`) -- list once, no
  recursion. The index focuses on this project's surface; vendored
  trees are reference, not surface.
- Build / generated directories (`_build/`, `build/`,
  `cmake-build-*/`) -- skipped entirely. Mention once in a
  "skipped" footnote if their presence is worth knowing.

```
submission/
|-- src/
|   |-- kraken/
|   |   `-- kraken/            general-purpose vocabulary headers
|   |       `-- network/       UDP / socket helpers
|   |-- order_routing/         UDP-source-facing domain
|   |-- market_data/           stdout-sink-facing domain
|   |-- matching_engine/       composition domain
|   `-- kraken_submission/     runtime shell + main
`-- test/                      per-module Catch2 unit tests (not expanded)
```

### 4. Module sections

One section per logical unit. For this repo that's `kraken`,
`order_routing`, `market_data`, `matching_engine`, and
`kraken_submission`.

Each module section has:

**Header** -- the module name and a 2-4 sentence summary of *what
the module does*: its responsibility, the abstractions it owns, the
problem it solves inside the system. Dependencies and consumers are
not part of the summary -- they're input the agent uses while
writing it, not output the reader needs to see. A dependency graph
in every module summary would dominate the file, and common
libraries like `kraken` would carry a consumer list the length of
the project.

The summary is informed by reading the module's README if one
exists, plus a full read of the module's headers **and**
implementation files. The header list is what ends up in
`INDEX.md`; the implementation files are read for context so the
descriptions are grounded in what the code actually does. The
summary should not paraphrase the README -- it should compress it.

When the README and the code disagree, the agent resolves intent
from the surrounding code and writes the description that reflects
present reality. Every such conflict is recorded in the companion
report so the user can confirm which side was actually right.

**File list** -- one line per header. Both public and internal
headers are listed; the index is for the agent's navigation, and
working on the module itself requires knowing where internal helpers
live too.

Each line is tagged with visibility:

- `(P)` -- public, included from outside the module.
- `(I)` -- internal, only included within the module.

When visibility is ambiguous (no clear convention in this codebase,
header sits in a public-looking directory but only internal
includers, etc.), list as `(P)`. The cost of an over-public tag is
a reader trying to use a stable-looking header; the cost of an
over-internal tag is a reader skipping the right file. Bias toward
the cheaper mistake.

Implementation files (`.cpp`) are **not** listed in `INDEX.md`. The
agent navigates to them from the header. But the indexing agent
must read them during discovery -- a header's description is
shallow if it comes only from the declarations.

**Path format** -- module-root-relative, with the base path stated
once at the top of the section. See the path-format trade-off below
for the rationale.

```
### matching_engine

Base path: `submission/src/matching_engine/matching_engine/`

Order-book matching domain. Consumes `order_routing` requests,
emits `market_data` events. Depends on `kraken` for vocabulary
helpers. Composed in the `kraken_submission` runtime shell.

Headers:

- (P) engine.hpp           -- engine entry point; dispatches requests to handlers
- (P) types.hpp            -- domain vocabulary (book id, level, side)
- (I) v3/order_node.hpp    -- pool-backed intrusive node for resting orders
- (I) v3/book.hpp          -- per-instrument bid/ask ladders
...
```

Format per file line:

```
(P|I) <relative path>  -- <description>
```

Descriptions describe *what the file does*. Strive for terseness;
target around 100 characters. Going longer is fine when the file
genuinely needs it (a header that anchors a small subsystem, a
vocabulary file whose entries don't share a single theme). The hard
upper limit is 300 characters -- past that the line becomes its
own paragraph and the listing stops being scannable. Split into a
follow-up note in the report rather than letting one line dominate
the section.

---

## Path format: full vs module-relative

Two options for how file paths appear inside a module's file list.

**Option A -- full path from repo root on every line.**

```
- (P) submission/src/matching_engine/matching_engine/engine.hpp -- ...
```

- Pro: every line is a copy-pasteable `Read` argument with no
  concatenation. Reliability is independent of section context.
- Pro: same-named files across modules (`types.hpp`, `errors.hpp`)
  are unambiguous at the line level.
- Con: high token cost. With ~80 char paths repeated across 100+
  headers, the repetition dominates the file.
- Con: visually noisy. The interesting part of each line (the file
  name + description) sits at the end of a long prefix.

**Option B -- base path once per section, then module-relative
paths.**

```
Base path: `submission/src/matching_engine/matching_engine/`

- (P) engine.hpp -- ...
- (I) v3/book.hpp -- ...
```

- Pro: dense, scannable, low-token.
- Pro: same-named files are still unambiguous because each file
  appears under its module's section -- the section header carries
  the disambiguator.
- Con: the agent has to concatenate base path + relative path to
  call `Read`. Trivial in principle, one extra step in practice.
- Con: if the section header is dropped (copy-paste of a fragment
  into another context), the relative paths lose their anchor.

**Recommendation: Option B.** The token savings on a realistic
codebase are large enough to justify the concat step, and the
section header keeps the anchor close. The "fragment copy-paste"
failure mode is rare enough to accept, and it can be mitigated by
keeping the base path repeated on the first line of each module's
file list (so a copied section always brings its anchor).

If we hit the failure mode in practice, switching to Option A is a
mechanical rewrite of the file.

---

## How file descriptions are written

This is where the skill earns its keep. A naive description reads
the file in isolation and produces something like "header declaring
class Foo." That's worse than `tree`.

The output is a description of *what the file does*. The agent gets
there by drawing on four contexts during exploration -- but these
are inputs to understanding, not content to enumerate in the line
itself.

1. **The file itself, plus its implementation.** What it declares:
   types, free functions, constants, the central abstraction. The
   indexing agent reads the matching `.cpp` (or whatever the
   implementation file is) -- the public surface plus the
   implementation together produce a grounded description.
2. **Its neighbors.** What other files sit next to it in the same
   directory/module, so the description can position the file
   against its peers when that disambiguates ("bid-side ladder" vs
   "ask-side ladder").
3. **The module's role.** The same `types.hpp` means different
   things in `order_routing` (incoming-request vocabulary) and
   `market_data` (outgoing-event vocabulary). The agent uses the
   module context to pick the right framing.
4. **Consumers.** Where the file's exports are used elsewhere in
   the repo. Useful as a sanity check on the description (a header
   included only by tests is a test helper; one that crosses domain
   boundaries is a contract). The consumer list itself does not
   appear in the line.

The line says what the file does. The four contexts are how the
agent figures out what to say.

Bad: `engine.hpp -- declares the engine class`
Bad: `engine.hpp -- declares engine; used by kraken_submission, included by tests/engine_test.cpp`
Better: `engine.hpp -- order-book engine entry point; matches incoming requests and emits trade events`

---

## Companion report

Alongside `INDEX.md`, the skill writes `INDEX-report.md` (sibling
file, project root). The report is for the user, not the agent --
it captures everything the skill ran into during indexing that
deserves human attention.

Contents:

- **Ambiguities** -- headers where visibility could not be
  determined cleanly, where the role inside the module was unclear,
  where two plausible descriptions both fit.
- **Inconsistencies** -- README claims that don't match the code
  (every case where the agent resolved a conflict by trusting one
  side over the other, with both sides quoted so the user can
  confirm), modules with no README where peers have one, headers
  in public-looking locations with no external includers (or vice
  versa), dead headers (no includers anywhere), duplicate
  declarations across modules.
- **Errors** -- files the skill could not parse, files with
  malformed includes, headers that wouldn't compile in isolation
  (if the skill bothers to check).
- **Decisions** -- non-obvious calls the skill made (which module a
  borderline header was assigned to, which of two candidate
  descriptions was chosen). Briefly, so the user can audit.
- **Skipped** -- anything intentionally not indexed (non-C++
  trees, vendored directories, generated code) and the reason.

Format: plain markdown, grouped by category, with file paths as
anchors so the user can navigate from the report into the source.
Empty sections are kept with a "none" note so the user knows the
skill actually checked rather than skipped.

---

## Skill structure

### Inputs

- No required arguments. Run from the project root.
- Optional: a module path (or list) to limit scope. Useful for
  re-running on one module without rewriting the whole file.

### Working folder

At the start of every run, the skill creates a working folder under
`/tmp` and uses it as the spine of phase-to-phase communication.
Subagent return messages compress what the agent learned; the
on-disk files are the source of truth.

Path: `/tmp/index-skill-<YYYYMMDD-HHMMSS>-<short-id>/`

Layout:

```
/tmp/index-skill-<run-id>/
|-- manifest.md             skill name, start time, args, working folder
|-- run.log                 timestamped append-only log of phase events
|-- preflight.md            preflight checks + their results
|-- consumer-map.json       cross-module includer edges (built once up front)
|-- discovery/
|   |-- <module>.md         per-module discovery output, full
|   `-- <module>.tests.md   nested-subagent test-intent summaries
|-- drafting/
|   `-- <module>.md         per-module draft of the INDEX section
`-- report-fragments/
    `-- <module>.md         per-module findings for the companion report
```

Rules:

- **Every phase writes its full output to the folder.** Nothing
  important lives only in a subagent return message.
- **Every subagent prompt names the working folder path and the
  exact file the subagent must write to.** Subagents do not pick
  their own paths; the main thread (or the dispatching subagent)
  decides the layout so assembly stays a predictable glob.
- **File naming is flat and predictable** -- `<module>.md`,
  `<module>.tests.md`, etc. Nested subagents write under the same
  conventions so their parent can find their output without
  threading paths back through messages.
- **The folder is not cleaned on completion.** It survives the run
  for debugging and inspection; `/tmp` self-cleans on a system
  schedule.
- **Final user-facing artifacts go to the project root, not the
  working folder.** `/tmp` holds working material only; `INDEX.md`
  and `INDEX-report.md` live in the repo.
- **`run.log` is append-only.** Each phase logs its start and end
  with a timestamp so a post-mortem can see where the run spent
  time and whether anything was skipped.

When the skill produces a wrong description, the working folder is
the audit trail: open `discovery/<module>.md`, see exactly what the
subagent had in front of it before it wrote the line.

### Preflight

Before any indexing work, verify the environment:

- A `CMakeLists.txt` exists at the project root.
- `clangd` is on `PATH`.
- A `compile_commands.json` exists where the project's build
  presets place it (typically `_build/<preset>/` or a top-level
  symlink).
- The `compile_commands.json` was generated within the last 60
  seconds.

If any check fails, the skill stops and tells the user exactly what
to do (run `./build.sh <preset>` to regenerate, install clangd, fix
the missing root marker). It does not fall back to grep-only mode
-- the structural information clangd provides is load-bearing for
accurate descriptions and visibility tags.

### Detection phase

- Modules are subdirectories of `src/` (or the layout the project's
  ADRs / `AGENTS.md` specify). Each module's headers live under its
  own `<module>/<module>/` directory by convention; the skill
  treats whatever directory matches that pattern as the header
  root for the module.
- The skill also locates the test root (`test/`) and any vendored
  / library trees (`libs/`, `third_party/`, `vendor/`) so it can
  list them once in the directory tree without recursion.

Headers (`.hpp`, `.h`) are the files of interest. Implementation
files (`.cpp`, `.cc`, `.cxx`) are read for context during
discovery but never listed in `INDEX.md`.

### Discovery phase

This phase is parallelizable and **should** be parallelized in any
project large enough that the full source set doesn't fit a single
context window. Use the `dispatching-parallel-agents` skill: one
subagent per module, each doing its own discovery + drafting end
to end. The main thread orchestrates, supplies the cross-module
consumer map (built once up front), and assembles the final
document.

The main thread runs one pre-pass before dispatch: a sweep of
`#include "..."` edges across the repo, producing a consumer map
keyed by header. Each subagent receives the slice of the map that
touches its module. The map is small (edges only) so it fits even
when the source itself does not.

For each module the subagent:

1. Reads context READMEs in widening rings -- any parent README
   (project root, `src/README.md` if it exists), the module's own
   README, every README nested under the module. The widening-rings
   read places the module inside the system before the agent drills
   into its files.
2. Gathers test-intent context. Tests show the module's public API
   in representative use: which entry points matter, which
   combinations are exercised, which edge cases the authors thought
   worth pinning. The discovery subagent **does not** read raw test
   files itself -- a module's test directory can run to thousands
   of lines and would swamp its context window. Instead it
   dispatches a nested subagent whose only job is to produce a
   short intent-summary of the module's tests. The nested subagent
   writes its summary to
   `discovery/<module>.tests.md` in the working folder; the
   discovery subagent reads that file and uses it as input for the
   per-header descriptions.
3. Enumerates headers in the module (git-tracked only).
4. For each header, collects:
   - The file's top-of-file comment.
   - The declared symbols (types, free functions, constants) via
     clangd / LSP.
   - The corresponding implementation file(s), read in full for
     grounding -- descriptions must reflect what the code actually
     does, not what the header declares.
   - The cross-module includer list, from the consumer-map slice
     the main thread supplied.
   - A visibility tag (`P` / `I`) inferred from where the includers
     live: external includers -> `(P)`; module-only includers ->
     `(I)`; ambiguous -> `(P)` per the bias rule above.

### Drafting phase

Each subagent produces its module's section in the exact format
specified under "Module sections" above. Writing follows the
project's documentation conventions: the subagent consults the
`/documentation` skill for structure and section choices, and the
`/writing-clearly-and-concisely` skill for prose-level conventions
(active voice, no hedging, no filler). The two skills together
cover the "how to write each line" question so this design note
does not have to.

When all subagents complete, the main thread assembles:

1. System overview, tech stack, directory tree (sections 1-3).
2. Module sections in a stable, readable order -- shared / utility
   libraries first, then leaf domains, then composition domains,
   then runtime / application shells. This matches the natural
   dependency order and the order a new reader would want.
3. Writes `INDEX.md` to the project root.

The main thread also assembles `INDEX-report.md` from the notes
each subagent surfaced during its run.

### Verification phase

Before declaring done:

- Every file path in `INDEX.md` resolves to a real file (cheap to
  check; `test -f` each one).
- Every module listed in section 4 corresponds to a real directory.
- The directory tree in section 3 matches the actual top-level
  layout (a `tree -d -L 2` diff).
- No section is empty.

If verification fails, the skill stops and reports -- it does not
hand-wave or skip the failure.

### Idempotency

Re-running the skill should be safe. If `INDEX.md` already exists,
the skill regenerates it from scratch rather than trying to merge.
The "optional: only this module" mode is the supported way to do
partial updates -- it rewrites just that module's section in place
and leaves the rest alone.

### What the skill does *not* do

- It does not write `AGENTS.md`, `DESIGN.md`, ADRs, or READMEs.
- It does not invent conventions.
- It does not summarize implementation files.
- It does not document tests (beyond mentioning the test directory
  in the tree).
- It does not call out "interesting" code, suggest refactors, or
  rate quality.

The index is a map. The map is not the territory; the map is not
the rulebook; the map is not the design doc. Keep it a map.

