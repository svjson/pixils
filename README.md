# Pixils!

Pixils! is a runtime for building 2D graphical applications in
[Lisple](https://github.com/svjson/lisple-lang) - a Lisp with a modern, practical flavor.
You write your application entirely in Lisple and run it with a single command. Pixils takes
care of the rendering loop, window management, input, and asset loading - powered by SDL2
underneath, invisible to your scripts.

The result is a tight feedback loop for building anything from games to tools to editors:
define your modes, write your logic, run it. No compilation step for your application code,
no C++ required.

Pixils is general-purpose. A tile-based game, a C64 asset editor, and a Minesweeper clone are
equally valid Pixils applications.

## The `pixils` binary

Run an application from a Lisple script with a single command:

```bash
pixils path/to/main.lisple
pixils path/to/app/          # loads main.lisple from that directory
```

## Writing an application

A Pixils application is a single `.lisple` file (or a collection of files loaded from
`main.lisple`). It declares a program, one or more modes, and wires them together.

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
   :render  (fn [state ctx] nil)})
```

All three hooks are optional. An absent hook is a no-op; an absent `:init` or `:update`
leaves the state unchanged.

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
| `:mouse-held`         | Set of mouse buttons currently held                                                      |
| `:view`               | The current view (see below)                                                             |

**`ctx :view`**

The `:view` key gives access to the current component's live view instance.

| Key            | Description                                                                    |
|----------------|--------------------------------------------------------------------------------|
| `:id`          | The view's unique identifier string                                            |
| `:bounds`      | The view's bounding rect in the buffer: `{:x N :y N :w N :h N}`               |
| `:interaction` | Interaction state: map with `:hovered` and `:pressed` booleans                |
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
```

The optional third argument to `push-mode!` is an override map. It accepts any key that
`defmode` accepts (hooks, `:style`, `:children`) and merges them onto the base mode for
that specific push only. The registered mode definition is not modified.

Mode transitions are message-based and take effect between frames, so it is safe to call
`push-mode!` or `pop-mode!` from inside any hook.

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
`:style {:width N}` in a row layout) is given exactly that many pixels. A child without a
size constraint fills the remaining space. Multiple fill children share the remainder evenly.
Margins are also declared in `:style`. A child's `:margin` consumes flow space around the
child and offsets its rendered bounds within the allocated slot.

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
the size allocated to other children.

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
| `:background` | `{:r N :g N :b N}` or `{:r N :g N :b N :a N}`                       | Background fill color (0-255). |
| `:margin`     | Number, `[vertical horizontal]`, `[top right bottom left]`, or `{:t N :r N :b N :l N}` | Outer space around a child in layout flow. |
| `:padding`    | Number, `[vertical horizontal]`, `[top right bottom left]`, or `{:t N :r N :b N :l N}` | Inset applied before the render hook's viewport is set. |
| `:border`     | Border map (see below)                                               | Draws a border inside the component bounds. |
| `:layout`     | `{:direction :row}`, `{:direction :column}`, optional `:gap :none`, `:gap N`, `:gap :space-between`, or wrapped gap maps | Child layout policy. Currently supports flow direction, fixed gap, explicit no-gap, and `space-between` distribution. |
| `:text`       | `{:color {:r N :g N :b N}}`, optional `:font :font/name`, optional `:scale N` | Text presentation properties for components that render text. |
| `:width`      | Number                                                               | Content width in pixels. Absent means fill remaining space. |
| `:height`     | Number                                                               | Content height in pixels. Absent means fill remaining space. |
| `:position`   | `:absolute`, `:flow`                                                 | Positioning mode. Default: `:flow`. |
| `:top`        | Number                                                               | Top offset when `:position :absolute`. |
| `:left`       | Number                                                               | Left offset when `:position :absolute`. |
| `:hidden`     | Boolean                                                              | When true, excluded from hit-testing and rendering. Layout space is preserved. |
| `:hover`      | Nested style map                                                     | Applied instead of the base style when the cursor is within bounds. |

The hover variant is injected automatically by the framework. No manual hit-testing is
needed in the component.

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
update hook without changing its parent's state:

```clojure
:update (fn [state ctx]
          (assoc-in! ctx [:view :style :hidden] (:should-hide? state))
          state)
```

### Mouse events

Modes and components respond to mouse events via hook keys in the mode definition or any
child slot override map.

```clojure
(pixils/defcomponent button
  {:on-click       (fn [state event ctx]
                     (assoc state :clicked true))
   :on-mouse-down  (fn [state event ctx]
                     (pixils.ui/stop-propagation! event)
                     (assoc state :pressing true))
   :on-mouse-up    (fn [state event ctx]
                     (assoc state :pressing false))
   :on-mouse-enter (fn [state event ctx] ...)
   :on-mouse-leave (fn [state event ctx] ...)
   :render (fn [state ctx] ...)})
```

**Mouse hook reference**

| Hook              | When it fires                                                              |
|-------------------|----------------------------------------------------------------------------|
| `:on-mouse-down`  | Mouse button pressed within bounds                                         |
| `:on-mouse-up`    | Mouse button released (fires on the component that received the down)      |
| `:on-click`       | Button pressed and released within the same component                      |
| `:on-mouse-enter` | Cursor enters the component's bounds                                       |
| `:on-mouse-leave` | Cursor leaves the component's bounds                                       |

**Event object fields**

| Field        | Description                                                     |
|--------------|-----------------------------------------------------------------|
| `:button`    | Which button (`:left`, `:right`, `:middle`) - mouse-down, mouse-up, on-click only |
| `:local-pos` | Cursor position relative to the component's content rect        |
| `:global-pos`| Cursor position in buffer coordinates                           |

By default the event propagates from the innermost hit component outward through its
ancestors. Call `(pixils.ui/stop-propagation! event)` to prevent it reaching further
handlers.

### Keyboard events

Root modes can respond directly to keyboard transitions with explicit key hooks.

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

These hooks currently fire on the root mode only. Pixils does not yet have focused
components, so child views do not receive keyboard hooks directly.

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
  {:on {:board/cell-clicked (fn [state payload ctx]
                               (handle-click state (:x payload) (:y payload)))}})
```

`emit!` takes the view from which the event bubbles, an event key, and an optional payload.
The event key can be any keyword; qualified keywords (`:ns/name`) are recommended to avoid
collisions.

The `:on` handler signature is `(fn [state payload ctx] ...)`. It returns the new state for
the mode that declared the handler. Handlers do not participate in propagation - only the
nearest ancestor with a matching `:on` key receives the event.

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

### Sound resources

A mode can also declare sounds under `:resources`.

```clojure
(pixils/defmode game-mode
  {:resources {:sounds {:laser "assets/laser.wav"
                        :boom  "assets/explosion.wav"}}})
```

Sounds are referenced as qualified keywords in the same way as images: `:mode-name/resource-id`.

**`pixils.audio/play!`**

Plays a sound effect loaded via a bundle or mode resource declaration.

```clojure
(pixils.audio/play! :game-mode/laser)
(pixils.audio/play! :game-mode/laser {:volume 0.35})
(pixils.audio/play! :game-mode/boom {:loops 1})
```

| Option    | Description |
|-----------|-------------|
| `:channel`| SDL_mixer channel to use. Default: `-1` (first free channel). |
| `:loops`  | Number of extra repeats after the first play. Default: `0`. |
| `:volume` | Playback volume from `0.0` to `1.0`. Default: `1.0`. |

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
   :sounds {:click      "assets/click.wav"}})
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
| `:scale`  | Integer pixel scale multiplier. Default: 1. |
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
| `defbundle`   | Declare a global named image bundle |
| `deffont`     | Declare a bitmap font |
| `push-mode!`  | Push a mode onto the stack. Args: `mode-sym`, optional `state`, optional `overrides-map` |
| `pop-mode!`   | Pop the top mode from the stack |

### `pixils.audio`

| Symbol  | Description |
|---------|-------------|
| `play!` | Play a sound resource. Args: qualified keyword `:bundle/id`, optional options map `{:channel N :loops N :volume N}`. Returns the SDL_mixer channel index, or `-1` if playback fails. |

### `pixils.ui`

| Symbol               | Description |
|----------------------|-------------|
| `bind-state`         | Create a live binding from a child state key to a path in the parent's state. Args: one or more keys forming the path. |
| `emit!`              | Emit a custom event that bubbles up the view tree. Args: `view`, `event-key`, optional `payload`. |
| `stop-propagation!`  | Prevent a mouse event from bubbling further up the component tree. Pass the event object from a mouse hook. |

### `pixils.image`

| Symbol   | Description |
|----------|-------------|
| `size`   | Return the size of an image resource as a dimension object with `:w` and `:h`. Args: qualified keyword `:bundle/id`. |
| `width`  | Return the width of an image resource. Args: qualified keyword `:bundle/id`. |
| `height` | Return the height of an image resource. Args: qualified keyword `:bundle/id`. |
| `rect`   | Return a rect for an image resource. Args: qualified keyword `:bundle/id`, optional point offset used as the rect's `:x` and `:y`. |

### `pixils.render`

| Symbol      | Description |
|-------------|-------------|
| `use-color!`| Set the current draw color. Accepts a color object or four RGBA numbers (r g b a). |
| `line!`     | Draw a line between two points. Optional third arg: color. |
| `rect!`     | Draw a rectangle. Args: `{:x :y :w :h}` rect (or two corner points), options map `{:color ... :fill true/false}`. |
| `polygon!`  | Draw a polygon from a vector of points. Options: `:close`, `:rotation`, `:offset`, `:color`, `:scale`. |
| `image!`    | Draw an image. Args: qualified keyword `:bundle/id`, options map with `:pos` (required), `:scale`, `:alpha`, and `:rotation` (radians). |
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

### `pixils.resource`

| Symbol                      | Description |
|-----------------------------|-------------|
| `make-resource-dependencies`| Declare resource dependencies explicitly. Takes `{:images {:id "file.png"} :sounds {:id "file.wav"}}`. The plain map form in `:resources` is equivalent and preferred. |

## Using Pixils as a library

The `pixils` binary covers the common case, but Pixils is also a linkable library. This
lets you write arbitrary portions of your application in C++, and extend Lisple with your
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

  Lisple::Runtime runtime = Pixils::init_lisple_runtime(
    ctx, "main", {"main.lisple"});

  Pixils::Client client(runtime, ctx);
  client.run();

  SDL_Quit();
  return 0;
}
```

To register additional namespaces, pass an initializer function to `init_lisple_runtime`:

```cpp
Lisple::Runtime runtime = Pixils::init_lisple_runtime(
  ctx, "main",
  [](Pixils::RuntimeConfiguration* cfg) {
    cfg->native_namespaces.push_back(std::make_unique<MyGameNamespace>());
  },
  {"main.lisple"});
```

Link against `pixils::pixils_static` or `pixils::pixils_shared` and find the package
with `find_package(pixils REQUIRED)`.

## Building from source

### Prerequisites

- CMake 3.20+
- SDL2, SDL2_image, SDL2_mixer
- The `lisple` CMake package installed (see the
  [Lisple repository](https://github.com/svjson/lisple-lang))

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
