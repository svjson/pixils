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
