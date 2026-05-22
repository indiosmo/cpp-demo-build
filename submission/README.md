# submission/

Everything the grading harness builds. Source, unit tests,
microbenchmarks, and copy-vendored third-party headers all sit
underneath this directory; the Docker pipeline copies `submission/`
into the image and builds from there.

The production order book's current latency snapshot is recorded in
[`docs/performance.md`](../docs/performance.md): at a representative
200-order single price level, the v3 `flat_order_book` reports ~12 ns
p99 insert and ~10 ns p99 cancel latency.

The aggregated `src/` / `test/` / `benchmarks/` split, the
stuttering `<lib>/<lib>/` header layout, and the library-plus-thin-executable
convention are recorded in
[ADR 0001](../docs/adr/0001-cpp-project-layout.md). The vendoring
mechanism for the third-party headers is recorded in
[ADR 0002](../docs/adr/0002-copy-vendor-third-party-utilities.md);
the per-utility inventory lives in
[`DEPENDENCIES.md`](../DEPENDENCIES.md).

## Tree

```
submission/
  src/            Libraries and the executable wrapper.
  test/           Catch2 unit tests, mirroring the src/ layout.
  benchmarks/     Google Benchmark microbenchmarks, mirroring the src/ layout.
  vendor/         Copy-vendored third-party headers.
```

Each library directory under `src/` carries its own README with a
one-paragraph responsibility statement and pointers to the ADRs and
design docs that explain its shape.
