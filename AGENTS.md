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

- Explore before implementing. Search for existing helpers, factories,
  callbacks, tests, and docs before adding new ones.
- Use `rg` for repository search. Use clangd/LSP for go-to-definition,
  references, call hierarchy, and include resolution when available.
- Prefer project vocabulary over generic names. In matching-engine code, use
  terms such as `bid`, `ask`, `resting_price`, `top_of_book`, `aggressor`,
  `maker`, `taker`, and `liquidity` where they fit.
- Keep durable docs free of work-in-progress paths. Promote stable decisions
  into `docs/`, ADRs, README files, or lab guidelines.
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
