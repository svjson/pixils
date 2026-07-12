This is a frozen benchmark fixture for `ExampleTilemapEditorBenchmark`.

Source: `examples/tilemap-editor` from commit `808e9fd`, where the benchmark cases
were introduced.

Compatibility edits:
- Source files were renamed from `.lisple` to `.roo`.
- `lisple.io` requires were updated to `roo.io`.
- The bundled spritesheet path points at this copied fixture asset.

Do not update this from the live `examples/tilemap-editor` application. Add a new
fixture snapshot instead if a benchmark needs to track a different application shape.
