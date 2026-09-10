
#ifndef PIXILS__PIXILS_NAMESPACE
#define PIXILS__PIXILS_NAMESPACE

#include <pixils/binding/shkey.h>
#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/program.h>
#include <pixils/ui/theme.h>

#include <memory>
#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/namespace.h>
#include <roo/type.h>

namespace Pixils::UI
{
  struct InteractionState;
} // namespace Pixils::UI

namespace Pixils::Runtime
{
  struct Component;
  struct Mode;
  struct ModeComposition;
  struct View;
} // namespace Pixils::Runtime

namespace Pixils::Script
{
  namespace HostType
  {
    /**
     * Type ref for APIs that accept an already-created host value, but must not
     * allow Roo's normal host-type coercion path. This keeps signatures such as
     * `push-mode!` precise: an argument can be a real Mode host object, but a
     * map or Component host object that could otherwise be coerced toward Mode
     * is rejected by the signature before native execution.
     */
    class ExactHostTypeRef : public Roo::TypeRef
    {
      const Roo::HostTypeRef& host_type;

     public:
      ExactHostTypeRef(const Roo::HostTypeRef& host_type, const std::string& name);

      bool is_type_of(const Roo::Value& val) const override;
      bool is_type_of(const Roo::AST::ASTNode& obj) const override;
    };
  } // namespace HostType

  /*!
   * @brief Constant for "pixils"
   */
  inline const std::string NS_PIXILS = "pixils";

  inline const std::string FN__PIXILS__MAKE_DISPLAY = "pixils/make-display";
  inline const std::string FN__PIXILS__MAKE_MODE = "pixils/make-mode";
  inline const std::string FN__PIXILS__MAKE_COMPONENT = "pixils/make-component";
  inline const std::string FN__PIXILS__MAKE_MODE_COMPOSITION =
    "pixils/make-mode-composition";
  inline const std::string FN__PIXILS__MAKE_DIMENSION = "pixils/make-dimension";
  inline const std::string FN__MAKE_DISPLAY = "display";
  inline const std::string FN__MAKE_RESOLUTION = "pixils/make-resolution";
  inline const std::string FN__POP_MODE_BANG = "pop-mode!";
  inline const std::string FN__POP_TO_BANG = "pop-to!";
  inline const std::string FN__PUSH_MODE_BANG = "push-mode!";
  inline const std::string FN__QUIT_BANG = "quit!";
  inline const std::string FN__SET_THEME_BANG = "set-theme!";
  inline const std::string FN__WARP_MOUSE_BANG = "warp-mouse!";

  inline const std::string ID__PIXILS__MODE_STACK = "pixils/mode-stack";
  inline const std::string ID__PIXILS__MODE_STACK_MESSAGES = "pixils/mode-stack-messages";
  inline const std::string ID__PIXILS__COMPONENTS = "pixils/components";
  inline const std::string ID__PIXILS__MODES = "pixils/modes";
  inline const std::string ID__PIXILS__PROGRAMS = "pixils/programs";
  inline const std::string ID__PIXILS__RENDER_CONTEXT = "pixils/render-context";
  inline const std::string ID__PIXILS__THEMES = "pixils/themes";

  namespace MapKey
  {
    DECL_SHKEY(BUFFER_SIZE)
    DECL_SHKEY(H)
    DECL_SHKEY(HELD_KEYS)
    DECL_SHKEY(INIT)
    DECL_SHKEY(KEY_DOWN)
    DECL_SHKEY(PIXEL_SIZE)
    DECL_SHKEY(QUIT)
    DECL_SHKEY(RENDER)
    DECL_SHKEY(UPDATE)
    DECL_SHKEY(W)
  } // namespace MapKey

  namespace HostType
  {
    HOST_TYPE(DIMENSION, "HDimension", FN__PIXILS__MAKE_DIMENSION)
    HOST_TYPE(DISPLAY, "HDisplay", FN__PIXILS__MAKE_DISPLAY)
    HOST_TYPE(FRAME_EVENTS, "HFrameEvents")
    HOST_TYPE(HOOK_CONTEXT, "HHookContext")
    HOST_TYPE(MODE, "HMode", FN__PIXILS__MAKE_MODE)
    HOST_TYPE(COMPONENT, "HComponent", FN__PIXILS__MAKE_COMPONENT)
    HOST_TYPE(MODE_COMPOSITION, "HModeComposition", FN__PIXILS__MAKE_MODE_COMPOSITION)
    HOST_TYPE(PROGRAM, "HProgram")
    HOST_TYPE(INTERACTION_STATE, "HInteractionState")
    HOST_TYPE(RENDER_CONTEXT, "HRenderContext")
    HOST_TYPE(RESOLUTION, "HResolution", FN__MAKE_RESOLUTION)
    HOST_TYPE(VIEW, "HView")

    inline const ExactHostTypeRef MODE_VALUE(MODE, "ModeValue");
    inline const Roo::MultiRef MODE_REFERENCE({&Roo::Type::SYMBOL_VALUE, &MODE_VALUE},
                                              "ModeReference");
  } // namespace HostType

  namespace Macro
  {
    /*!
     * @brief Declare an immutable bundle of file-backed resources.
     * @since 0.1.0
     * @see pixils/defbundle-dynamic
     * @see pixils.resource/make-resource-dependencies
     *
     * A bundle groups images, music, sounds, and fonts under a symbol. Its
     * resources are subsequently addressed by qualified keywords whose
     * qualifier is the bundle name. Image entries may be file-name strings or
     * maps containing `:file-name` and optional `:transparency-color`; the
     * other resource maps associate names with file-name strings.
     *
     * Bundles declared with `defbundle` are static and cannot be changed with
     * the mutation functions in `pixils.resource`.
     *
     * Usage:
     * @code
     * (pixils/defbundle game-assets
     *   {:images {:ship "images/ship.png"
     *             :cursor {:file-name "images/cursor.png"
     *                      :transparency-color "#ff00ff"}}
     *    :sounds {:laser "audio/laser.wav"}
     *    :music {:theme "audio/theme.ogg"}
     *    :fonts {:body "fonts/body.ttf"}})
     * @endcode
     *
     * | Arg         | Description                                             |
     * | ----------- | ------------------------------------------------------- |
     * | name        | Literal symbol naming the resource bundle.              |
     * | declaration | Map describing image, music, sound, and font resources. |
     *
     * @return `nil` after declaring the bundle.
     */
    SPECIAL_FORM_DECL(DefBundleForm, def_bundle);

    /*!
     * @brief Declare a mutable bundle for runtime-managed resources.
     * @since 0.1.0
     * @see pixils/defbundle
     * @see pixils.resource/create-bundle!
     * @see pixils.resource/add-image!
     * @see pixils.resource/create-image!
     *
     * The optional declaration has the same shape as `defbundle`. Unlike a
     * static bundle, a dynamic bundle may be populated, updated, and pruned at
     * runtime through `pixils.resource`. Omitting the declaration creates an
     * empty bundle.
     *
     * Usage:
     * @code
     * (pixils/defbundle-dynamic project-assets)
     *
     * (pixils/defbundle-dynamic session-assets
     *   {:images {:placeholder "images/placeholder.png"}})
     * @endcode
     *
     * | Arg  | Description                               |
     * | ---- | ----------------------------------------- |
     * | name | Literal symbol naming the dynamic bundle. |
     *
     * | Arg         | Description                               |
     * | ----------- | ----------------------------------------- |
     * | name        | Literal symbol naming the dynamic bundle. |
     * | declaration | Initial resource declaration map.        |
     *
     * @return `nil` after declaring the bundle.
     */
    SPECIAL_FORM_DECL(DefBundleDynamicForm, def_bundle_dynamic);

    /*!
     * @brief Declare a named image-backed mouse pointer.
     * @since 0.1.0
     *
     * The required `:image` is a qualified image-resource keyword. `:source`
     * optionally selects a rectangle from a sprite sheet, `:hotspot` defaults
     * to `{:x 0 :y 0}`, and `:scale` defaults to 1 and is clamped to at least 1.
     *
     * Custom pointers render in the Pixils application buffer by default so
     * they follow its scaling and pixel grid. Set `:render :native` to have
     * SDL create and display an operating-system cursor instead. The pointer is
     * referenced from styles by a keyword with exactly the declared name.
     *
     * Usage:
     * @code
     * (pixils/defpointer workbench/pointer
     *   {:image :workbench-assets/cursors
     *    :source {:x 16 :y 0 :w 16 :h 16}
     *    :hotspot {:x 8 :y 8}
     *    :scale 2})
     *
     * {:style {:cursor :workbench/pointer}}
     * @endcode
     *
     * | Arg        | Description                                      |
     * | ---------- | ------------------------------------------------ |
     * | name       | Literal symbol naming the pointer.               |
     * | definition | Image, crop, hotspot, scale, and render options. |
     *
     * @return `nil` after registering the pointer.
     */
    SPECIAL_FORM_DECL(DefPointerForm, def_pointer);

    /*!
     * @brief Declare a bitmap-atlas or TrueType font.
     * @since 0.1.0
     * @see pixils/defbundle
     *
     * `:resource` is required and identifies either an image resource for a
     * bitmap font or a font resource for a TTF font. `:type` defaults to
     * `:bitmap`; use `:ttf` to load TrueType data. An unqualified font name is
     * registered in the `font` qualifier, while a qualified name is preserved.
     *
     * Bitmap fonts use `:glyphs` to map characters to atlas rectangles. TTF
     * fonts use `:size`, which defaults to `:line-height` when positive and 16
     * otherwise. `:spacing` defaults to 1 and `:line-height` defaults to 0.
     * `:baseline` may override the inferred baseline. The optional
     * `:styles {:underline {:offset n :thickness n}}` map defines underline
     * metrics shared by either font type.
     *
     * Usage:
     * @code
     * (pixils/deffont digits
     *   {:type :bitmap
     *    :resource :game-assets/digits
     *    :spacing 0
     *    :glyphs {'0' {:x 0 :y 0 :w 8 :h 12}
     *             '1' {:x 8 :y 0 :w 8 :h 12}}})
     *
     * (pixils/deffont ui/body
     *   {:type :ttf
     *    :resource :game-assets/body
     *    :size 16})
     * @endcode
     *
     * | Arg        | Description                                      |
     * | ---------- | ------------------------------------------------ |
     * | name       | Literal symbol naming the registered font.      |
     * | definition | Font type, resource, metrics, and glyph options. |
     *
     * @return `nil` after registering the font.
     */
    SPECIAL_FORM_DECL(DefFontForm, def_font);

    /*!
     * @brief Declare a runnable Pixils program.
     * @since 0.1.0
     * @see pixils/make-display
     * @see pixils/deftheme
     * @see pixils/defmode
     *
     * A program selects its initial mode and application-wide display and
     * theme configuration. `:initial-mode` accepts a mode symbol or a view
     * spec such as `{:mode 'game/root :state {:level 1}}`; view specs may
     * provide per-instance mode overrides without declaring a
     * derived mode. `:display` accepts a display value or coercible map and
     * defaults to an automatic-resolution display. `:theme` accepts one theme
     * symbol or a vector of theme symbols composed in order;
     * `:theme-variant` selects a keyword or symbol variant. `:pointer :off`
     * hides the pointer. `:target-frame-rate` controls frame pacing, with 0
     * disabling the frame-rate limit.
     *
     * The resulting program is registered in `pixils/programs` under its
     * literal symbol name.
     *
     * Usage:
     * @code
     * (pixils/defprogram game
     *   {:display {:resolution {:scale 3}
     *              :scaling :scaling/fit
     *              :background "#000000"}
     *    :initial-mode 'game/root
     *    :theme ['pixils/windows-3 'game/layout]
     *    :theme-variant :dark
     *    :target-frame-rate 60})
     * @endcode
     *
     * | Arg        | Description                                        |
     * | ---------- | -------------------------------------------------- |
     * | name       | Literal symbol naming the program.                 |
     * | definition | Display, initial mode, theme, pointer, and pacing. |
     *
     * @return `nil` after registering the program.
     */
    SPECIAL_FORM_DECL(DefProgramForm, def_program);

    /*!
     * @brief Declare a mode definition for application behaviour and state.
     * @since 0.1.0
     *
     * A mode is a registered, reusable view definition. It may represent a
     * screen, a layout container, or another unit of application behaviour.
     * Its ordinary `:init` and `:update` hooks own application state; they are
     * not responsible for preserving component-internal UI state. Rendering,
     * layout, focus, input, children, styling, resources, and event handlers
     * are configured in the definition map. Modes are registered in the
     * `pixils/modes` registry and may use mode composition.
     *
     * Usage:
     * @code
     * (pixils/defmode game-mode
     *   {:init      (fn [state ctx] initial-state)
     *    :update    (fn [state ctx] (update-game-state state ctx))
     *    :focusable true
     *    :render    (fn [state ctx] nil)
     *    :children  [{:component 'ui/button
     *                 :state {:label "Pause"}}]})
     *
     * (pixils/defmode pause-mode
     *   "Pause screen and controls."
     *   {:focusable true
     *    :render (fn [state ctx] nil)})
     * @endcode
     *
     * All hooks are optional. An absent `:init` or `:update` leaves ordinary
     * application state unchanged. `:focusable` defaults to false. A mode
     * definition can inherit from another mode with `:extend`; the derived
     * definition replaces or merges only the fields it supplies.
     *
     * | Arg        | Description                                  |
     * | ---------- | -------------------------------------------- |
     * | name       | Literal symbol naming the mode.              |
     * | definition | Mode definition map or coercible mode value. |
     *
     * | Arg        | Description                                  |
     * | ---------- | -------------------------------------------- |
     * | name       | Literal symbol naming the mode.              |
     * | docstring  | Literal documentation string.                |
     * | definition | Mode definition map or coercible mode value. |
     *
     * @return `nil` after registering the mode definition.
     */
    SPECIAL_FORM_DECL(DefModeForm, declare_mode);

    /*!
     * @brief Declare a reusable UI component definition.
     * @since 0.1.0
     *
     * A component is a distinct runtime definition kind intended for reusable
     * UI elements and controls. It shares the view-definition features of a
     * mode, including application-state `:init` and `:update` hooks, rendering,
     * layout, focus, input, children, styling, resources, and event handlers.
     * Components are registered in the `pixils/components` registry and are
     * not mode-stack frames; they may nevertheless participate in layout
     * trees. Components cannot use mode composition.
     *
     * Component-internal state is kept in a separate `:ui-state` channel. The
     * component lifecycle hooks `:init-ui`, `:update-ui`, and
     * `:after-layout-ui` are provided for primitive components that own such
     * state. They receive `[ui-state state ctx]` and return the next UI-state
     * map, or `nil` to leave it unchanged. This keeps interaction invariants
     * out of ordinary application `:update` hooks, so application updates do
     * not have to preserve or accidentally purge component-internal state.
     * The `:ui/state-keys` field declares state keys that may participate in
     * the optional shared-state policy. The `:ui/models` field declares
     * state-transition models with `:owns`, `:depends-on`, `:transition`, and
     * optional `:change-event` fields. These mechanisms are for component
     * internals; ordinary component authors generally need none of the
     * UI-state hooks.
     *
     * Usage:
     * @code
     * (pixils/defcomponent save-button
     *   {:focusable true
     *    :render    (fn [state ctx] nil)
     *    :on-click  (fn [state ctx] (save-document state))})
     *
     * (pixils/defcomponent toolbar-button
     *   "A button used in toolbars."
     *   {:extend 'save-button
     *    :class :toolbar-button})
     * @endcode
     *
     * A component can inherit from another component with `:extend`; the
     * inherited definition and component UI-state contract are preserved and
     * extended by fields supplied in the derived definition.
     *
     * | Arg        | Description                                  |
     * | ---------- | -------------------------------------------- |
     * | name       | Literal symbol naming the component.         |
     * | definition | Component definition map or coercible value. |
     *
     * | Arg        | Description                                  |
     * | ---------- | -------------------------------------------- |
     * | name       | Literal symbol naming the component.         |
     * | docstring  | Literal documentation string.                |
     * | definition | Component definition map or coercible value. |
     *
     * @return `nil` after registering the component definition.
     */
    SPECIAL_FORM_DECL(DefComponentForm, declare_component);

    /*!
     * @brief Declare a named theme of defaults, variables, and style rules.
     * @since 0.1.0
     * @see pixils/var
     * @see pixils/set-theme!
     *
     * `:defaults` supplies the base style applied to themed views, while
     * `:styles` maps component, class, compound, or pseudo-state selectors to
     * style maps. `:extend` accepts a theme symbol or vector of theme symbols
     * and overlays them in order before applying the new declarations.
     *
     * Theme variables are grouped by variant under `:vars` and referenced in
     * declarations with `pixils/var`. A theme containing variables must name a
     * `:default-variant`; unresolved variables are omitted from the resolved
     * style property.
     *
     * Usage:
     * @code
     * (pixils/deftheme game/theme
     *   {:default-variant :light
     *    :vars {:light {:surface "#ffffff" :text "#101010"}
     *           :dark {:surface "#202020" :text "#f0f0f0"}}
     *    :defaults {:text {:color (pixils/var :text)}}
     *    :styles {:ui/panel {:background (pixils/var :surface)}
     *             :ui/button:hover {:background "#6699cc"}}})
     * @endcode
     *
     * | Arg        | Description                                          |
     * | ---------- | ---------------------------------------------------- |
     * | name       | Literal symbol naming the theme.                     |
     * | definition | Inheritance, variant variables, defaults, and rules. |
     *
     * @return `nil` after registering the theme.
     */
    SPECIAL_FORM_DECL(DefThemeForm, declare_theme);
  } // namespace Macro

  namespace Function
  {
    /*!
     * @brief Create an unregistered mode value from a definition map.
     * @since 0.1.0
     * @see pixils/defmode
     * @see pixils/push-mode!
     *
     * The definition accepts the same mode fields as `defmode`, including
     * lifecycle hooks, rendering, children, style, resources, focus behavior,
     * theme selection, and mode composition. Component-only UI-state fields
     * are rejected. Unlike `defmode`, this function does not add the result to
     * `pixils/modes`; the returned value can be passed directly to
     * `push-mode!`.
     *
     * Usage:
     * @code
     * (pixils/make-mode
     *   {:name "inline-dialog"
     *    :focusable true
     *    :render (fn [state ctx] nil)})
     * => #<HMode>
     * @endcode
     *
     * | Arg        | Description                           |
     * | ---------- | ------------------------------------- |
     * | definition | Map describing the inline mode value. |
     *
     * @return A new native mode value.
     */
    FUNC(MakeMode, make);

    /*!
     * @brief Create an unregistered component value from a definition map.
     * @since 0.1.0
     * @see pixils/defcomponent
     *
     * The definition accepts the same component fields as `defcomponent`,
     * including application-state and UI-state hooks, children, style,
     * resources, focus behavior, and event handlers. The returned component is
     * not added to `pixils/components` and is not itself a valid mode-stack
     * target.
     *
     * Usage:
     * @code
     * (pixils/make-component
     *   {:name "inline-label"
     *    :render (fn [state ctx] nil)})
     * => #<HComponent>
     * @endcode
     *
     * | Arg        | Description                                |
     * | ---------- | ------------------------------------------ |
     * | definition | Map describing the inline component value. |
     *
     * @return A new native component value.
     */
    FUNC(MakeComponent, make);

    /*!
     * @brief Create a mode-composition policy.
     * @since 0.1.0
     * @see pixils/defmode
     *
     * Composition controls whether processing continues into modes beneath
     * the current mode-stack frame. `:render :pass` and `:update :pass` allow
     * lower frames to render and update. `:interaction :pass` allows lower
     * frames to retain interaction, while `:interaction :refresh` recomputes
     * it there. Omitted settings block their corresponding phase.
     *
     * Usage:
     * @code
     * (pixils/make-mode-composition
     *   {:render :pass
     *    :update :block
     *    :interaction :refresh})
     * => #<HModeComposition>
     * @endcode
     *
     * | Arg    | Description                                      |
     * | ------ | ------------------------------------------------ |
     * | policy | Render, update, and interaction composition map. |
     *
     * @return A new native mode-composition value.
     */
    FUNC(MakeModeComposition, make);

    /*!
     * @brief Create a mutable width-and-height dimension value.
     * @since 0.1.0
     *
     * Usage:
     * @code
     * (pixils/make-dimension {:w 320 :h 180})
     * => #<HDimension>
     * @endcode
     *
     * | Arg       | Description                                  |
     * | --------- | -------------------------------------------- |
     * | dimension | Map containing required numeric `:w` and `:h`. |
     *
     * @return A new native dimension value.
     */
    FUNC(MakeDimension, make);

    /*!
     * @brief Create a display configuration.
     * @since 0.1.0
     * @see pixils/make-resolution
     * @see pixils/defprogram
     *
     * `:resolution` is required and accepts a resolution value or coercible
     * resolution map. `:align :align/center` centers fixed content. `:scaling`
     * accepts `:scaling/fit` to preserve aspect ratio or
     * `:scaling/stretch` to fill the output; omitted settings use no alignment
     * or scaling. `:background` defaults to opaque black.
     *
     * Usage:
     * @code
     * (pixils/make-display
     *   {:resolution {:w 320 :h 180}
     *    :align :align/center
     *    :scaling :scaling/fit
     *    :background "#101018"})
     * => #<HDisplay>
     * @endcode
     *
     * | Arg    | Description                                      |
     * | ------ | ------------------------------------------------ |
     * | config | Resolution, alignment, scaling, and background.  |
     *
     * @return A new native display value.
     */
    FUNC(MakeDisplay, make);

    /*!
     * @brief Queue a mode transition onto the Pixils mode stack.
     * @since 0.1.0
     *
     * `push-mode!` requests that Pixils push a new root mode frame. The mode
     * reference may be a registered mode symbol or an inline mode value created
     * with `pixils/make-mode`. A registered component symbol is accepted as
     * shorthand for a generated root mode that contains that component as a
     * state-bound child; component values themselves are not valid push targets.
     *
     * The optional state value becomes the initial state for the pushed root
     * mode. When the pushed root is a component shorthand wrapper, this state is
     * also bound to the wrapped component child.
     *
     * The optional override map applies only to this push. It may override
     * normal mode definition fields such as hooks, style, children, focusable,
     * theme, and drag behavior. It also accepts transition metadata:
     * `:origin` controls where a later `pop-mode!` result event is delivered,
     * and `:overlay` places the pushed root relative to a view or rectangle
     * after layout. Overlay placement accepts `:bottom-start`, `:top-start`,
     * `:right-start`, and `:left-start`.
     *
     * Per-push `:compose :update` accepts `:block`, `:pass`, or
     * `{:pass scopes}`. Full pass updates the immediate underlay; scoped pass
     * updates only its listed `{:view view}` subtrees. An explicitly empty
     * scope list performs no work in the immediate underlay but continues
     * traversal through that frame's update composition.
     *
     * Mode transitions are message-based. Calling `push-mode!` from a hook is
     * safe; the new root becomes active when the session processes queued mode
     * messages.
     *
     * Usage:
     * @code
     * (pixils/push-mode! 'main/game)
     *
     * (pixils/push-mode! 'main/pause-menu {:resumed-from state})

     * (pixils/push-mode!
     *   'main/dialog
     *   {}
     *   {:compose {:update {:pass []}}})
     *
     * (pixils/push-mode!
     *   (pixils/make-mode {:name "inline-popup"
     *                      :children [{:mode 'main/menu-item
     *                                  :state {:label "Open"}}]})
     *   {:anchor {:x 0 :y 24}}
     *   {:origin {:view (:view ctx)
     *             :event :menu/selected}
     *    :overlay {:anchor (:view ctx)
     *              :placement :bottom-start
     *              :fallback-placement :top-start
     *              :viewport-padding 8}})
     * @endcode
     *
     * | Arg  | Description                                                    |
     * | ---- | -------------------------------------------------------------- |
     * | mode | Mode symbol, component symbol shorthand, or inline mode value. |
     *
     * | Arg   | Description                                                    |
     * | ----- | -------------------------------------------------------------- |
     * | mode  | Mode symbol, component symbol shorthand, or inline mode value. |
     * | state | Initial root state.                                            |
     *
     * | Arg       | Description                                                    |
     * | --------- | -------------------------------------------------------------- |
     * | mode      | Mode symbol, component symbol shorthand, or inline mode value. |
     * | state     | Initial root state.                                            |
     * | overrides | Per-push override and transition metadata map.                 |
     *
     * @return The supplied mode reference.
     */
    FUNC(PushModeBangFunction, push_mode);

    /*!
     * @brief Queue removal of the active mode-stack frame.
     * @since 0.1.0
     * @see pixils/push-mode!
     * @see pixils/pop-to!
     *
     * The optional payload is delivered through the transition origin
     * configured when the active frame was pushed. Without a payload, `nil` is
     * delivered. Mode transitions are message-based, so calling `pop-mode!`
     * from a hook is safe; removal occurs when the session processes queued
     * mode messages.
     *
     * Usage:
     * @code
     * (pixils/pop-mode!)
     *
     * (pixils/pop-mode! {:type :confirm :value selection})
     * @endcode
     *
     * | Arg | Description                      |
     * | --- | -------------------------------- |
     * |     | This variant takes no arguments. |
     *
     * | Arg     | Description                                |
     * | ------- | ------------------------------------------ |
     * | payload | Value delivered to the transition origin. |
     *
     * @return `nil`.
     */
    FUNC(PopModeBangFunction, pop_mode);

    /*!
     * @brief Queue removal of modes above the frame containing a view.
     * @since 0.1.0
     *
     * `pop-to!` finds the mode-stack frame containing `target` and queues
     * removal of every frame above it. Intermediate pops receive `nil`; the
     * optional payload is delivered only by the final pop into the target
     * frame. Calling it for a view in the active frame has no effect.
     *
     * Usage:
     * @code
     * (pixils/pop-to! (:view ctx))
     *
     * (pixils/pop-to! (:view ctx)
     *                 {:type :navigate-parent})
     * @endcode
     *
     * | Arg    | Description                                       |
     * | ------ | ------------------------------------------------- |
     * | target | View whose containing frame should become active. |
     *
     * | Arg     | Description                                       |
     * | ------- | ------------------------------------------------- |
     * | target  | View whose containing frame should become active. |
     * | payload | Payload delivered by the final pop.               |
     *
     * @return `nil`.
     */
    FUNC(PopToBangFunction, pop_to);

    /*!
     * @brief Queue a request to stop the active Pixils session.
     * @since 0.1.0
     *
     * The quit request is message-based and takes effect when the session
     * processes its queued mode-stack messages.
     *
     * Usage:
     * @code
     * (pixils/quit!)
     * @endcode
     *
     * @return `nil`.
     */
    FUNC(QuitBangFunction, quit);

    /*!
     * @brief Queue a change to the application-level theme.
     * @since 0.1.0
     * @see pixils/deftheme
     * @see pixils/var
     *
     * `theme` may be one registered theme symbol, a vector of symbols composed
     * in order, or `nil` to clear the application theme. The optional variant
     * is a keyword or symbol. The change takes effect when the session
     * processes its queued mode-stack messages.
     *
     * Usage:
     * @code
     * (pixils/set-theme! ['game/visuals 'game/compact-layout])
     *
     * (pixils/set-theme! 'game/theme :dark)
     * @endcode
     *
     * | Arg   | Description                                      |
     * | ----- | ------------------------------------------------ |
     * | theme | Theme symbol, vector of theme symbols, or `nil`. |
     *
     * | Arg     | Description                                      |
     * | ------- | ------------------------------------------------ |
     * | theme   | Theme symbol, vector of theme symbols, or `nil`. |
     * | variant | Keyword or symbol selecting a theme variant.    |
     *
     * @return The supplied theme value.
     */
    FUNC(SetThemeBangFunction, set_theme);

    /*!
     * @brief Create a late-resolved theme-variable reference.
     * @since 0.1.0
     * @see pixils/deftheme
     * @see pixils/set-theme!
     *
     * Place the returned reference in a `deftheme` style or defaults map. Its
     * keyword or symbol name is resolved against the selected theme variant
     * when declarations are materialized, so changing variants changes the
     * resulting style without redefining it. An unresolved reference causes
     * its containing style property to be omitted.
     *
     * Usage:
     * @code
     * (pixils/deftheme game/theme
     *   {:default-variant :base
     *    :vars {:base {:surface "#202020"}}
     *    :styles {:ui/panel {:background (pixils/var :surface)}}})
     * @endcode
     *
     * | Arg  | Description                               |
     * | ---- | ----------------------------------------- |
     * | name | Keyword or symbol naming a theme variable. |
     *
     * @return A theme-variable reference value.
     */
    FUNC(ThemeVarFunction, theme_var);

    /*!
     * @brief Create a fixed or automatic display resolution.
     * @since 0.1.0
     * @see pixils/make-display
     *
     * A dimension value or a map containing `:w` and `:h` creates a fixed
     * logical buffer size. `:auto` derives the buffer from the window at a
     * 1:1 pixel scale. A map containing `:scale` selects automatic resolution
     * with that pixel scale.
     *
     * Usage:
     * @code
     * (pixils/make-resolution {:w 320 :h 180})
     * => #<HResolution>
     *
     * (pixils/make-resolution :auto)
     * => #<HResolution>
     *
     * (pixils/make-resolution {:scale 3})
     * => #<HResolution>
     * @endcode
     *
     * | Arg       | Description                       |
     * | --------- | --------------------------------- |
     * | dimension | Native fixed-size dimension value. |
     *
     * | Arg       | Description                            |
     * | --------- | -------------------------------------- |
     * | specifier | Keyword resolution specifier `:auto`. |
     *
     * | Arg        | Description                                      |
     * | ---------- | ------------------------------------------------ |
     * | resolution | Map containing `:w` and `:h`, or numeric `:scale`. |
     *
     * @return A new native resolution value.
     */
    FUNC(MakeResolution, make_resolution);

    /*!
     * @brief Move the mouse pointer to a logical Pixils buffer point.
     * @since 0.1.0
     *
     * With a hook context, the function uses that hook's render context and
     * also updates its frame-event mouse position immediately. With only a
     * point, it uses the current global render context. Coordinates are in the
     * logical Pixils buffer rather than output-window pixels.
     *
     * Usage:
     * @code
     * (pixils/warp-mouse! {:x 120 :y 64})
     *
     * (pixils/warp-mouse! ctx {:x 0 :y 0})
     * @endcode
     *
     * | Arg   | Description                                         |
     * | ----- | --------------------------------------------------- |
     * | point | Logical buffer point to which the pointer is moved. |
     *
     * | Arg   | Description                                          |
     * | ----- | ---------------------------------------------------- |
     * | ctx   | Hook context whose renderer and events are used.     |
     * | point | Logical buffer point to which the pointer is moved.  |
     *
     * @return A new point value containing the requested coordinates.
     */
    FUNC(WarpMouseBangFunction, warp_mouse);
  } // namespace Function

  /*! @brief DimensionAdapter - A Roo HostObject Adapter for Dimension */
  NATIVE_ADAPTER(DimensionAdapter, Dimension, (w, h), (w, h));
  /*! @brief Roo HostObject Adapter for Display */
  NATIVE_ADAPTER(DisplayAdapter, Display, (resolution), (resolution));
  /*! @brief FrameEventsAdapter - A Roo HostObject Adapter for FrameEvents */
  NATIVE_ADAPTER(FrameEventsAdapter, FrameEvents, (key_down, held_keys));
  /*! @brief HookContextAdapter - unified context passed as second arg to all mode hooks */
  NATIVE_ADAPTER(HookContextAdapter,
                 HookContext,
                 (key_down,
                  held_keys,
                  mouse_pos,
                  mouse_button_down,
                  mouse_button_up,
                  mouse_wheel,
                  mouse_held,
                  pixel_size,
                  buffer_dim,
                  available_width,
                  available_height,
                  view,
                  interaction_scope));
  /*! @brief InteractionStateAdapter - engine-computed hover/focus/press state on a view */
  NATIVE_ADAPTER(InteractionStateAdapter,
                 UI::InteractionState,
                 (hovered, focused, focus_within, pressed));
  /**
   * @brief Roo HostObject Adapter for View.
   *
   * `content_bounds` exposes the logical content rectangle produced by the
   * effective style's border and padding, in the same coordinate space as
   * `bounds`.
   */
  NATIVE_ADAPTER(ViewAdapter,
                 Runtime::View,
                 (id,
                  state,
                  ui_state,
                  state_policy,
                  bounds,
                  content_bounds,
                  external_bounds,
                  visual_bounds,
                  visual_scale,
                  interaction,
                  style,
                  effective_style,
                  on_mouse_down,
                  on_mouse_up,
                  on_mouse_wheel,
                  on_click,
                  on_double_click));
  /*! @brief ModeAdapter - A Roo HostObject Adapter for Mode */
  NATIVE_ADAPTER(ModeAdapter, Runtime::Mode, (init, update, render));
  /*! @brief ComponentAdapter - A Roo HostObject Adapter for Component */
  NATIVE_ADAPTER(ComponentAdapter, Runtime::Component, (init, update, render));
  /*! @brief ModeCompositionAdapter - A Roo HostObject Adapter for ModeComposition */
  NATIVE_ADAPTER(ModeCompositionAdapter,
                 Runtime::ModeComposition,
                 (render, update, interaction));
  /*! @brief Roo HostObject Adapter for Program */
  NATIVE_ADAPTER(ProgramAdapter,
                 Program,
                 (name, display, initial_mode, theme, theme_variant, target_frame_rate),
                 (display));
  /*! @brief Roo HostObject Adapter for RenderContext */
  NATIVE_ADAPTER(RenderContextAdapter, RenderContext, (pixel_size, buffer_dim));
  /*! @brief Roo HostObject Adapter for Resolution */
  NATIVE_ADAPTER(ResolutionAdapter, Resolution, (dimension));
  class PixilsNamespace : public Roo::Namespace
  {
   public:
    PixilsNamespace(const RenderContext& render_context);
    explicit PixilsNamespace(std::unique_ptr<RenderContext> render_context);

   private:
    explicit PixilsNamespace(Roo::sptr_val render_context);
  };

} // namespace Pixils::Script

#endif
