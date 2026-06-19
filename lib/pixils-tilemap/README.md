# Pixils Tilemap

`pixils-tilemap` is the runtime tilemap model and renderer used by Pixils
tilemap applications. It keeps the runtime data shape small: maps have
dimensions, tile size, layers, tilesets, optional terrain sets, and optional
terrain-stamp rules. The package does not own editor state, save dialogs, or UI
workflow; those live in higher-level packages and applications.

Install it from a Pixils package with:

```clojure
{:dependencies {pixils-tilemap {:path "../../lib/pixils-tilemap"}}}
```

Adjust the relative path to match the package location.

Use the runtime model namespace for map geometry, layer editing, and tile
lookup:

```clojure
(ns game.map
  (:require [pixils.tilemap.model :as tilemap]))

(def tilesets
  [{:id :game
    :tiles [{:id :floor :type :color :color {:r 0x30 :g 0x30 :b 0x30}}
            {:id :wall :type :sprite :image :game/tiles :source {:x 0 :y 0 :w 16 :h 16}}]}])

(def level
  (let [tilemap {:width 4
                 :height 3
                 :tile-size 16
                 :layers []}
        ground (tilemap/filled-layer tilemap
                                     {:id :scene/ground
                                      :tileset :game
                                      :tile :floor})
        tilemap (tilemap/add-layer tilemap ground)]
    (tilemap/set-layer-cell tilemap :scene/ground {:x 1 :y 1} :wall)))
```

Layer cells store tile refs. A tile ref can be a tile id keyword, a numeric
index into the layer tileset, or `nil` for no tile. Terrain layers use
`:data-kind :terrain` and `:terrain-set`; tile layers use `:data-kind :tile-ref`
and `:tileset`.

When tilesets use property sets, applications can bake the effective property
values onto each tile before runtime lookup:

```clojure
(def runtime-tilesets
  (tilemap/apply-tilesets-property-defaults property-sets tilesets))

(def wall
  (tilemap/tile-in runtime-tilesets :game :wall))

(get (:properties wall) :blocked)
```

`apply-tilesets-property-defaults` applies property-set defaults, then tileset
defaults, then per-tile values. `apply-terrain-sets-property-defaults` does the
same for terrain sets and terrains.

Sparse layer patches update existing map layers without assigning game-specific
meaning to the cells:

```clojure
(tilemap/apply-layer-patch
 tilemap
 {:layer-id :scene/ground
  :cells [{:pos {:x 1 :y 1} :tile :wall}
          {:pos {:x 2 :y 1} :tile nil}]})
```

Use `:tile` for tile refs, or `:value` when the layer stores another cell value
shape. Patch positions outside the map are ignored. Bounds checks use
`pixils.tilemap.model/in-bounds?`, which checks the map rectangle through
`pixils.rect/inside?`.

Render a map with `pixils.tilemap.renderer/render-layers!`:

```clojure
(ns game.view
  (:require [pixils.tilemap.renderer :as renderer]))

(defun render-level! [state ctx]
  (renderer/render-layers!
   (:tilemap state)
   {:tilesets (:tilesets state)
    :terrain-sets (:terrain-sets state)
    :offset (:camera state)
    :zoom (:zoom state)
    :target-rect {:x 0 :y 0 :w 320 :h 180}
    :show-grid? (:show-grid? state)}))
```

Useful renderer options:

- `:tilesets` supplies tile definitions referenced by tile layers.
- `:terrain-sets` supplies terrain definitions referenced by terrain layers.
- `:layers` overrides the tilemap's own `:layers` for this render pass.
- `:hidden-layer-indices` skips selected layer indexes.
- `:offset` scrolls the rendered map in pixels.
- `:zoom` scales tile size for rendering.
- `:target-rect` clips rendering and limits work to visible cells.
- `:tile-substitutions` maps tile refs to replacement tile refs, or maps with
  `{:tile replacement :render-opts opts}`.
- `:show-grid?` draws the grid overlay.

`render-layers!` automatically uses the package's native renderer when the map
is already render-ready. Callers do not opt in; unsupported render inputs fall
back to the Roo renderer.

The native path handles maps whose non-hidden layers are prepared tile-stack
layers:

```clojure
{:width 40
 :height 25
 :tile-size 32
 :layers [{:id :scene
           :kind :prepared-tile-stack
           :tiles [[[{:type :sprite
                      :image :game/tiles
                      :source {:x 0 :y 0 :w 32 :h 32}}]
                    []
                    nil]]}]}
```

Prepared tile-stack cells contain actual tile definitions, not tile ids or
terrain refs. `nil` and `[]` both mean "draw nothing" for a cell. This shape is
useful when an application keeps a render map that is patched as state changes,
so rendering can draw the prepared cells directly.

Supported tile definitions in the native path are:

```clojure
{:type :sprite :image :game/tiles :source {:x 0 :y 0 :w 32 :h 32}}
{:type :image  :image :game/portrait}
{:type :color  :color {:r 0 :g 0 :b 0 :a 128}}
```

Sprite and image tiles can opt into explicit draw geometry when their visual
asset is larger than the logical map cell:

```clojure
{:id :tall-wall
 :type :sprite
 :image :game/tiles
 :source {:x 0 :y 0 :w 16 :h 32}
 :draw-offset {:x 0 :y -16}}
```

`:draw-offset` is relative to the logical cell origin in unscaled tile pixels.
`:draw-size` can be supplied to override the source dimensions; otherwise the
source rect or image size is used. The renderer pads visible ranges for these
tiles so an oversized sprite can still draw when its anchor cell is just outside
the viewport.

The Roo renderer is used instead when any non-hidden layer is not
`:prepared-tile-stack`, when `:show-grid?` is enabled, or when
`:tile-substitutions` is non-empty. This includes ordinary tile-ref layers,
terrain layers, unresolved tile-stack layers, and editor/debug rendering that
needs grid or substitution behavior.

Use `pixils.tilemap.render/tile!` when only a single tile needs to be drawn:

```clojure
(ns game.tile
  (:require [pixils.tilemap.render :as tile-render]))

(tile-render/tile! tileset :wall {:pos {:x 8 :y 8}})
```

Use `pixils.tilemap.resources/ensure-render-input-resources!` before rendering
runtime input that carries resource bundle declarations. It creates only the
image resources used by the supplied tilesets:

```clojure
(ns game.resources
  (:require [pixils.tilemap.resources :as resources]))

(resources/ensure-render-input-resources!
 {:resources {:bundles {:game {:images {:tiles {:file-name "tiles.png"}}}}}
  :tilesets tilesets})
```

`pixils.tilemap.materialize` converts terrain layers and terrain-stamp rules
into plain renderable tile-ref layers:

```clojure
(ns game.materialized
  (:require [pixils.tilemap.materialize :as materialize]))

(materialize/materialize-render-map tilemap
                                    {:terrain-sets terrain-sets
                                     :rulesets rulesets})
```

Materialization can also apply stable weighted tile substitutions after terrain
preview tiles and terrain-stamp output have been resolved. Rules target a tile
inside a tileset; choices are selected by a deterministic hash of `:seed`, `x`,
and `y`, so full-map and rect materialization agree:

```clojure
(materialize/materialize-render-map
 {:width 5
  :height 1
  :tile-substitution-rules [{:tileset :world
                             :tile :floor
                             :seed 7
                             :choices [{:tile :floor :weight 4}
                                       {:tile :floor-crack :weight 1}]}]
  :layers [{:id :ground
            :tileset :world
            :tiles [[:floor :floor :floor :floor :floor]]}]})
```

Include the original tile as one of the choices when it should remain part of
the output distribution.

`materialize-render-rect` does the same for a sub-rectangle, useful for palette
previews, dirty-region updates, and tests.

`pixils.tilemap.string-map` is a compact test and fixture format. It converts
between tilemaps and arrays of strings using each tile's `:char`:

```clojure
(ns game.fixtures
  (:require [pixils.tilemap.string-map :as string-map]))

(string-map/string-map->map ["..WW"
                             ".P W"]
                            {:tilesets tilesets
                             :tileset :game
                             :layer {:id :scene/level}})

(string-map/map->string-map tilemap
                            {:tilesets tilesets
                             :layers [:scene/level]})
```

Run the package tests with:

```sh
make test
```
