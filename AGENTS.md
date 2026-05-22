# AGENTS.md

Agent operating rules for this repository.

## Project Direction

This repository is being reframed as `matching-engine-lab`: a C++26 portfolio
demonstrator for a multi-threaded UDP matching engine. Treat every change as
part of that direction unless the user says otherwise.

Current implementation anchors still contain legacy names and paths. Prefer the
target vocabulary from [`docs/lab-guidelines/`](docs/lab-guidelines/) when
planning new work, and preserve buildability while the rename proceeds.

## Core Context

Read these first when starting substantive work:

- [`README.md`](README.md) -- current project orientation.
- [`DESIGN.md`](DESIGN.md) -- design narrative and trade-offs.
- [`docs/engine-specs.md`](docs/engine-specs.md) -- matching behavior.
- [`docs/lab-guidelines/README.md`](docs/lab-guidelines/README.md) -- local
  mapping for C++ helpers, layout, tests, and debugging.

For task-specific context, walk down from the nearest README, ADR, or guideline
file. Do not load sibling topic files speculatively.

## Navigating Further

Walk down the tree on demand from these entry points:

- root [`INDEX.md`](INDEX.md) -- current codebase map.
- [`submission/src/<module>/README.md`](submission/src/) -- current per-module
  orientation during the reframe.
- future `src/<module>/README.md` and `src/<module>/INDEX.md` -- target
  per-module orientation and navigation after the root layout lands.
- [`docs/lab-guidelines/design.md`](docs/lab-guidelines/design.md),
  [`testing.md`](docs/lab-guidelines/testing.md), and
  [`debugging.md`](docs/lab-guidelines/debugging.md) -- local mappings on top
  of the shared C++ guides.
- [`docs/cpp-guidelines/`](docs/cpp-guidelines/) -- shared submodule. Read each
  tree's README before drilling into a topic; do not edit files there.
- [`docs/adr/`](docs/adr/) -- architecture decision records.
- [`work-in-progress/portfolio-reframe-overview.md`](work-in-progress/portfolio-reframe-overview.md)
  -- temporary north star for the portfolio conversion.

Read the README or index nearest to the code you are touching, then drill down
to the specific topic.

## Progressive Disclosure

Most detailed C++ guidance lives outside this file.

- Shared C++ rules:
  [`docs/cpp-guidelines/agent/cpp-agent-context.md`](docs/cpp-guidelines/agent/cpp-agent-context.md).
- Shared good/bad examples:
  [`docs/cpp-guidelines/agent/cpp-agent-examples.md`](docs/cpp-guidelines/agent/cpp-agent-examples.md).
- Lab mapping:
  [`docs/lab-guidelines/`](docs/lab-guidelines/).
- Lab good/bad examples:
  [`docs/lab-guidelines/agent-examples.md`](docs/lab-guidelines/agent-examples.md).

Slice example files by heading. Use `rg -n '^## ' <file>` to find the relevant
section, then read only that section. Load a whole example file only when one
edit spans many topics.

## Operating Rules

- For comments and in-code prose, follow the active formatter. Do not manually
  force narrow wrapping when `.clang-format` allows wider comments; wrap earlier
  only when it improves readability for the specific comment.
- Explore before implementing. Search for existing helpers, factories,
  callbacks, tests, and docs before adding new ones.
- Use `rg` for repository search. Use clangd/LSP for go-to-definition,
  references, call hierarchy, and include resolution when available.
- For any `#include`, go-to-definition on the include line is usually faster
  than grepping for the file by name.
- Vendor or third-party headers should resolve through clangd first. If they
  do not, check the current vendored tree under `submission/vendor/` and the
  target tree under `vendor/` after the layout reframe.
- Do not run wide-net searches such as `find ~/`, `find /`, or `rg /` to locate
  dependencies. If clangd, focused repo search, and known vendor locations do
  not find it, report what you tried and ask for the path.
- Prefer project vocabulary over generic names. In matching-engine code, use
  terms such as `bid`, `ask`, `resting_price`, `top_of_book`, `aggressor`,
  `maker`, `taker`, and `liquidity` where they fit.
- Keep user-facing durable docs free of work-in-progress paths. `AGENTS.md`
  may point at the portfolio reframe overview while the conversion is active;
  promote stable decisions into `docs/`, ADRs, README files, or lab guidelines.
- Keep examples and fixtures as demonstration assets, not assessment harnesses.
- For external library/API documentation, use the `searching-docs` skill.

## Build And Test

Use `./build.sh [preset] [target]`. Presets include `debug`, `release`,
`asan`, `tsan`, and `clang`.

Run the narrowest meaningful check after edits, then broaden when the change
touches shared behavior, runtime wiring, or public interfaces. Use sanitizer
presets for memory, lifetime, and threading changes.

## Guidelines And Docs

`docs/cpp-guidelines/` is the shared guidelines submodule. Do not edit files
there from this repo; update the source upstream and then bump the submodule.

`docs/lab-guidelines/` is editable in this repo. Update it when local
conventions, helper names, test patterns, or debugging workflows change.

Update the README/INDEX/ADR tree when a change invalidates it:

- source or header files are added, removed, renamed, or moved;
- a module is added, removed, renamed, or moved;
- public behavior, responsibility boundaries, or workflows change.

### Keeping The Index In Sync

The root `INDEX.md` is the navigation map for the current codebase. After the
root-layout reframe, per-module `src/<module>/INDEX.md` files should carry the
same role at module scope.

Update the index whenever a change of yours would invalidate it:

- adding, removing, renaming, or moving a source/header file;
- adding, removing, or renaming a module;
- substantial behavior or responsibility changes that make the current index
  description wrong.

Pure refactors that preserve the existing description do not require an index
update.

Use the `index-cpp-codebase` skill when regenerating or reconciling index
files. Run the index update as part of the same change so navigation does not
lag behind code.

### Keeping READMEs In Sync

The root `README.md` and per-module READMEs are orientation prose: what the
project or module is for, how it relates to the rest of the system, and where
to read next. The index owns file inventories; READMEs own the mental model.

Update the README tree when a change of yours would invalidate it:

- adding, removing, renaming, or moving a module;
- changing a module's public responsibility or composition;
- changing build, run, or verification workflows;
- changing the documentation hierarchy.

When a change touches several modules, review the README for each affected
module. Pure refactors that preserve existing descriptions do not require a
README update.

### Editing Guidelines

`docs/lab-guidelines/` is editable in this repo; update it when local helper
names, test patterns, debugging workflows, or agent-facing examples change.

`docs/cpp-guidelines/` is a shared submodule. Do not edit it from this repo;
changes belong upstream and should come back as a submodule pointer bump.

## Callback Wiring

Many pipeline stages store callbacks in `lab::inplace_function` target types.
During the transition, the current implementation may still use the legacy
namespace, but the rule is the same: tests, sandboxes, spikes, and PoCs must
wire explicit noop callbacks for ignored outputs before teardown.

Production wiring must assign every callback in the composition layer.

## Comments

Comments describe the current code and the reason for non-obvious choices.
Avoid negative documentation: no history, no "now handled elsewhere", and no
lists of responsibilities a function does not have. Use
[`docs/lab-guidelines/agent-examples.md`](docs/lab-guidelines/agent-examples.md)
for examples.
