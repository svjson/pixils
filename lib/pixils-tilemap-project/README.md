# Pixils Tilemap Project

`pixils-tilemap-project` is the editor/project document layer for Pixils
tilemaps. It normalizes tilemap-editor documents, keeps project-only data such
as brushes and layer profiles, strips editor-only resource bundles from runtime
output, and produces render input compatible with `pixils-tilemap`.

Install it from a Pixils package with:

```clojure
{:dependencies {pixils-tilemap-project {:path "../../lib/pixils-tilemap-project"}}}
```

Adjust the relative path to match the package location.

The project document format is identified by:

```clojure
pixils.tilemap.project.model/project-format
; => :pixils.tilemap-editor/project
```

A project document can contain:

- `:resources` with project-owned resource bundles.
- `:tilesets` with `:color`, `:image`, or `:sprite` tile definitions.
- `:terrain-sets` with terrain definitions and preview tiles.
- `:rulesets` for generated terrain-stamp output.
- `:layer-profiles` describing source and target layers.
- `:brushes` normalized against the selected layer profile.
- `:maps` containing one or more tilemap documents.
- `:selected-map` identifying the active map.

Normalize project documents through `pixils.tilemap.project.model`:

```clojure
(ns editor.project
  (:require [pixils.tilemap.project.model :as project]))

(def document
  (project/normalize-project-document
   {:format project/project-format
    :resources {:bundles {:game-assets {:images {:tiles {:file-name "tiles.png"}}}}}
    :tilesets [{:id :game
                :tiles [{:id :wall
                         :type :sprite
                         :image :game-assets/tiles
                         :source {:x 0 :y 0}}]}]
    :selected-map :map/main
    :maps [{:id :map/main
            :width 4
            :height 3
            :layers [{:id :scene/level
                      :tileset :game
                      :data-kind :tile-ref
                      :tiles [[:wall nil nil nil]
                              [nil nil nil nil]
                              [nil nil nil :wall]]}]}]}))
```

Normalization fills defaults and makes the document easier for editor UI to
consume:

- missing map dimensions default to `40x28`
- missing tile size defaults to `16`
- map dimensions are clamped to `1..4096`
- sprite tiles get a default source rectangle
- layer tiles are normalized so serialized `nil` values behave like empty cells
- missing layer profiles are derived from the selected map layers
- selected map and selected layer profile ids are made valid
- brushes and terrain-stamp rules are normalized against the active layer
  profile

Use `project-document` when saving editor state back to a project file:

```clojure
(project/project-document
 {:selected-map-id :map/main
  :selected-map-label "Main"
  :map-width 40
  :map-height 28
  :tile-size 16
  :resources resources
  :tilesets tilesets
  :layers layers
  :layer-profiles layer-profiles
  :selected-layer-profile-id :default
  :terrain-sets terrain-sets
  :rulesets rulesets
  :brushes brushes})
```

Use `pixils.tilemap.project.runtime` to produce runtime data. `tilemap-pack`
normalizes the project and returns the runtime pack shape:

```clojure
(ns game.assets
  (:require [pixils.tilemap.project.runtime :as runtime]))

(def pack (runtime/tilemap-pack document))
```

Runtime packs use:

```clojure
{:format :pixils.tilemap/pack
 :version 1
 :resources resources
 :tilesets tilesets
 :terrain-sets terrain-sets
 :maps maps
 :selected-map selected-map-id}
```

`render-input` is the usual bridge into `pixils-tilemap` rendering:

```clojure
(def input (runtime/render-input document))

; input contains:
; {:tilemap selected-map
;  :resources runtime-resources
;  :tilesets tilesets
;  :terrain-sets terrain-sets
;  :layers (:layers selected-map)}
```

Before rendering, pass that input through
`pixils.tilemap.resources/ensure-render-input-resources!`, then render with
`pixils.tilemap.renderer/render-layers!`:

```clojure
(ns game.map-runtime
  (:require [pixils.tilemap.resources :as resources]
            [pixils.tilemap.renderer :as renderer]))

(resources/ensure-render-input-resources! input)

(renderer/render-layers! (:tilemap input)
                         {:tilesets (:tilesets input)
                          :terrain-sets (:terrain-sets input)
                          :layers (:layers input)})
```

Run the package tests with:

```sh
make test
```
