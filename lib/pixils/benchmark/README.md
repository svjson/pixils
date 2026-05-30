# Pixils Benchmarks

Build and run the full benchmark suite:

```sh
make benchmark
```

Pass benchmark runner arguments with `BENCHMARK_ARGS`:

```sh
make benchmark BENCHMARK_ARGS="--benchmark-category appfixture"
```

Build benchmarks explicitly:

```sh
cmake --build build --target benchmarkpixils
```

Run all benchmarks and write CSV files:

```sh
build/lib/pixils/benchmark/benchmarkpixils
```

Benchmark CSV output is intended to be kept in the repository. The default
output root is `benchmarks/pixils`, organized by a manual goalpost directory.
Repeated runs append rows to the same per-benchmark CSV files so git history
captures changes over time.

Useful options:

- `--benchmark-dir <dir>` writes under a custom output root. Default:
  `benchmarks/pixils`.
- `--benchmark-goalpost <name>` selects the manual goalpost directory. Default:
  `003_broader_benchmark_metrics`.
- `--benchmark-run <name>` selects the run label written to CSV. Default:
  `history`.
- `--benchmark-category <name>` runs only benchmarks from that category.
- `--no-csv` prints results without writing CSV files.
- `--allow-dirty-worktree` allows CSV writes when tracked library files are
  dirty.
- GoogleTest filters still work, for example:
  `--gtest_filter=LayoutBenchmark.layout_120_mixed_children`.

Run only the composed appfixture benchmarks:

```sh
build/lib/pixils/benchmark/benchmarkpixils --benchmark-category appfixture
```

CSV files are written per run, category, and case:

```text
benchmarks/pixils/<goalpost>/<category>/<benchmark>.csv
```

Each category owns its metric column set. Shared timing columns come first:

```text
timestamp,goalpost,run,category,benchmark,iterations,total_time_ms,mean_time_ms,...
```

Existing CSV files are append-only. A benchmark run adds one row per benchmark
file and never rewrites old rows for schema migration. If a category's metric
columns change, start a new goalpost directory or migrate the file explicitly.

CSV writes require tracked library files under `lib/pixils` to be clean,
excluding `lib/pixils/benchmark`. Untracked files, docs, benchmark harness
changes, and benchmark CSVs do not block a run. This keeps accidental
library-code benchmark runs out of history while still allowing goalpost bumps
and repeated runs of the same committed revision to append more rows for
time-noise normalization.

The goalpost is deliberately manual. Change the default in
`benchmark/support/benchmark.h` when starting a new benchmark milestone, similar
to liblisple's `CHANGE_ME` benchmark folder.

Stable counters live in `include/pixils/benchmark/counters.h` as `MetricId`.
Append new metrics instead of renaming existing IDs or changing existing CSV
column names. Production and test builds compile counter call sites to no-ops;
only `pixils_benchmark_objects` is compiled with
`PIXILS_ENABLE_BENCHMARK_COUNTERS`.

The broader UI/appfixture categories include style/theme cache counters, dirty
layout placeholders, text measurement/render counters, render operation counts,
and nanosecond phase timers for runtime, layout, render, style, hooks, and text.

The `appfixture` category runs real composed test applications through
`ComposableAppSessionFixture` and includes runtime, layout, render, event, and
`UI::Style` allocation counters.
