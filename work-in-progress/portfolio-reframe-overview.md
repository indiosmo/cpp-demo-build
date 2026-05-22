# Portfolio Reframe Overview

## Purpose

This repository began as a senior C++ take-home exercise: a multi-threaded UDP
order book that reads CSV order commands, maintains price-time books, matches
crossing orders, and publishes market data records. The implementation already
goes beyond the narrow exercise by presenting production-shaped architecture,
documentation, tests, benchmarks, and design rationale.

The next direction is to turn that work into `matching-engine-lab`: a C++26
portfolio demonstrator. The project should read as a deliberate engineering
showcase, not as an interview artifact. Future implementation plans should be
validated against this goal before they are accepted.

The local mapping for those C++ conventions lives in
`docs/lab-guidelines/`. Future implementation plans should consult it when
choosing helper names, result handling shape, test layout, and debugging
tooling.

## Starting Point

The current repo carries two identities at once. The strongest code is the
matching-engine core and the surrounding architecture: typed domain boundaries,
runtime composition, thread ownership, structured errors, benchmarks, and
per-library documentation. That material belongs in the portfolio version.

The surrounding frame still belongs to the original delivery channel. Names,
paths, scripts, generated reports, Docker harnesses, and docs refer to Kraken,
grading, submission bundles, provided scenarios, and the constraints of the
take-home. That frame should be removed or rewritten.

## Target State

`matching-engine-lab` should present a self-contained C++ systems project:

- A modern C++26 codebase that uses new language and library features where
  they make the code clearer, safer, or easier to maintain.
- A clear README that introduces the matching engine, runtime pipeline, and
  learning value of the repo.
- A normal root project layout with source, tests, benchmarks, vendor code,
  docs, and examples in conventional locations.
- Project identity based on `matching-engine-lab`, with `lab` as the internal
  utility namespace.
- A server application that receives UDP order commands and publishes market
  data.
- A client library and command-line app that can send orders to the server.
- Scenario fixtures and integration tests that demonstrate behavior without
  reading like an assessment harness.
- Documentation that explains the design, trade-offs, and extension points as
  portfolio material.

## Planning Standard

Every implementation plan for this reframe should pass these checks:

- It removes interview, employer, grading, and submission framing from durable
  project surfaces.
- It preserves the technical strengths already present: matching semantics,
  typed boundaries, domain/runtime split, benchmark story, and design
  rationale.
- It moves the project toward modern C++26 without using new features for
  novelty alone.
- It follows `docs/lab-guidelines/` when choosing helper names, result
  handling shape, test layout, and debugging tooling.
- It makes the project easier for an outside engineer to clone, build, run,
  inspect, and discuss.
- It favors ordinary project workflows over assessment workflows.
- It treats examples, fixtures, and integration tests as demonstration assets,
  not as hidden-test preparation.
- It keeps changes staged so the repo can be built and tested between phases.

## Non-Goals

This reframe is not a rewrite of the matching engine. Algorithmic changes,
performance experiments, input validation expansion, and new exchange features
belong in separate plans after the project identity and workflow are clean.

The first outcome is clarity: a reader should understand what the project is,
why it exists, how to run it, and what engineering judgment it demonstrates.
