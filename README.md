# Pixils!

Pixils! is a runtime for building 2D graphical applications in
[Roo](https://github.com/svjson/roo-lang) - a Lisp with a modern, practical flavor.
You write your application entirely in Roo and run it with a single command. Pixils takes
care of the rendering loop, window management, input, and asset loading - powered by SDL3
underneath, invisible to your scripts.

The result is a tight feedback loop for building anything from games to tools to editors:
define your modes, write your logic, run it. No compilation step for your application code,
no C++ required.

Pixils is general-purpose. A tile-based game, a C64 asset editor, and a Minesweeper clone are
equally valid Pixils applications.

## Third-party code

Pixils vendors Clipper2 for polygon boolean operations. See
`lib/pixils/THIRD_PARTY.md` for provenance, license, and build/distribution
details.

## The `pixils` binary

Run an application from a Roo script with a single command:

```bash
pixils path/to/main.roo
pixils path/to/app/          # loads main.roo from that directory
```

## Running through `roo`

Package-based applications can launch through Roo's package tool mechanism
instead of invoking the `pixils` CLI binary directly. Add `pixils-runner` as a
dependency and declare it as the package runner:

```clojure
{:name my-pixils-app
 :dependencies {pixils-runner {:path "../lib/pixils-runner"}}
 :load-roots ["src"]
 :entry-points [my-pixils-app.core]
 :run pixils-runner}
```

Then run the application package with:

```bash
roo pixils-runner
```

The runner resolves the application package root from Roo's tool context,
loads the app entry points into a Pixils runtime, initializes SDL, and starts
the Pixils client in-process.

## Writing an application

A Pixils application is a single `.roo` file (or a collection of files loaded from
`main.roo`). It declares a program, one or more modes, and wires them together.

### `defprogram`

`defprogram` declares the application entry point: its display configuration and the mode
that runs first.

```clojure
(pixils/defprogram my-app
  {:display      {:resolution {:w 320 :h 200}
                  :align      :center
                  :scaling    :fit
                  :background {:r 0 :g 0 :b 0}}
   :initial-mode 'main/game-mode})
```

**Display options**

| Key           | Values                                          | Description                                                              |
|---------------|-------------------------------------------------|--------------------------------------------------------------------------|
| `:resolution` | `{:w N :h N}`, `:auto`, or `{:scale N}`        | Logical render buffer size. `:auto` matches the window. `{:scale N}` uses window size with N physical pixels per logical pixel. |
| `:align`      | `:center`                                       | How the buffer is positioned in the window when smaller than the window. |
| `:scaling`    | `:fit`, `:stretch`                              | How the buffer is scaled up to the window.                               |
| `:background` | `{:r N :g N :b N}` or `{:r N :g N :b N :a N}` | Clear color used each frame (0-255).                                     |

**Other `defprogram` options**

| Key        | Values | Description                                                     |
|------------|--------|-----------------------------------------------------------------|
| `:theme`   | Symbol or vector of symbols | Theme layer(s) applied to the root mode tree for this program. |
| `:theme-variant` | Keyword or symbol | Optional variant name used by theme variables, such as `:dark`. |
| `:pointer` | `:off` | Hide the OS mouse cursor. Omit to leave the cursor visible.    |

### `defmode` and `defcomponent`

A mode is the unit of application state and behaviour. Every screen, panel, and reusable
component is a mode. `defmode` and `defcomponent` are interchangeable - both declare a mode.
The distinction is only in intent: `defmode` for full screens and layout containers,
`defcomponent` for reusable leaf components.

Whether something is a full-screen mode or a layout panel is decided at the point of *use*,
not at definition.

```clojure
(pixils/defmode game-mode
  {:init    (fn [state ctx] initial-state)
   :update  (fn [state ctx] new-state)
   :focusable true
   :render  (fn [state ctx] nil)})
```

All three hooks are optional. An absent hook is a no-op; an absent `:init` or `:update`
leaves the state unchanged.

Focusability is a core mode property, not a style property. Modes are not focusable by
default. Set `:focusable true` on controls or containers that should be allowed to own
keyboard focus.

**Built-in UI components**

Pixils ships reusable UI primitives under the `ui/` namespace. These are embedded
Roo components, so applications can use them like any other mode.

| Component | Purpose |
|-----------|---------|
| `ui/text` | Styled text node with scaling, alignment, and word wrapping. |
| `ui/button` | Focusable button wrapper with inner text/content styling. |
| `ui/toggle-button` | Button subtype that keeps a persistent toggled state and renders toggled-on as pressed. |
| `ui/toggle-button-group` | Data-driven group of toggle buttons with optional forced selection. |
| `ui/checkbox` | Focusable boolean toggle with a styleable box and label. |
| `ui/option-box` | Focusable option row with a styleable selection marker and label. |
| `ui/option-box-group` | Data-driven exclusive radio-style option group. |
| `ui/text-input` | Basic editable single-line text field. |
| `ui/number-input` | Integer text field with numeric filtering, min/max clamping, and keyboard stepping. |
| `ui/list-box` | Data-driven selectable list with optional multi-select and item reordering. |
| `ui/menu-bar`, `ui/popup-menu` | Data-driven menu controls. |
| `ui/window` | Lightweight draggable window primitive. |
| `ui/header-panel` | Static content panel with a styled header/title region. |
| `ui/group-box` | Titled inset-border container with the title drawn over the top border. |
| `ui/scrollbar` | Standalone horizontal or vertical scrollbar primitive. |
| `ui/scrollbar-corner` | Filler component for the square where two scrollbars meet. |
| `ui/scroll-pane` | Scrollable viewport composed from a clipped content area and stock scrollbars. |
| `ui/progress-bar` | Passive horizontal or vertical progress indicator for literal or bound numeric values. |
| `ui/dialog-frame` | Full-screen overlay used by dialog helpers such as `open-confirm!` and `open-dialog!`. |
| `ui/file-dialog-body` | File chooser body used by `pixils.ui.file-dialog/open-file-dialog!`. |
| `ui/icon` | Focusable visual item primitive with selection, activation, and drag events. |
| `ui/icon-preview` | Non-hit-tested overlay primitive for rendering a dragged icon representation. |
| `ui/desktop-icon` | Desktop-style icon composed from an image and centered label. |
| `ui/desktop-icon-preview` | Non-hit-tested desktop-icon variant for drag previews. |
| `ui/icon-container` | Focusable icon coordinator/drop surface with optional grid layout, snapping, reordering, and keyboard navigation. |

Focused `ui/button` views activate their `:on-click` handler when Enter/Return
is pressed; the keyboard activation is delivered as a left-button click event.

`pixils.ui.window/make` creates the standard window view map. The window root
owns focus, movement, positioning, and future window-level behavior; the chrome
option controls which visible frame is composed around the caller-provided body.

```clojure
(pixils.ui.window/make
  {:title-bar {:title "Tools"}
   :style {:width 180 :height 140}
   :body [{:mode 'tools-panel}]})
```

Windows auto-focus by default and center themselves in the render buffer when
`:position` is not supplied. Auto-positioned windows are clamped into the buffer
so the visible frame remains reachable. Supplying `:position` makes the
coordinate explicit, so Pixils preserves it even when it is off-screen. Pass
`:auto-focus? false` to opt out of automatic focus. Pass `:auto-focus :first-body`
to focus the first enabled focusable descendant inside `ui/window-body`; when no
body descendant can take focus, the window itself is focused.

The current chrome variants are:

| `:chrome` | Behavior |
|-----------|----------|
| `:standard` | Default window chrome: title bar plus `ui/window-body`. |
| `:frame` | Fixed body-only frame with normal window border styling. |
| `:dialog-frame` | Fixed body-only dialog frame. In the Windows 3 theme this uses the thicker blue dialog border. |
| `:none` | Body-only chromeless window. |

`:kind` names the semantic role of the window and chooses a default chrome when
`:chrome` is omitted:

| `:kind` | Default `:chrome` |
|---------|-------------------|
| omitted or `:window` | `:standard` |
| `:panel` | `:none` |
| `:frame` | `:frame` |
| `:dialog-frame` | `:dialog-frame` |

Use `:kind` when the role is meaningful and the theme's default representation is
what you want. Use `:chrome` when the frame variant must be explicit.

```clojure
;; A plain fixed frame.
(pixils.ui.window/make
  {:kind :frame
   :style {:width 160 :height 80}
   :body [{:mode 'score-summary}]})

;; A Windows-3-style dialog frame when the windows-3 theme is active.
(pixils.ui.window/make
  {:kind :dialog-frame
   :style {:width 180 :height 100}
   :body [{:mode 'highscore-table}]})

;; A chromeless window that still participates in focus and positioning.
(pixils.ui.window/make
  {:kind :panel
   :body [{:mode 'floating-palette}]})
```

The public capability map is currently stored under `:window/capabilities` on
the window state. `:standard` windows default to movable, minimizable, and
system-menu capable. Fixed frames default those capabilities off. Callers may
override capability flags with `:capabilities`, for example
`{:movable? false}`. At the moment movement is the implemented behavior;
close/minimize/resize capabilities are reserved for follow-up passes.

`ui/toggle-button` is a subtype of `ui/button`. It accepts the same public
button state, plus `:toggled?`. When `:toggled?` is true, the button is treated
as pressed even when the mouse is not down. On click it toggles `:toggled?` and
emits `:toggle-button/change` with `{:toggled? bool :value value :index index}`.

```clojure
(pixils/defmode toolbar
  {:on {:toggle-button/change
        (fn [state event ctx]
          (assoc state :grid-visible? (-> event :payload :toggled?)))}
   :children [(pixils.ui.button/make-toggle-button
               {:label "Grid"
                :value :grid
                :toggled? (pixils.ui/bind-state :grid-visible?)})]})
```

Standalone toggle buttons may be switched off by clicking them again. Add
`:selection-required? true` or `:force-selection? true` when a standalone toggle
must stay on after it has become selected.

```clojure
{:mode 'ui/toggle-button
 :state {:label "Snap"
         :value :snap
         :toggled? true
         :selection-required? true}}
```

For mutually exclusive buttons, use `pixils.ui.button/make-toggle-button-group`.
Each entry in `:buttons` should provide at least `:label` and `:value`. The group
stores the selected button value in `:selected`, updates child `:toggled?` states,
and emits `:toggle-button-group/change` with
`{:selected value :value value :index index}`.

```clojure
(pixils/defmode tools-panel
  {:on {:toggle-button-group/change
        (fn [state event ctx]
          (assoc state :tool (-> event :payload :selected)))}
   :children [(pixils.ui.button/make-toggle-button-group
               {:buttons [{:label "Draw" :value :draw}
                          {:label "Erase" :value :erase}
                          {:label "Fill" :value :fill}]
                :selected (pixils.ui/bind-state :tool)
                :button-row-style {:layout {:direction :row}}})]})
```

Grouped toggle semantics are configured with `:selection-required?`:

| Setting | Behavior |
|---------|----------|
| omitted or `false` | Clicking the selected button clears the selection, so no button may be toggled on. |
| `true` | One selectable button is always toggled on. If `:selected` is missing or disabled, the first selectable button is chosen. |

`:force-selection? true` is accepted as an alias for `:selection-required? true`
on both standalone toggle buttons and groups.

`pixils.ui.option-box/make` creates a radio-button-style option row with a
themeable circular selection glyph. It accepts `:label`, `:value`, `:selected?`,
`:disabled?`, and the same `:selection-required?` / `:force-selection?` flags as
toggle buttons. Clicks emit `:option-box/change` with
`{:selected? bool :value value :index index}`. Option boxes keep themselves
selected by default; set `:selection-required? false` only when a standalone
option should be allowed to toggle off.

For mutually exclusive choices, use `pixils.ui.option-box/make-group`. Each
option can provide `:label`, `:value`, `:id`, `:disabled?`, `:style`, `:class`,
and extra child `:state`. The group stores the selected value in `:selected`,
keeps child `:selected?` states in sync, and emits `:option-box-group/change`
with `{:selected value :value value :index index :option option}`. One selectable
option is kept selected by default; set `:selection-required? false` only when
the group should allow no selected option.

```clojure
(pixils/defmode terrain-rule-mode-panel
  {:on {:option-box-group/change
        (fn [state event ctx]
          (assoc state :terrain-rule-application (-> event :payload :selected)))}
   :children [(pixils.ui.option-box/make-group
               {:options [{:label "Preview only" :value :preview}
                          {:label "Bake on paint" :value :paint-baked}]
                :selected (pixils.ui/bind-state :terrain-rule-application)})]})
```

Use `ui/checkbox` for independent on/off choices where more than one item can
be selected at the same time.

`pixils.ui.list-box/make` accepts `:options`, where each option can provide
`:value`, `:label`, `:name`, and `:disabled?`. Selection changes emit
`:list-box/change` with `{:selected-indices [...] :selected-values [...] :value value}`.
Pass `:multi-select? true`, `:force-selection? true`, and `:toggle-selected? true`
to opt into the extended selection behavior.

List boxes can opt into drag reordering with `:reorderable? true`. The list-box
does not mutate `:options` itself; on drop it emits `:list-box/reorder`, and the
application updates its own data order. `:reorder-visual-strategy` controls
drag feedback: `:placeholder` makes the dragged row follow the cursor while the
remaining rows move around the prospective drop slot, and `:none` keeps the list
visually unchanged while dragging.

```clojure
(pixils/defmode layer-panel
  {:on {:list-box/reorder
        (fn [state event ctx]
          (let [payload (:payload event)]
            ;; Payload includes :from-index, :to-index, :drop-index, :value, and :option.
            (assoc state :last-reorder payload)))}
   :children [(pixils.ui.list-box/make
               {:options [{:value :background :label "Background"}
                          {:value :objects :label "Objects"}
                          {:value :foreground :label "Foreground"}]
                :style {:width 160}
                :reorderable? true
                :reorder-visual-strategy :placeholder})]})
```

`:drop-index` is the insertion position in the original list, while `:to-index`
is the final item index after removing the dragged item. Dropping an item back
onto its original position does not emit `:list-box/reorder`.

`ui/menu-bar` is usually created with `pixils.ui.menu/make-menu` from a menu
definition and action map. The first argument can be either the menu state map
or an options map with `:state` plus node options such as `:style`:

```clojure
(pixils.ui.menu/make-menu
  {:state {:game (pixils.ui/bind-state :game)}
   :style {:height 18}}
  menu-definition
  menu-keymap)
```

`ui/scrollbar` is intended for direct composition. Use `:axis :x` or `:axis :y`,
bind `:value` to the scroll offset you want to control, and provide `:content-size`
for the total scrollable span. Optional `:step` controls the arrow-button increment.

`ui/progress-bar` is usually created with `pixils.ui.progress-bar/make`. Bind
`:value` to the numeric state you want to display and optionally set `:min`,
`:max`, and `:axis`:

```clojure
(pixils.ui.progress-bar/make
  {:style {:width 120 :height 12}
   :value (pixils.ui/bind-state :load-progress)
   :max 100})
```

`ui/header-panel` is usually created with `pixils.ui.header-panel/make` so the
header/body structure stays consistent while applications provide body children:

```clojure
(pixils.ui.header-panel/make
  {:title "Inspector"
   :children [{:mode 'ui/text
               :state {:value "Selected: Grass"}}]})
```

`ui/group-box` is usually created with `pixils.ui.group-box/make`. It draws a
theme-styled inset frame with the title over the top border:

```clojure
(pixils.ui.group-box/make
  {:title "Options"
   :children [{:mode 'ui/checkbox
               :state {:label "Snap to grid"}}]})
```

`ui/number-input` reuses the same text editing core as `ui/text-input`, but only
accepts integer text. Bind `:value` to numeric state, use optional `:min`, `:max`,
and `:step`, and listen for `:number-input/change` when an ancestor needs the
change payload. Up/down arrows step the value and clamp it to the configured
range.

```clojure
{:mode 'ui/number-input
 :state {:value (pixils.ui/bind-state :tile-size)
         :min 1
         :max 64
         :step 1}}
```

The scrollbar uses its effective style as its visual default: `:background` for
the track, arrow buttons, and handle; `:border :color` for outlines; and
`:text :color` for arrow glyphs. State colors can still override individual
parts: `:track-color`, `:button-color`, `:button-border-color`, `:handle-color`,
`:handle-pressed-color`, and `:glyph-color`.

`ui/scroll-pane` is a convenience composition around `ui/scrollbar`. Use
`pixils.ui.scroll-pane/make` when supplying arbitrary child content:

```clojure
(pixils.ui.scroll-pane/make
  {:style {:width 160 :height 120}
   :content-size {:w 160 :h 320}
   :children [{:mode 'main/list-content}]})
```

The first version expects explicit `:content-size` and stores its offset in
`{:offset {:x N :y N}}`. The viewport uses `:clip true`, and the content view is
positioned absolutely inside that clipped area. Pass `:scroll-x? false` or
`:scroll-y? false` to omit one axis from helper-created panes. `:content-state-key`
can wrap the pane's `:content-state` binding in a named child-state key when the
scrolling content needs state from the owner.

`pixils.ui.dialog/open-confirm!` opens a modal confirmation dialog and sends a result event
back to the opening view when the dialog closes. By default it shows a single OK button and
emits `:dialog/result`; override that with `:result-event`.

```clojure
(pixils.ui.dialog/open-confirm!
 ctx
 {:title "Delete layer"
  :body [{:mode 'ui/text
          :state {:value "Delete the selected layer?"}}]
  :buttons [{:type :dialog/ok :label "Delete"}
            :dialog/cancel]
  :payload {:layer-id (:selected-layer state)}
  :result-event :layer/delete-result})
```

The result payload has the shape `{:choice choice :payload payload}`. Button choices are
the button `:type`, such as `:dialog/ok`, `:dialog/cancel`, `:dialog/yes`, or `:dialog/no`.
The built-in button presets are `:dialog/ok`, `:dialog/ok-cancel`, `:dialog/yes-no`, and
`:dialog/yes-no-cancel`; vectors are rendered left-to-right, so callers control ordering.
Dialog windows use `:auto-focus :first-body` by default, so the first enabled
input or button in the body receives initial focus.

`pixils.ui.dialog/open-dialog!` is the lower-level helper for custom dialog windows. It
pushes `ui/dialog-frame`, creates the window, and wraps result handlers so user code returns
the value to deliver to the origin instead of calling `pop-mode!` itself. Use `:state` for
dialog-local state, `:body` for the window body, and `:results` to map custom events to
return values.

```clojure
(pixils.ui.dialog/open-dialog!
 ctx
 {:title (pixils.ui/bind-state :title)
  :state {:title "New component"
          :name ""
          :type :sprite}
  :body {:mode 'new-component-form}
  :result-event :component/dialog-result
  :results {:dialog/confirm
            (fn [state event ctx]
              {:name (:name state)
               :type (:type state)})
            :dialog/cancel
            (fn [state event ctx]
              {:cancelled? true})}})
```

`open-dialog!` is modal by default. Pass `:modal false` to let events reach modes below the
dialog frame, or `:dismissable true` to allow Escape/outside-click dismissal. Dismissal uses
the standard confirm result shape with `:choice :dialog/dismiss`.

`pixils.ui.file-dialog/open-file-dialog!` opens a Pixils-provided file chooser on top of
`open-dialog!`. It uses the active `roo.io` filesystem and keeps directories visible
while applying the selected filter to files. The result shape is
`{:type :confirm :mode mode :path path :directory directory
:filename filename :paths paths :filenames filenames :filter filter}` or
`{:type :cancel ...}`. `:path` and `:filename` are the first selected file, kept
for single-file callers.

```clojure
(pixils.ui.file-dialog/open-file-dialog!
 ctx
 {:title "Open Project"
  :mode :file-dialog/open
  :path pixils.ui.file-dialog/default-path
  :filters [{:label "EDN files (*.edn)"
             :extensions [".edn"]}]
  :result-event :project/open-result})
```

Use `:mode :file-dialog/save` for save dialogs. The helper also accepts `:position`,
`:style`, `:filename`, and `:result-event`. Open dialogs accept `:multi-file? true`
or `:multi-select? true` to allow selecting more than one file.

`ui/icon` is intentionally only the item primitive. It expects an `:item` in
state, optionally positioned by `:position` on either the state or item map, and
emits `:ui/icon-select`, `:ui/icon-activate`, `:ui/icon-drag-start`,
`:ui/icon-drag-move`, and `:ui/icon-drag-end`. `ui/icon-container` is the
coordinator/drop surface: when an icon drag is released over it, it emits
`:ui/icon-drop` with `{:item item :target target :position point}`. The target
comes from container state `:target` or `:path`. Applications decide what the
item and drop mean; filesystem moves, ownership, and persistence are not part of
the core icon components.

`ui/icon-container` can also coordinate grid behavior without creating the icon
children itself. With `:snap-to-grid? true`, drops are snapped to `:grid`
settings and the drop payload also includes `:raw-position`. With
`:layout-mode :grid`, existing child views are positioned by child order. Grid
settings accept `:cell-width`, `:cell-height`, optional `:cell-size`, optional
`:origin`, optional `:columns`, and optional `:min-rows`; when `:columns` is omitted, it is derived
from the container width. In grid mode, the container reports a natural content
size from its active icon count, so `ui/scroll-pane` can measure it without an
application-provided `:content-size`. Add `:reorderable? true` in grid mode to emit
`:ui/icon-reorder` with `{:item item :target target :from-index n :to-index n}`
when a dragged icon payload matches one of the container's child items by id.
Set `:icon-count` or `:item-count` when only the first N children should be
treated as icons. Alternatively, set `:icon-count-key` to a state collection key;
the default helper uses `:items` when present.

For the common "grid in a scroll pane" case, use
`pixils.ui.icon-container/make-grid`. It creates an auto-managed `ui/scroll-pane`
containing a grid-mode `ui/icon-container`, defaults to vertical auto scrolling,
keeps at least one empty row as a drop surface, and copies the supplied `:state`
into both the scroll content and the icon container so child bindings keep
working through the scroll-pane wrapper. When the grid lives inside an owner
component and `:state` contains `ui/bind-state` values, pass
`:bind-content-state? true` so the scroll-pane content receives the owner's
runtime state instead of treating those bindings as literal content data.

```clojure
(pixils.ui.icon-container/make-grid
  {:style {:width :fill :height :fill}
   :bind-content-state? true
   :state {:items (pixils.ui/bind-state :items)
           :selected-id (pixils.ui/bind-state :selected-id)}
   :grid {:cell-width 104
          :cell-height 120
          :columns 6}
   :children [(app/inventory-icon-child 0)
              (app/inventory-icon-child 1)]})
```

When focused, `ui/icon-container` supports keyboard navigation over its current
children. Arrow keys move by one cell or row, Home/End jump to the first/last
child, and Enter emits `:ui/icon-activate` for the selected child. Keyboard
selection emits `:ui/icon-select` with
`{:item item :index index :id id :input-source :keyboard}`. The container uses
`:selected-index`, `:selected-id`, or child `:selected?` / `:selected` state to
find the current selection.

`ui/desktop-icon` extends `ui/icon` with stock image and label children. Its
update step derives `:image` from `(:image item)` or state `:image`, derives
`:label` from `(:label item)`, `(:title item)`, `(:name item)`, or state
`:label`, and marks the icon selected when state `:selected?` is true or when
`:selected-id` matches the item `:id`.

The `pixils.ui.desktop-icon/make-desktop-icon` make function creates a complete
desktop icon definition from the data needed to wire those children:

```clojure
(ns app.desktop
  (:require [pixils.ui :as ui]
            [pixils.ui.desktop-icon :as desktop-icon]
            [pixils.ui.drag :as drag]))

(drag/make-draggable
 (desktop-icon/make-desktop-icon
  {:item {:id :readme
          :label "README"
          :image :app/readme-icon
          :position {:x 32 :y 32}}
   :selected-id (ui/bind-state :selected-id)
   :style {:cursor :pointer}
   :icon {:style {:width 32
                  :height 32}}
   :label {:style {:width 72}}})
 {:start-event :ui/icon-drag-start
  :move-event :ui/icon-drag-move
  :end-event :ui/icon-drag-end})
```

Top-level `:item`, `:image`, `:position`, `:selected-id`, `:selected?`, and
non-map `:label` entries are copied into component state. Use `:state` when
state should be bound or supplied as a map. If label-child options are also
needed, put the display label in `:item` or `:state`, because a map-valued
top-level `:label` configures the label child.

The optional `:icon` and `:label` maps customize the generated image and label
children. Each accepts `:mode`, `:style`, `:state`, and `:state-keys`.
`:state-keys` defaults to `[:image :dragging?]` for `:icon` and
`[:label]` for `:label`; listed keys are bound from the desktop icon
state into the child state. `make-desktop-icon` also passes through `:init`,
`:update`, `:on`, `:on-click`, `:on-double-click`, and `:on-drop`.

Use `pixils.ui.desktop-icon/make-desktop-icon-preview` for drag overlays. It
accepts the same options but extends `ui/desktop-icon-preview`, which inherits
the same desktop icon rendering while disabling hit testing.

**Hook signatures**

All hooks receive the same two arguments: the current state and a unified context object.

| Hook      | Arguments     | Return value                                                   |
|-----------|---------------|----------------------------------------------------------------|
| `:init`   | `[state ctx]` | Initial state. Returned value replaces the passed-in state.   |
| `:update` | `[state ctx]` | New state for the next frame.                                  |
| `:render` | `[state ctx]` | Ignored. Called for side effects only.                         |

A hook value can be an anonymous function or a quoted symbol naming a top-level function
declared with `defun`:

```clojure
(defun my-render! [state ctx]
  (pixils.render/rect! ...))

(pixils/defmode my-mode
  {:render 'my-ns/my-render!})
```

**`ctx` - unified per-frame context**

`ctx` bundles display information, input state, and the current view. All keys are accessible
with standard map lookup.

| Key                   | Description                                                                              |
|-----------------------|------------------------------------------------------------------------------------------|
| `:buffer-size`        | Map with `:w` and `:h` of the logical render buffer                                     |
| `:pixel-size`         | Physical pixels per logical pixel (scaling factor)                                       |
| `:key-down`           | Keyword for the key pressed this frame, or `nil`                                         |
| `:held-keys`          | Set of keywords for all currently held keys                                              |
| `:mouse-pos`          | Current cursor position as a point (`{:x N :y N}`)                                      |
| `:mouse-button-down`  | Keyword for the button pressed this frame (`:left`, `:right`, `:middle`), or `nil`      |
| `:mouse-button-up`    | Keyword for the button released this frame (`:left`, `:right`, `:middle`), or `nil`     |
| `:mouse-wheel`        | Wheel delta for this frame as a point (`{:x N :y N}`), or `nil` when no wheel event occurred |
| `:mouse-held`         | Set of mouse buttons currently held                                                      |
| `:view`               | The current view (see below)                                                             |

**`ctx :view`**

The `:view` key gives access to the current component's live view instance.

| Key            | Description                                                                    |
|----------------|--------------------------------------------------------------------------------|
| `:id`          | The view's unique identifier string                                            |
| `:bounds`      | The view's bounding rect in the buffer: `{:x N :y N :w N :h N}`               |
| `:interaction` | Interaction state: map with `:hovered`, `:focused`, `:focus-within`, and `:pressed` |
| `:style`       | The view's current style object. The `:hidden` property is mutable.           |

The bounds reflect the viewport assigned by the layout engine in buffer coordinates, not
local to the component. Use bounds to compute absolute positions for overlays or popups.

**Example - a simple counter**

```clojure
(pixils/defmode counter-mode
  {:init   (fn [state ctx]
             {:count 0})

   :update (fn [state ctx]
             (if (= (:key-down ctx) :key/space)
               (assoc state :count (+ (:count state) 1))
               state))

   :render (fn [state ctx]
             (pixils.render/use-color! 255 255 255 255)
             nil)})
```

**Time-based state timers**

`pixils.state.timer` provides a small state value for real-time cadence. A timer records
whether its interval elapsed when ticked:

```clojure
(ns my-game
  (:require [pixils.state.timer :as timer]))

(pixils/defmode game-mode
  {:init (fn [state ctx]
           (assoc state :physics-timer (timer/make {:interval-ms 120})))

   :update (fn [state ctx]
             (let [state (timer/tick-at state :physics-timer)]
               (if (timer/ticked-at? state :physics-timer)
                 (update-physics state)
                 state)))})
```

Missed intervals are not replayed. A timer can tick at most once per frame.

**Derived modes**

A mode can extend another mode with `:extend`, inheriting its hooks, style, and children.
The derived mode starts as a copy of the named base and applies only the keys present in
its own definition - absent keys retain the base value.

```clojure
(pixils/defcomponent highlight-button
  {:extend 'ui/button
   :style  {:background {:r 80 :g 160 :b 80}
            :hover      {:background {:r 100 :g 200 :b 100}}}})
```

`:extend` works with any key that `defmode` accepts. A hook present in the derived
definition replaces the inherited one. Event handlers under `:on` are merged: derived
handlers overlay the base set, with the derived handler winning if the same key appears
in both.

### Mode stack

Modes are managed as a stack. The top mode is the active one. Pushing a new mode suspends
the current one; popping returns to it.

```clojure
; Push a mode onto the stack with optional initial state
(pixils/push-mode! 'main/pause-menu {:resumed-from state})

; Push a mode and override fields from its definition
(pixils/push-mode! 'main/popup-menu
  {:anchor {:x 0 :y 24}}
  {:style    {:background {:r 30 :g 30 :b 30}}
   :children [{:mode 'main/menu-item :style {:height 20} :state {:label "New"}}
              {:mode 'main/menu-item :style {:height 20} :state {:label "Open"}}]})

; Pop the top mode and return to the one below
(pixils/pop-mode!)

; Pop with a payload that is returned to the pusher
(pixils/pop-mode! {:selected :expert})
```

The optional third argument to `push-mode!` is an override map. It accepts any key that
`defmode` accepts (hooks, `:style`, `:children`, `:focusable`) and merges them onto the base mode for
that specific push only. It also accepts `:origin`, which controls where a later
`pop-mode!` result event is delivered. The registered mode definition is not modified.

Mode transitions are message-based and take effect between frames, so it is safe to call
`push-mode!` or `pop-mode!` from inside any hook.

**Anchored overlays and popovers**

For popovers, dropdowns, and other pushed modes that need to appear relative to
an existing view, use `:overlay` in the third `push-mode!` argument. Overlay
placement is applied after the pushed mode has been laid out and before it is
rendered, so it has real anchor and popup geometry. This avoids the one-frame
jump that can happen when a component computes absolute coordinates from
`ctx.view.bounds` during `:update`.

```clojure
(pixils/push-mode! 'main/action-popover
  {:options options}
  {:origin {:view (:view ctx)
            :event :action/select}
   :overlay {:anchor (:view ctx)
             :placement :bottom-start
             :fallback-placement :top-start
             :viewport-padding 8}
   :children [{:mode 'main/action-popover-panel
               :style {:position :absolute
                       :left 0
                       :top 0
                       :width :shrink
                       :height :shrink}}]})
```

The overlay anchor must be a view. `:placement` currently supports
`:bottom-start` and `:top-start`; `:fallback-placement` also accepts `:none`.
Use `:match-anchor-width? true` when the overlay should inherit the anchor's
visual width. Use `:viewport-padding` to keep the overlay away from the render
buffer edge.

The pushed overlay remains a normal mode. Style the popup panel with ordinary
layout/style keys, but let `:overlay` own the final screen placement.

**Returning data from a pushed mode**

When a mode is popped, it can return data to the view tree that is exposed underneath it.
By default, the runtime emits a `:pop/result` custom event from the newly exposed root view.

If the pushed mode should return to a specific initiating view instead, pass `:origin` in
the third argument to `push-mode!`:

```clojure
; Return to a specific view with the default :pop/result event
(pixils/push-mode! 'main/popup-menu
  nil
  {:origin (:view ctx)})

; Return to a specific view with a custom event name
(pixils/push-mode! 'main/popup-menu
  nil
  {:origin {:view (:view ctx)
            :event :menu/select}})
```

Then, from the pushed mode:

```clojure
(pixils/pop-mode! {:selected :expert})
```

The delivered event is a normal custom event whose metadata identifies the popped mode:

```clojure
{:event-key :pop/result
 :source-mode 'main/popup-menu
 :payload {:selected :expert}}
```

This lets the receiver distinguish which mode was popped even when multiple pushed modes
return results through the same handler, while keeping the payload itself unchanged.

### Mode composition

By default, only the top mode runs each frame. `:compose` controls whether the mode below
the top also participates in update and/or render - useful for overlays and HUDs.

```clojure
(pixils/defmode hud-overlay
  {:compose {:render :pass :update :pass}
   :render  (fn [state ctx] ...)})
```

With `:render :pass`, both this mode and the one below render each frame (the lower mode
first). With `:update :pass`, both update each frame.

### Children and layout

A mode can declare child modes laid out inside its area. Children are sized and positioned
by the framework; each child renders into its own viewport and does not need to know where
it is placed.

```clojure
(pixils/defmode root-layout
  {:children [{:mode 'main/menu-bar  :style {:height 12}}
              {:mode 'main/game-area}]})
```

Children are laid out in a column by default (top to bottom). Set `:style {:layout {:direction :row}}`
to arrange them left to right instead.

```clojure
(pixils/defmode toolbar
  {:style    {:layout {:direction :row}}
   :children [{:mode 'main/file-button :style {:width 48}}
              {:mode 'main/edit-button :style {:width 48}}
              {:mode 'main/help-button}]})
```

Sizing is declared in the `:style` map. A child with `:style {:height N}` (or
`:style {:width N}` in a row layout) is given exactly that many pixels by default as a
border box. Set `:box-sizing :content-box` to interpret fixed sizes as content dimensions
instead. A child without a size constraint fills the remaining space. Multiple fill children
share the remainder evenly. Margins are also declared in `:style`. A child's `:margin`
consumes flow space around the child and offsets its rendered bounds within the allocated slot.

The layout map can also distribute leftover space between flow children:

```clojure
(pixils/defmode toolbar
  {:style {:layout {:direction :row
                    :gap :space-between}}
   :children [{:mode 'main/file-button :style {:width 48}}
              {:mode 'main/edit-button :style {:width 48}}
              {:mode 'main/help-button :style {:width 48}}]})
```

`:gap` also accepts:
- the explicit wrapped form `{:mode :space-between}`
- a fixed number like `:gap 8`
- the explicit wrapped fixed form `{:mode :fixed :size 8}`
- `:none` for explicit default/no extra gap

Row layouts can wrap flow children onto additional lines when horizontal space runs out:

```clojure
(pixils/defmode toolbar
  {:style {:height :shrink
           :layout {:direction :row
                    :wrap :line
                    :gap 6
                    :line-gap 4}}
   :children [{:mode 'main/open-button :style {:width :shrink}}
              {:mode 'main/save-button :style {:width :shrink}}
              {:mode 'main/export-button :style {:width :shrink}}]})
```

Use natural or `:shrink` widths for text-wrap-like inline flow. Use `:width :fill`
with a numeric `:min-width` when children should wrap at a basis size and then
grow to share the remaining width on their own line:

```clojure
{:style {:layout {:direction :row
                  :wrap :line
                  :gap 8
                  :line-gap 8}}
 :children [{:mode 'main/tile :style {:width :fill :min-width 80}}
            {:mode 'main/tile :style {:width :fill :min-width 80}}
            {:mode 'main/tile :style {:width :fill :min-width 80}}]}
```

Size can come from the mode's own `:style` definition or be overridden per child slot:

```clojure
; The mode declares its own height
(pixils/defmode menu-bar
  {:style {:height 12}
   ...})

; The parent can also override height on the slot
{:mode 'main/menu-bar :style {:height 14}}
```

**Per-instance overrides**

Child slot maps accept any `defmode` key alongside the structural ones. These override the
registered mode definition for that specific slot only, without modifying the mode itself.

```clojure
{:children [{:mode 'main/button
             :state {:label "OK"}
             :on-click (fn [state event ctx] ...)
             :style {:height 20 :background {:r 60 :g 120 :b 60}}}
            {:mode 'main/button
             :state {:label "Cancel"}
             :on-click (fn [state event ctx] ...)
             :style {:height 20 :background {:r 120 :g 60 :b 60}}}]}
```

The same override map accepted by `push-mode!`'s third argument works here too.
That includes behavioral properties like `:focusable`, not just style and hooks.

**Absolute positioning**

A child can be taken out of the flow and positioned relative to its parent's content rect
with `:position :absolute`:

```clojure
{:mode 'main/popup
 :style {:position :absolute
         :top  24
         :left 10
         :width  120
         :height 80}}
```

Absolutely positioned children do not consume space in the flow layout and do not affect
the size allocated to other children. They are rendered after flow-positioned siblings,
preserving child order among other absolutely positioned children.

For a root mode, absolute positioning is relative to the root render area because there is
no parent content rect.

### Style

Modes and components can declare a style that the framework applies before their render
hook fires.

```clojure
(pixils/defcomponent menu-item
  {:style {:background {:r 60 :g 60 :b 70}
           :padding    4
           :hover      {:background {:r 90 :g 90 :b 110}}}
   :render (fn [state ctx] ...)})
```

| Style key     | Values                                                                | Description |
|---------------|-----------------------------------------------------------------------|-------------|
| `:background` | `{:r N :g N :b N}`, `{:r N :g N :b N :a N}`, `:bundle/image`, or `{:color ... :image ...}` | Background fill and/or a non-repeating image. Image maps may include `:source`, `:fit`, `:align`, `:offset`, and `:opacity`. |
| `:margin`     | Number, `[vertical horizontal]`, `[top right bottom left]`, or `{:t N :r N :b N :l N}` | Outer space around a child in layout flow. |
| `:padding`    | Number, `[vertical horizontal]`, `[top right bottom left]`, or `{:t N :r N :b N :l N}` | Inset applied before the render hook's viewport is set. |
| `:border`     | Border map (see below)                                               | Draws a border inside the component bounds. |
| `:layout`     | `{:direction :row}`, `{:direction :column}`, optional `:align-items :start|:center|:end`, optional `:gap :none`, `:gap N`, `:gap :space-between`, wrapped gap maps, optional `:wrap :none|:line`, optional `:line-gap N` | Child layout policy. Supports flow direction, cross-axis alignment of flow children, fixed gap, explicit no-gap, `space-between` distribution, and wrapped row lines. |
| `:text`       | `{:color {:r N :g N :b N}}`, `{:color :none}`, optional `:font :font/name`, optional `:scale N`, optional `:align :left|:center|:right`, optional `:wrap :word|:none` | Text presentation properties for components that render text. Text scale accepts fractional values. |
| `:box-sizing` | `:border-box`, `:content-box`                                        | How fixed `:width`/`:height` are interpreted. Default: `:border-box`. |
| `:scale`      | Integer `N`                                                          | Render this view subtree at logical size, then copy it to an external footprint scaled by `N`. Minimum effective value: `1`. |
| `:opacity`    | Number from `0.0` to `1.0`                                           | Render this whole view subtree translucent. `1.0` is fully opaque, `0.0` is invisible. |
| `:width`      | Number                                                               | Fixed width in pixels using the selected `:box-sizing`. Absent means fill remaining space. |
| `:height`     | Number                                                               | Fixed height in pixels using the selected `:box-sizing`. Absent means fill remaining space. |
| `:min-width`  | Number                                                               | Minimum width in pixels. Used as the wrapping basis for `:width :fill` children in wrapped row layouts. |
| `:min-height` | Number                                                               | Minimum height in pixels. |
| `:max-width`  | Number                                                               | Maximum width in pixels. Capped `:width :fill` children release remaining row space to uncapped fill siblings. |
| `:max-height` | Number                                                               | Maximum height in pixels. Capped `:height :fill` children release remaining column space to uncapped fill siblings. |
| `:position`   | `:absolute`, `:flow`                                                 | Positioning mode. Default: `:flow`. |
| `:top`        | Number                                                               | Top offset when `:position :absolute`. |
| `:left`       | Number                                                               | Left offset when `:position :absolute`. |
| `:hidden`     | Boolean                                                              | When true, excluded from hit-testing and rendering. Layout space is preserved. |
| `:hit-test`   | Boolean                                                              | When false, excluded from hit-testing while still rendering. Useful for overlays and drag previews. |
| `:clip`       | Boolean                                                              | When true, descendant rendering and hit testing are clipped to this view's content rect. |
| `:cursor`     | Cursor keyword, pointer keyword, or pointer map                      | Mouse cursor to show while this view, or a descendant without its own cursor, is hovered. |
| `:hover`      | Nested style map                                                     | Applied instead of the base style when the cursor is within bounds. |
| `:focus-within` | Nested style map                                                   | Applied when this view or any descendant owns focus. |
| `:focus`      | Nested style map                                                     | Applied when this exact view owns focus. |

These interaction variants are injected automatically by the framework. No manual
hit-testing or focus bookkeeping is needed in the component. When a focused leaf exists,
it receives both `:focus` and `:focus-within`; its ancestors receive `:focus-within`
only.

Background image maps support `:source {:x :y :w :h}` for spritesheet crops,
`:fit :none|:contain|:cover|:fill`, `:align` (`:top-left`, `:top`,
`:top-right`, `:left`, `:center`, `:right`, `:bottom-left`, `:bottom`,
`:bottom-right`), and `:offset {:x N :y N}`. The default remains top-left at
natural image size:

```clojure
{:style {:background {:image :editor-assets/brush
                      :source {:x 0 :y 0 :w 16 :h 16}
                      :fit :contain
                      :align :center}}}
```

`:cursor` controls the visible mouse cursor. The deepest hovered view with a cursor wins;
if that view has no cursor, Pixils falls back through its hovered ancestors. It can be
declared directly or inside interaction variants:

```clojure
(pixils/defcomponent draggable-splitter
  {:style {:cursor :resize-x
           :hover  {:background {:r 220 :g 220 :b 220}}}})

(pixils/defcomponent text-region
  {:style {:cursor :text}})
```

Supported cursor keywords are `:default`, `:pointer`, `:hand`, `:text`, `:crosshair`,
`:move`, `:not-allowed`, `:wait`, `:progress`, `:resize-x`, `:resize-y`,
`:resize-nwse`, and `:resize-nesw`. `:hand` is accepted as an alias for `:pointer`.

Custom pointers are named with `defpointer` and can reuse normal image assets. The
definition name is used exactly as written: `(defpointer workbench-pointer ...)` is
referenced as `:workbench-pointer`; use `(defpointer workbench/pointer ...)` when you
want `:workbench/pointer`. Image pointers default to `:render :app`, which hides the OS
cursor and draws the pointer into Pixils' logical buffer so it scales and snaps to the same
pixel grid as the application.

```clojure
(pixils/defbundle workbench-assets
  {:images {:cursor "assets/cursor.png"
            :cursors "assets/cursors.png"}})

(pixils/defpointer workbench/pointer
  {:image :workbench-assets/cursor
   :hotspot {:x 0 :y 0}
   :scale 1})

(pixils/defpointer precision-crosshair
  {:image :workbench-assets/cursors
   :source {:x 16 :y 0 :w 16 :h 16}
   :hotspot {:x 8 :y 8}
   :scale 2})

{:style {:cursor :workbench/pointer}}
```

Use `:render :native` when you need SDL to draw a custom cursor outside the app's logical
pixel grid:

```clojure
(pixils/defpointer native-pointer
  {:image :workbench-assets/cursor
   :hotspot {:x 0 :y 0}
   :render :native})
```

Inline custom cursor maps are also accepted for one-off use:

```clojure
{:style {:cursor {:image :workbench-assets/cursor
                  :hotspot {:x 2 :y 1}
                  :scale 2}}}
```

Use program-level `:pointer :off` when the OS cursor should be hidden entirely.

`:scale` is a view-boundary scale. The view and its descendants still lay out and
receive local mouse positions in the unscaled logical coordinate system, but the parent
layout reserves the scaled footprint. For example, a `160x100` view with `:scale 2`
behaves internally as `160x100` and externally occupies `320x200`.

```clojure
(pixils/defcomponent pixel-panel
  {:style {:width 160
           :height 100
           :scale 2}
   :children [{:mode 'ui/text
               :state {:value "Scaled panel"}}]})
```

The first version supports uniform integer scale factors only. Pixils renders the subtree
to a logical-size target texture and copies it back without component-specific font,
image, or layout scaling.

Text wrapping is enabled by default for `ui/text` when the node can see a real
available width, either from its own fixed/fill `:width` or from a constrained
ancestor. Auto-width text uses that available width as a wrapping maximum without
filling the full width itself. Use `:text {:wrap :none}` to keep it on a single
line. Explicit newline characters still create separate lines.

Programmatic focus control is also available from hooks:

```clojure
(pixils.ui/focus! ctx)         ; focus the current view
(pixils.ui/focus! (:view ctx)) ; equivalent explicit form
(pixils.ui/focus-first! ctx)   ; focus first enabled focusable descendant
(pixils.ui/blur!)              ; clear the current focused leaf
```

`focus!` only succeeds for views whose mode is `:focusable true`. Mouse focus acquisition
uses the same rule and will walk up the hit chain to the nearest focusable ancestor.
`focus-first!` accepts an optional container mode, such as
`(pixils.ui/focus-first! ctx 'ui/window-body)`, to limit the descendant search.
Pressing TAB moves focus to the next enabled, visible focusable view in tree order; pressing
Shift+TAB moves to the previous one. Traversal wraps at the ends. A focused view can keep
TAB for its own behavior by handling `:on-key-down` and calling
`(pixils.ui/stop-propagation! event)`.

With the default `:box-sizing :border-box`, a fixed size includes padding and border but
not margin. Use `:box-sizing :content-box` to opt into the previous behavior where padding
and border are added on top of the fixed content size.

### Themes

Themes let you define reusable style rules and apply them to a mode tree with `:theme`
on `defprogram`, `defmode`, or child mode entries. `:theme` accepts either one
theme symbol or a vector of theme symbols, applied left to right.

Pixils also applies an internal built-in base theme underneath any user theme. That base
theme is the framework's equivalent of browser default styles: always present, and used
as-is when no explicit theme is active.

The current built-in named themes are `pixils/classic-blue`,
`pixils/windows-3`, and `pixils/windows-95`. The Windows themes include their
bitmap font and small control images as embedded Pixils assets.

Stock themes also define a small semantic UI vocabulary for application-level
layout surfaces:

| Class | Intended use |
|-------|--------------|
| `:ui/app` | Root application surface. |
| `:ui/panel` | Generic panel or sidebar surface. |
| `:ui/title` | Section or tool heading text. |
| `:ui/muted` | Secondary text. |
| `:ui/accent` | Highlight text. |
| `:ui/canvas` | Framed drawing or editor surface. |
| `:ui/list-item` | Selectable list/palette rows; `{:selected true}` marks the selected row. |

Themes are presentation profiles, and presentation includes both visual styling
and layout policy. Applications can rely on the built-in base theme, choose a
stock theme, or define their own theme layers for application layout, density,
spacing, and visual identity.

For example, an application may define both a compact layout theme and a roomy
layout theme, then compose either one with `pixils/classic-blue` or
`pixils/windows-95`. The composition can be named with vector `:extend` in a
concrete application theme, or selected directly where the theme is applied:

```clojure
(pixils/defprogram tilemap-editor
  {:theme ['pixils/windows-3 'tilemap-editor/compact-theme]
   :initial-mode 'main-mode})
```

Component-local `:style` remains useful for intrinsic structure and one-off
constraints, but reusable application spacing and density belong naturally in
theme rules.

```clojure
(pixils/deftheme win95
  {:styles {'button {:padding 2}
            '(button {:pressed true})
            {:border {:thickness 2
                      :line-style :bevel}}}})

(pixils/defmode toolbar-button
  {:theme 'win95
   :init (fn [state ctx] {:pressed false})
   :render (fn [state ctx] ...)})
```

Themes may optionally define CSS-variable-like tokens and variants with `:vars`.
This is not required for a theme; themes with only `:styles` behave normally.
When variables are used, `:default-variant` names the fallback variant. Variant
names and token names are ordinary keywords or symbols:

```clojure
(pixils/deftheme win3
  {:default-variant :light
   :vars {:light {:panel-face {:r 0xc0 :g 0xc7 :b 0xc8}
                  :panel-text {:r 0 :g 0 :b 0}}
          :dark {:panel-face {:r 0x32 :g 0x35 :b 0x38}
                 :panel-text {:r 0xe8 :g 0xea :b 0xec}}}
   :styles {:ui/panel {:background (pixils/var :panel-face)
                       :text {:color (pixils/var :panel-text)}}}})
```

An application can select a variant without changing the major theme:

```clojure
(pixils/defprogram app
  {:theme 'pixils/windows-3
   :theme-variant :dark})
```

`:theme-variant` is also accepted where `:theme` is accepted on `defmode` and
child mode entries.

Missing token keys in the selected variant fall back to the theme's
`:default-variant`. Missing unresolved tokens are errors.

Theme selectors are map keys under `:styles`:

| Selector form | Meaning |
|---------------|---------|
| `'button` | Matches modes/components named `button`, including modes that `:extend 'button`. |
| `:menu/item` | Matches views whose `:class` contains `:menu/item`. |
| `'window:focus-within` | Matches a `window` view when it or any descendant contains focus. |
| `:menu/item:focus` | Matches a `:menu/item` classed view when it is the focused leaf. |
| `{:pressed true}` | Matches when the view state contains at least `{:pressed true}`. |
| `'(button {:pressed true})` | Compound selector: all parts must match the same view. |
| `['window:focus-within 'button]` | Descendant selector: the left selector must match an ancestor of the right selector. |

State selector maps use subset matching, so a selector such as `{:pressed true}` matches
any view state map that contains `:pressed true` alongside any other keys. Interaction
pseudo-state is expressed by suffixing component or class selectors with `:hover`,
`:focus`, or `:focus-within`.

Modes and child entries may declare `:class` as either a single keyword or a vector of
keywords:

```clojure
(pixils/defmode status-panel
  {:class :ui/panel
   ...})

{:mode 'main/button
 :class [:ui/toolbar-item :ui/primary]}
```

**Border**

The `:border` style key accepts a map with optional per-side overrides:

```clojure
; Uniform border on all sides
:border {:thickness 1 :line-style :solid :color {:r 0 :g 0 :b 0}}

; Bevel border using per-side colors
:border {:thickness 2
         :line-style :bevel
         :top {:color {:r 255 :g 255 :b 255}}
         :left {:color {:r 255 :g 255 :b 255}}
         :bottom {:color {:r 135 :g 136 :b 143}}
         :right {:color {:r 135 :g 136 :b 143}}}

; Per-side overrides - base values apply to all sides; side maps override individually
:border {:line-style :solid :color {:r 0 :g 0 :b 0}
         :bottom {:thickness 1}}
```

Base border keys: `:thickness` (number), `:line-style` (`:solid`, `:bevel`), `:color` (color map).
Directional override keys: `:top`, `:right`, `:bottom`, `:left` - each accepts the same fields.

**Hidden**

The `:hidden` style property is mutable. A component can show or hide itself during its
update hook without changing its parent's state. Prefer `pixils.ui/style!` for runtime
style changes; it accepts either the hook context or a view and merges the supplied style
fragment onto that live view instance:

```clojure
:update (fn [state ctx]
          (pixils.ui/style! ctx {:hidden (:should-hide? state)})
          state)
```

### Mouse events

Modes and components respond to mouse events via hook keys in the mode definition or any
child slot override map.

```clojure
(pixils/defcomponent button
  {:focusable true
   :on-click       (fn [state event ctx]
                     (assoc state :clicked true))
   :on-mouse-down  (fn [state event ctx]
                     (pixils.ui/stop-propagation! event)
                     (assoc state :pressing true))
   :on-mouse-up    (fn [state event ctx]
                     (assoc state :pressing false))
   :on-mouse-wheel (fn [state event ctx]
                     (update state :scroll-y + (:y event)))
   :on-mouse-enter (fn [state event ctx] ...)
   :on-mouse-leave (fn [state event ctx] ...)
   :render (fn [state ctx] ...)})
```

**Mouse hook reference**

| Hook              | When it fires                                                              |
|-------------------|----------------------------------------------------------------------------|
| `:on-mouse-down`  | Mouse button pressed within bounds                                         |
| `:on-mouse-up`    | Mouse button released over the component                                   |
| `:on-click`       | Button pressed and released within the same component                      |
| `:on-mouse-enter` | Cursor enters the component's bounds                                       |
| `:on-mouse-leave` | Cursor leaves the component's bounds                                       |
| `:on-mouse-wheel` | Mouse wheel scrolled over the component                                    |
| `:on-drag-start`  | Pressed button begins dragging after moving, routed to the press chain     |
| `:on-drag`        | Cursor moves while an active drag is in progress, routed to the press chain |
| `:on-drag-end`    | Active drag ends when the initiating button is released                    |
| `:on-drop`        | Active drag is released over the component, routed to the hover chain      |

Modes can opt into explicit drag policy with `:drag`:

```lisp
:drag {:button :left
       :start {:mode :threshold :distance 3}
       :payload (fn [state event ctx] {:kind :file :id (:id state)})}
```

`:start` supports `:motion`, `:immediate`, or `{:mode :threshold :distance n}`.
The payload is evaluated once when the drag operation starts, then delivered on
`:on-drag-start`, `:on-drag`, `:on-drag-end`, and `:on-drop`.

Wheel events are routed to the view under the cursor and then bubble through
that view's ancestors. If SDL reports multiple wheel ticks before the next
Pixils update, Pixils accumulates them into one `:on-mouse-wheel` event for the
frame. `(:x event)` is horizontal wheel movement, positive to the right.
`(:y event)` is vertical wheel movement, positive away from the user and
negative toward the user.

**Event object fields**

| Field        | Description                                                     |
|--------------|-----------------------------------------------------------------|
| `:button`            | Which button (`:left`, `:right`, `:middle`) - mouse-down, mouse-up, click, drag |
| `:position`          | Cursor position relative to the current component                            |
| `:global-position`   | Cursor position in buffer coordinates                                        |
| `:x`                 | Horizontal wheel delta - wheel hooks only                                   |
| `:y`                 | Vertical wheel delta - wheel hooks only                                     |
| `:start-position`    | Drag start position relative to the current component - drag hooks only      |
| `:start-global-position` | Drag start position in buffer coordinates - drag hooks only            |
| `:delta`             | Wheel delta as a point for wheel hooks; movement since the previous drag lifecycle event for drag hooks |
| `:total-delta`       | Movement since the initiating mouse-down - drag hooks only                   |
| `:payload`           | Source-defined drag data - drag and drop hooks only                         |

By default the event propagates from the innermost hit component outward through its
ancestors. Call `(pixils.ui/stop-propagation! event)` to prevent it reaching further
handlers.

Drag capture is source-based rather than hover-based: once a view chain begins a drag,
its drag hooks continue receiving events even after the cursor leaves its bounds.
Ordinary clicks remain unchanged unless that press chain has drag hooks and a drag
actually starts.

### Keyboard events

Focused views receive keyboard transitions first, and those events bubble up
through their ancestor chain to the root. If nothing is focused, the root mode
receives keyboard hooks directly.

```clojure
(pixils/defmode game-mode
  {:on-key-down (fn [state event ctx]
                  (if (= (:key event) :key/space)
                    (assoc state :firing true)
                    state))
   :on-key-held {:key/up (fn [state event ctx]
                           (assoc state :thrusting true))
                 [:key/left-ctrl :key/space] (fn [state event ctx]
                                               (assoc state :special true))}
   :on-key-up   (fn [state event ctx]
                  (if (= (:key event) :key/space)
                    (assoc state :firing false)
                    state))
   :update      (fn [state ctx] ...)})
```

Call `(pixils.ui/stop-propagation! event)` from a key hook to prevent the event
from reaching further ancestors.

For text-entry style behavior, `pixils.keyboard/event->text` converts a keyboard
event into the printable text it represents, or returns `nil` for non-text keys:

```clojure
(pixils/defmode text-input
  {:init (fn [state ctx] {:value ""})
   :on-key-down (fn [state event ctx]
                  (if-let [text (pixils.keyboard/event->text event)]
                    (assoc state :value (str (:value state) text))
                    state))})
```

`:on-key-held` accepts either:

- A function, which is called once per frame with the full held-key set in `(:held-keys event)`.
- A map, where a keyword key matches a single held key and a vector key matches a full combo.

For declarative map dispatch, Pixils chooses the most specific matching entry for the
current held-key set. A combo vector therefore wins over a matching single-key entry.

**Keyboard hook reference**

| Hook            | When it fires                             |
|-----------------|-------------------------------------------|
| `:on-key-down`  | A translated key is pressed this frame    |
| `:on-key-held`  | Keys are currently held this frame        |
| `:on-key-up`    | A translated key is released this frame   |

**Keyboard event fields**

| Field        | Description                                                           |
|--------------|-----------------------------------------------------------------------|
| `:key`       | The pressed or released key keyword for `:on-key-down` / `:on-key-up` |
| `:held-keys` | The full held-key set for `:on-key-held`                              |
| `:match`     | The matched keyword or combo vector for declarative `:on-key-held`    |

### Custom events

Components emit named events that bubble up the view tree. Ancestor modes handle them via
an `:on` map keyed by event keyword.

```clojure
; Emitting from a leaf component
(pixils/defcomponent cell
  {:on-click (fn [state event ctx]
               (pixils.ui/emit! (:view ctx) :board/cell-clicked {:x (:x state) :y (:y state)})
               state)})

; Handling in an ancestor
(pixils/defmode board
  {:on {:board/cell-clicked (fn [state event ctx]
                               (let [payload (:payload event)]
                                 (handle-click state (:x payload) (:y payload))))}})
```

`emit!` takes the view from which the event bubbles, an event key, and an optional payload.
The event key can be any keyword; qualified keywords (`:ns/name`) are recommended to avoid
collisions.

The `:on` handler signature is `(fn [state event ctx] ...)`. It returns the new state for
the mode that declared the handler. Custom events bubble through matching ancestors until
they are stopped with `(pixils.ui/stop-propagation! event)`.

**Custom event object fields**

| Field        | Description |
|--------------|-------------|
| `:event-key` | The emitted event keyword |
| `:source-mode` | The mode that originated the custom event, such as the emitting component's mode or a popped mode |
| `:payload`   | The optional payload passed to `emit!` |

Pushed modes are separate root trees, so `emit!` from inside a pushed mode does not bubble
into the underlying mode stack automatically. To return data to the underlying tree when a
pushed mode closes, use `pop-mode!` together with `push-mode!`'s `:origin` option described
above.

### State binding

When a parent passes state to a child, `pixils.ui/bind-state` creates a live link: the
child's bound key is updated from the parent each frame rather than fixed at push time.

```clojure
(pixils/defmode scoreboard
  {:children [{:mode 'main/score-display
               :state {:count (pixils.ui/bind-state :score)}}]})
```

Each frame, the child's `:count` is replaced with the parent's current `:score` value.
`bind-state` accepts a path of one or more keys:

```clojure
; Bind row 3 of the parent's :board vector
:state {:row (pixils.ui/bind-state :board 3)}

; Bind a nested value
:state {:value (pixils.ui/bind-state :settings :audio :volume)}
```

State binding is one-directional (parent -> child). Changes to a bound key inside the child
are overwritten by the parent's value each frame. To propagate changes back, use custom
events with `:on` and `pixils.ui/emit!`.

### Image resources

A mode declares the images it needs via `:resources`. They are loaded before the mode's
`:init` is called and remain cached for the lifetime of the session.

```clojure
(pixils/defmode sprite-mode
  {:resources {:images {:ship      "ship.png"
                        :explosion "explosion.png"}}

   :render (fn [state ctx]
             (pixils.render/image! :sprite-mode/ship
               {:pos {:x 100 :y 80} :scale 2.0}))})
```

Images are referenced as qualified keywords: `:mode-name/resource-id`.

**`pixils.image/trace-polygons`**

Traces closed contour polygons from an image's alpha channel. The result is a
vector of polygons, where each polygon is a vector of points in image-local
coordinates, or source-rect-local coordinates when `:source` is provided. The
polygons are implicitly closed; draw them with `pixils.render/polygon!` and
`:close true`.

```clojure
(def ship-outline
  (pixils.image/trace-polygons :sprites/ships
    {:source {:x 0 :y 0 :w 32 :h 32}
     :alpha-threshold 8
     :edge :outer
     :omit-straight-edges [:east]}))

(for [polygon ship-outline]
  (pixils.render/polygon! polygon
    {:offset {:x 100 :y 80}
     :scale 2
     :close true
     :color {:r 255 :g 240 :b 80}}))
```

`:edge` controls where returned coordinates sit relative to the alpha edge:
`:outer` offsets half a pixel to the transparent side, `:inner` offsets half a
pixel to the opaque side, and `:boundary` returns the exact pixel boundary.
`:outer` is the default.

`:omit-straight-edges` accepts a direction keyword or vector of direction
keywords, using `:north`, `:east`, `:south`, and `:west` or `:n`, `:e`, `:s`,
and `:w`. Omitted source-boundary edges may produce open outline paths; draw
those with `:close false`.

**`pixils.render/polygon!`**

Draws a polygon from a vector of points. By default, polygons are stroked as an
open polyline. Use `:close true` to close an outline, or `:fill true` to fill the
polygon. Filled polygons are implicitly closed.

```clojure
; Outline.
(pixils.render/polygon!
  [{:x 8 :y 4} {:x 28 :y 12} {:x 12 :y 24}]
  {:close true
   :stroke-width 4
   :line-join :round
   :color {:r 255 :g 240 :b 80}})

; Solid fill.
(pixils.render/polygon!
  [{:x 8 :y 4} {:x 28 :y 12} {:x 12 :y 24}]
  {:fill true
   :color {:r 80 :g 180 :b 255}})

; Canonical solid fill-style form.
(pixils.render/polygon!
  [{:x 8 :y 4} {:x 28 :y 12} {:x 12 :y 24}]
  {:fill true
   :fill-style {:type :solid
                :color {:r 80 :g 180 :b 255}}})

; Generate a regular polygon approximation of a circle, then draw it.
(pixils.render/polygon!
  (pixils.polygon/circle {:x 16 :y 16 :r 12}
                         {:segments 24})
  {:fill true
   :color {:r 80 :g 180 :b 255}})

; Erase alpha from the current render target using a filled polygon.
(pixils.render/polygon!
  reveal-polygon
  {:close true
   :fill true
   :color {:r 255 :g 255 :b 255 :a 255}
   :blend-mode :erase-alpha
   :rasterization :pixel})
```

`:fill-style` describes how a filled polygon is colored. The first non-solid
fill style is vertex color interpolation:

```clojure
(pixils.render/polygon!
  [{:x 0 :y 0}
   {:x 64 :y 0}
   {:x 64 :y 64}
   {:x 0 :y 64}]
  {:fill true
   :fill-style {:type :vertex-colors
                :colors [{:r 40 :g 90 :b 180}
                         {:r 80 :g 190 :b 120}
                         {:r 230 :g 220 :b 120}
                         {:r 120 :g 80 :b 180}]}})
```

For `:vertex-colors`, `:colors` must contain exactly one color per point, in the
same order as the polygon's vertices. Pixils triangulates the polygon and uses
barycentric interpolation inside each triangle. This supports triangles, quads,
and simple concave polygons. Quads currently use the triangulated behavior, not
bilinear interpolation.

`:line-join` controls how adjacent stroked polygon edges meet when
`:stroke-width` is greater than `1`:

| Join      | Description |
|-----------|-------------|
| `:miter`  | Pointy corner using the standard miter join. This is the default. Very sharp joins fall back to bevel. |
| `:round`  | Rounded corner using an arc fan around the vertex. |
| `:bevel`  | Clipped corner connecting the outer stroke edges directly. |
| `:none`   | Preserve the previous independent stroked-segment behavior. |

| Option          | Description |
|-----------------|-------------|
| `:close`        | Close an outlined polygon by drawing the final segment back to the first point. Default: `false`. |
| `:fill`         | Fill the polygon instead of drawing only its outline. Default: `false`. |
| `:color`        | Solid color for outlines, solid fills, or an explicit stroke drawn over a fill. |
| `:fill-style`   | Fill style map. Supported forms: `{:type :solid :color color}` and `{:type :vertex-colors :colors [color ...]}`. |
| `:blend-mode`   | Blend mode: `:blend`, `:none`, or `:erase-alpha`. Default: `:blend`. `:erase-alpha` reduces destination alpha by the source alpha and is intended for render-target masks. |
| `:line-join`    | Stroke join style: `:miter`, `:round`, `:bevel`, or `:none`. Default: `:miter`. |
| `:rasterization` | `:pixel` or `:smooth`. Default: `:pixel`. For polygons, smooth rasterization applies to outlines and explicit strokes. Filled polygons keep the existing fill renderer; vertex-color fills keep the triangulated geometry path. |
| `:stroke-width` | Stroke width in pixels. Outlines default to `1`; filled polygons default to no stroke unless this is supplied. |
| `:rotation`     | Rotation in radians around the origin before offset is applied. Default: `0`. |
| `:offset`       | Point offset applied after scale and rotation. Default: `{:x 0 :y 0}`. |
| `:scale`        | Scale multiplier applied before rotation and offset. Default: `1.0`. |

**Shape Rasterization**

`circle!`, `ellipse!`, and `polygon!` support two rasterization modes:

```clojure
; Existing integer pixel rasterization. This is the default.
(pixils.render/circle!
  {:x 16 :y 16 :r 4}
  {:fill true
   :color {:r 255 :g 80 :b 80}
   :rasterization :pixel})

; Signed-distance coverage rasterization for smoother small curves.
(pixils.render/ellipse!
  {:x 16 :y 16 :rx 8 :ry 4}
  {:color {:r 255 :g 255 :b 255}
   :rasterization :smooth})

; Coverage-sampled polygon outline.
(pixils.render/polygon!
  [{:x 8 :y 4} {:x 28 :y 12} {:x 12 :y 24}]
  {:close true
   :stroke-width 2
   :color {:r 80 :g 180 :b 255}
   :rasterization :smooth})
```

For circles and ellipses, `:rasterization` applies to both fills and outlines.
For polygons, it applies to outlines and explicit strokes over a fill. `:pixel`
keeps the hard-edged integer scanline/perimeter behavior. `:smooth` uses
coverage near the shape edge so small circles, ellipses, and polygon strokes
read less jagged. Polygon fills keep their existing fill renderers: solid fills
use scanlines, and `:fill-style {:type :vertex-colors ...}` uses triangulated
vertex-color rendering.

Filled circles, ellipses, and polygons also accept the canonical solid
fill-style form:

```clojure
(pixils.render/circle!
  {:x 16 :y 16 :r 4}
  {:fill true
   :fill-style {:type :solid
                :color {:r 80 :g 180 :b 255}}
   :rasterization :smooth})
```

| Option             | Description |
|--------------------|-------------|
| `:fill`            | Fill the circle or ellipse instead of drawing only its outline. Default: `false`. |
| `:color`           | Solid color for outlines or simple solid fills. |
| `:fill-style`      | Fill style map for filled circles/ellipses/polygons. Supported solid form: `{:type :solid :color color}`. Polygons also support `{:type :vertex-colors :colors [color ...]}`. |
| `:rasterization`   | `:pixel` or `:smooth`. Default: `:pixel`. |

**`pixils.render/image!`**

Draws an image resource. The first argument is a qualified image keyword. The
second argument can be a point, a rect, or an options map.

```clojure
; Draw at natural image size.
(pixils.render/image! :sprite-mode/ship {:pos {:x 100 :y 80}})

; Equivalent target spelling for the common point case.
(pixils.render/image! :sprite-mode/ship {:target {:x 100 :y 80}})

; Scale one image copy into a target rect.
(pixils.render/image! :sprite-mode/ship
  {:target {:x 100 :y 80 :w 64 :h 32}})

; Draw a sprite from a sheet.
(pixils.render/image! :sprites/tiles
  {:target {:x 10 :y 10}
   :source {:x 16 :y 0 :w 8 :h 8}
   :scale 2})

; Repeat from the target anchor to fill the clip rect.
(pixils.render/image! :terrain/water
  {:target {:x 14 :y 10}
   :clip-rect {:x 10 :y 10 :w 320 :h 200}
   :repeat-x? true
   :repeat-y? true})
```

Point targets draw one image copy at natural size, or at the source-crop size
when `:source` is provided. `:scale` applies to point targets. Rect targets scale
one image copy into that rect. If a direct map argument includes `:w` or `:h`, it
is treated as a rect target; otherwise direct `{:x N :y N}` maps are treated as
point targets. When an options map is used, placement must be under `:pos` or
`:target`.

`r/image!` repeats from the target's top-left anchor. If `:clip-rect` is
provided, Pixils clips drawing to that rect and uses it as the repeat fill
bounds. If `:clip-rect` is omitted but a renderer clip is already active, repeat
fills the active clip. Without repeat, `:clip-rect` only clips the single
rendered copy.

| Option       | Description |
|--------------|-------------|
| `:pos`       | Point placement alias for the common natural-size target case. |
| `:target`    | Point or rect target for one image copy. Point targets use image/source size; rect targets scale one copy. |
| `:clip-rect` | Rect that clips drawing and bounds repeated drawing. |
| `:source`    | Optional source crop rect in image pixels. |
| `:scale`     | Scale multiplier for point targets. Default: `1.0`. |
| `:repeat-x?` | Repeat copies horizontally. Default: `false`. |
| `:repeat-y?` | Repeat copies vertically. Default: `false`. |
| `:opacity`   | Alpha multiplier from `0.0` to `1.0`. Default: `1.0`. |
| `:rotation`  | Rotation in radians. Default: `0`. |
| `:flip-x?`   | Flip each copy horizontally. Default: `false`. |
| `:flip-y?`   | Flip each copy vertically. Default: `false`. |

### Sound resources

A mode can also declare sounds and music under `:resources`. Sounds are short effects
loaded for immediate playback. Music is long-running audio, such as MP3 tracks, loaded
without predecoding and played through Pixils' single managed music track.

```clojure
(pixils/defmode game-mode
  {:resources {:sounds {:laser "assets/laser.wav"
                        :boom  "assets/explosion.wav"}
               :music  {:theme "assets/music/theme.mp3"}}})
```

Audio resources are referenced as qualified keywords in the same way as images:
`:mode-name/resource-id`.

**`pixils.audio/play!`**

Plays a sound effect loaded via a bundle or mode resource declaration.

```clojure
(pixils.audio/play! :game-mode/laser)
(pixils.audio/play! :game-mode/laser {:volume 0.35})
(pixils.audio/play! :game-mode/boom {:loops 1})
(pixils.audio/play! :game-mode/ambience {:loops :forever})
```

| Option    | Description |
|-----------|-------------|
| `:channel`| SDL_mixer channel to use. Default: `-1` (first free channel). |
| `:loops`  | Number of extra repeats after the first play, or `:forever` for infinite looping. Numeric `-1` is also accepted for infinite looping. Default: `0`. |
| `:volume` | Playback volume from `0.0` to `1.0`. Default: `1.0`. |

**`pixils.audio/play-music!`**

Starts music playback from a music resource. Pixils keeps one managed music track;
starting another music resource replaces the current one. SDL_mixer-supported formats,
including MP3 when decoder support is available, can be used.

```clojure
(pixils.audio/play-music! :game-mode/theme)
(pixils.audio/play-music! :game-mode/theme
  {:loops :forever
   :volume 0.8
   :fade-in-ms 1500})
(pixils.audio/play-music! :game-mode/battle
  {:crossfade-ms 1200
   :loops :forever
   :volume 0.8})
```

| Option        | Description |
|---------------|-------------|
| `:loops`      | Number of extra repeats after the first play, or `:forever` for infinite looping. Numeric `-1` is also accepted for infinite looping. Default: `:forever`. |
| `:volume`     | Playback volume from `0.0` to `1.0`. Default: `1.0`. |
| `:fade-in-ms` | Fade-in duration in milliseconds. Default: `0`. |
| `:crossfade-ms` | Crossfade duration in milliseconds. If music is already playing, Pixils fades it out while fading the new music in. If no music is playing, this behaves as a fade-in. Default: `0`. |

**Music controls**

```clojure
(pixils.audio/set-music-volume! 0.45)
(pixils.audio/pause-music!)
(pixils.audio/resume-music!)
(pixils.audio/music-playing?)
(pixils.audio/stop-music! {:fade-out-ms 800})
```

`set-music-volume!`, `pause-music!`, `resume-music!`, and `stop-music!` return
`true` when the operation succeeds. `music-playing?` returns whether the managed
music track is currently playing. `stop-music!` also accepts no options for an
immediate stop.

### Text and fonts

Pixils supports bitmap fonts. A font is declared with `deffont` and references an image
loaded via a bundle.

**`defbundle`**

`defbundle` declares a named set of resources independently of any mode's lifecycle. Bundles
are the primary mechanism for loading font sheets and other shared assets.

```clojure
(pixils/defbundle ui-assets
  {:images {:font-sheet "assets/font.png"
            :icons      "assets/icons.png"}
   :sounds {:click      "assets/click.wav"}
   :music  {:theme      "assets/music/theme.mp3"}})
```

Bundle resources are referenced as `:bundle-name/resource-id`.

**`deffont`**

`deffont` declares a bitmap font. It references an image loaded by a `defbundle` or a mode's
`:resources`, and maps characters to their source rects in that image.

```clojure
(pixils/deffont my-font
  {:type     :bitmap
   :resource :ui-assets/font-sheet
   :spacing  1
   :glyphs   {'A {:x 0   :y 0 :w 8 :h 10}
              'B {:x 9   :y 0 :w 8 :h 10}
              ' ' {:x 200 :y 0 :w 4 :h 10}}})
```

| Key        | Description |
|------------|-------------|
| `:type`    | Font type. Currently only `:bitmap` is supported. |
| `:resource`| Qualified keyword `:bundle-name/image-id` pointing to the glyph sheet. |
| `:spacing` | Extra pixels between characters. Default: 1. |
| `:glyphs`  | Map from character to `{:x N :y N :w N :h N}` source rect in the sheet. |

The declared font is accessible as `:font/my-font`. The `font/` namespace prefix is added
automatically if the name does not already contain `/`.

**`pixils.render/text!`**

Renders a string at a given position and returns the rendered bounds as `{:x N :y N :w N :h N}`.

```clojure
(pixils.render/text! "Score: 999" {:x 10 :y 10}
                     {:font   :font/my-font
                      :color  {:r 255 :g 255 :b 255}
                      :scale  1
                      :shadow {:offset {:x 1 :y 1}
                               :color  {:r 0 :g 0 :b 0}}})
```

| Option    | Description |
|-----------|-------------|
| `:font`   | Font keyword. Defaults to the built-in console font. |
| `:color`  | Text color. Defaults to white. |
| `:scale`  | Pixel scale multiplier. Accepts fractional values, for example `0.75` or `1.5`. Default: 1. |
| `:shadow` | Shadow spec `{:offset {:x N :y N} :color {...}}` or a vector of specs for multiple shadows. |

**`pixils.render/text-size`**

Returns the `{:w N :h N}` dimensions that `text!` would occupy, without drawing anything.
Accepts the same `:font` and `:scale` options as `text!`.

```clojure
(let [size (pixils.render/text-size "Hello" {:font :font/my-font})]
  (:w size))
```

## Namespace reference

### `pixils`

| Symbol        | Description |
|---------------|-------------|
| `defprogram`  | Declare the application entry point |
| `defmode`     | Declare a mode |
| `defcomponent`| Declare a reusable component (alias for `defmode`) |
| `defbundle`   | Declare a global named resource bundle |
| `deffont`     | Declare a bitmap font |
| `push-mode!`  | Push a mode onto the stack. Args: `mode-sym`, optional `state`, optional override map. The override map also accepts optional `:origin` for pop-result routing. |
| `pop-mode!`   | Pop the top mode from the stack. Optional arg: payload returned as a `:pop/result`-style custom event. |

### `pixils.audio`

| Symbol  | Description |
|---------|-------------|
| `play!` | Play a sound resource. Args: qualified keyword `:bundle/id`, optional options map `{:channel N :loops N/:forever :volume N}`. Returns the SDL_mixer channel index, or `-1` if playback fails. |
| `play-music!` | Play a music resource on Pixils' managed music track. Args: qualified keyword `:bundle/id`, optional options map `{:loops N/:forever :volume N :fade-in-ms N :crossfade-ms N}`. Returns `true` on success. |
| `stop-music!` | Stop the managed music track. Optional options map: `{:fade-out-ms N}`. Returns `true` on success. |
| `pause-music!` | Pause the managed music track. Returns `true` on success. |
| `resume-music!` | Resume the managed music track. Returns `true` on success. |
| `set-music-volume!` | Set current music volume from `0.0` to `1.0`. Returns `true` on success. |
| `music-playing?` | Return whether the managed music track is currently playing. |

### `pixils.ui`

| Symbol               | Description |
|----------------------|-------------|
| `bind-state`         | Create a live binding from a child state key to a path in the parent's state. Args: one or more keys forming the path. |
| `blur!`              | Clear focus, or clear it only when the optional hook `ctx`/`view` argument is the focused leaf. |
| `emit!`              | Emit a custom event that bubbles up the view tree. Args: `view`, `event-key`, optional `payload`. |
| `focus!`             | Focus a view. Arg: hook `ctx` or a `view` object. |
| `stop-propagation!`  | Prevent a mouse or custom event from bubbling further. Pass the event object from a mouse hook or `:on` handler. |

### `pixils.image`

| Symbol   | Description |
|----------|-------------|
| `size`   | Return the size of an image resource as a dimension object with `:w` and `:h`. Args: qualified keyword `:bundle/id`. |
| `width`  | Return the width of an image resource. Args: qualified keyword `:bundle/id`. |
| `height` | Return the height of an image resource. Args: qualified keyword `:bundle/id`. |
| `rect`   | Return a rect for an image resource. Args: qualified keyword `:bundle/id`, optional point offset used as the rect's `:x` and `:y`. |
| `trace-polygons` | Trace contour polygons from image alpha. Args: qualified keyword `:bundle/id`, optional opts `{:source rect :alpha-threshold N :edge :outer/:inner/:boundary :omit-straight-edges [:north/:east/:south/:west]}`. Returns a vector of point vectors in image/source-local coordinates. Omitted source-boundary edges can produce open paths. |

### `pixils.render`

| Symbol      | Description |
|-------------|-------------|
| `use-color!`| Set the current draw color. Accepts a color object or four RGBA numbers (r g b a). |
| `line!`     | Draw a line between two points. Optional third arg: color or options map `{:color ... :stroke-width N}`. |
| `rect!`     | Draw a rectangle. Args: `{:x :y :w :h}` rect (or two corner points), options map `{:color ... :fill true/false}`. |
| `circle!`   | Draw a circle. Args: `{:x :y :r}` center/radius map, options include `:color`, `:fill`, `:fill-style`, and `:rasterization`. |
| `ellipse!`  | Draw an ellipse. Args: `{:x :y :rx :ry}` center/radius map, options include `:color`, `:fill`, `:fill-style`, and `:rasterization`. |
| `polygon!`  | Draw a polygon from a vector of points. Options include `:close`, `:fill`, `:stroke-width`, `:line-join`, `:rasterization`, `:rotation`, `:offset`, `:color`, `:scale`, `:blend-mode`, and `:fill-style` for solid or vertex-colored fills. |
| `image!`    | Draw an image. Args: qualified keyword `:bundle/id`, then point, rect, or options map. Options include `:pos`, `:target`, `:clip-rect`, `:source`, `:scale`, `:repeat-x?`, `:repeat-y?`, `:opacity`, `:rotation`, `:flip-x?`, and `:flip-y?`. |
| `text!`     | Render a string. Args: string, position point, options map. Returns rendered bounds `{:x :y :w :h}`. |
| `text-size` | Measure text without rendering. Args: string, optional options map. Returns `{:w :h}`. |

### `pixils.color`

| Symbol       | Description |
|--------------|-------------|
| `make-color` | Construct a color from `{:r :g :b}` or `{:r :g :b :a}` (0-255). Alpha defaults to 255. |
| `with-alpha` | Return a copy of a color with a new alpha value. |

### `pixils.point`

| Symbol     | Description |
|------------|-------------|
| `point`    | Construct a point from two numbers or a `{:x :y}` map. |
| `+`        | Add two points. |
| `-`        | Subtract two points. |
| `*`        | Multiply a point by a scalar. |
| `div`      | Divide a point by a scalar. |
| `clamp` | Clamp a point to a rect's bounds. |
| `translate` | Translate a point by `dx dy`. |
| `translate-x` | Translate a point by `dx` along the x axis. |
| `translate-y` | Translate a point by `dy` along the y axis. |
| `wrap` | Wrap a point around a rect's bounds. |
| `rotate`   | Rotate a point. Args: `point angle` or `point origin angle`. |
| `distance` | Distance between two points. |
| `distance-squared` | Squared Euclidean distance between two points. Useful for distance ordering without a square root. |

### `pixils.rect`

| Symbol        | Description |
|---------------|-------------|
| `make-rect`   | Construct a rect from `{:x :y :w :h}`. |
| `contains?`   | Return whether a rect fully contains a point, rect, or polygon. Rect right/bottom edges are exclusive. |
| `intersects?` | Return whether a rect intersects another rect or a polygon. For rect-vs-rect, touching edges without overlap do not intersect unless an options map `{:include-boundary? true}` is passed as the third argument. |
| `inside?`     | Compatibility alias for point-in-rect checks. |
| `intersect?`  | Compatibility alias for rect-rect intersection checks. |

### `pixils.line`

| Symbol          | Description |
|-----------------|-------------|
| `closest-point` | Return the closest point on a finite line to a point. Args: line start point, line end point, query point. The result is clamped to the line endpoints. |

### `pixils.polygon`

| Symbol        | Description |
|---------------|-------------|
| `area`        | Return the absolute shoelace area of a polygon. Returns `0` for fewer than three vertices. |
| `bounds`      | Return the integer bounding rect for a polygon, or `nil` for an empty point vector. |
| `circle`      | Generate a polygon approximating a circle. Args: `{:x :y :r}`, optional opts `{:segments N :rotation radians}`. Default segments: `32`. Returns `[]` for non-positive radius. |
| `closest-edge-point` | Return the closest point on any polygon edge to a query point, or `nil` for fewer than two vertices. |
| `contains?`   | Return whether a polygon fully contains a point, rect, or another polygon. Polygon edges count as contained. |
| `ellipse`     | Generate a polygon approximating an ellipse. Args: `{:x :y :rx :ry}`, optional opts `{:segments N :rotation radians}`. Default segments: `32`. Returns `[]` for non-positive radii. |
| `intersection` | Return the polygon regions shared by two polygon inputs. Args may be one polygon or a vector of polygons. Returns a vector of polygons. Optional opts: `{:precision N}` where `N` is Clipper decimal precision from `-8` to `8`; default `4`. |
| `intersects?` | Return whether a polygon intersects a rect or another polygon. Edge/vertex touches count as intersections. |
| `simplify`    | Remove redundant polygon vertices. Accepts one polygon or a vector of polygons, with optional opts `{:threshold N}` in coordinate units. Default threshold: `0.001`. |
| `union`       | Return the geometric union of polygon input. Accepts `(union polygons)` or `(union polygons-a polygons-b)`, where each input may be one polygon or a vector of polygons. Returns disconnected islands as separate polygons. Optional opts: `{:precision N}`. |
| `vertex-center` | Return the average of all polygon vertices as a point, or `nil` for an empty point vector. |

```clojure
; Clip one polygon to another.
(pixils.polygon/intersection reveal-polygon cell-polygon)
;; => [clipped-polygon ...]

; Merge stored coverage with newly revealed coverage.
(pixils.polygon/union existing-polygons new-polygons)
;; => [merged-polygon ...]

; Remove redundant vertices introduced by repeated boolean operations.
(map (fn [polygon]
       (pixils.polygon/simplify polygon {:threshold 0.01}))
     (pixils.polygon/union existing-polygons new-polygons))
```

### `pixils.resource`

| Symbol                      | Description |
|-----------------------------|-------------|
| `create-bundle!`            | Create or update a dynamic bundle at runtime. Args: bundle keyword, optional resource dependency map. |
| `add-image!`                | Add or replace a file-backed image in a dynamic bundle. Args: qualified image keyword and file dependency. |
| `create-image!`             | Create or replace a generated image in a dynamic bundle. Args: qualified image keyword, `{:size {:w :h} :clear color}`, and a zero-arg drawing function. Drawing commands inside the function target the new image. |
| `remove-image!`             | Remove a file-backed or generated image from a dynamic bundle. Args: qualified image keyword. |
| `list-images`               | List image resources in a bundle. Generated images include `:source :generated` and `:size`. |
| `make-resource-dependencies`| Declare resource dependencies explicitly. Takes `{:images {:id "file.png"} :sounds {:id "file.wav"} :music {:id "file.mp3"}}`. The plain map form in `:resources` is equivalent and preferred. |

## Using Pixils as a library

The `pixils` binary covers the common case, but Pixils is also a linkable library. This
lets you write arbitrary portions of your application in C++, and extend Roo with your
own native namespaces and host types - exposing domain-specific data structures and
functions directly to your scripts.

The minimal host application looks like this:

```cpp
#include <pixils/client.h>
#include <pixils/context.h>
#include <pixils/init_sdl.h>
#include <pixils/script.h>

int main()
{
  auto opt_ctx = Pixils::init_sdl("My App");
  if (!opt_ctx) { SDL_Quit(); return 1; }

  Pixils::RenderContext ctx = *opt_ctx;

  Roo::Runtime runtime = Pixils::init_roo_runtime(
    ctx, "main", {"main.roo"});

  Pixils::Client client(runtime, ctx);
  client.run();

  SDL_Quit();
  return 0;
}
```

To register additional namespaces, pass an initializer function to `init_roo_runtime`:

```cpp
Roo::Runtime runtime = Pixils::init_roo_runtime(
  ctx, "main",
  [](Pixils::RuntimeConfiguration* cfg) {
    cfg->native_namespaces.push_back(std::make_unique<MyGameNamespace>());
  },
  {"main.roo"});
```

Link against `pixils::pixils_static` or `pixils::pixils_shared` and find the package
with `find_package(pixils REQUIRED)`.

## Building from source

### Prerequisites

- CMake 3.20+
- SDL3, SDL3_image, SDL3_mixer
- The `roo` CMake package installed (see the
  [Roo repository](https://github.com/svjson/roo-lang))

### Build commands

```bash
# Configure (Release, installs to $HOME/.local)
make configure

# Debug build
make configure BUILD_TYPE=Debug

# Custom install prefix
make configure PREFIX=/usr/local

# Build
make build

# Build and install
make install
```
