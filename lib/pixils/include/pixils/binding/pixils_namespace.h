
#ifndef PIXILS__PIXILS_NAMESPACE
#define PIXILS__PIXILS_NAMESPACE

#include <pixils/binding/shkey.h>
#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/program.h>
#include <pixils/ui/theme.h>

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
    /*! @brief Define a resource bundle independent of any mode */
    SPECIAL_FORM_DECL(DefBundleForm, def_bundle);
    /*! @brief Define a mutable resource bundle for runtime-managed assets */
    SPECIAL_FORM_DECL(DefBundleDynamicForm, def_bundle_dynamic);
    /*! @brief Define a named custom pointer */
    SPECIAL_FORM_DECL(DefPointerForm, def_pointer);
    /*! @brief Define a custom font */
    SPECIAL_FORM_DECL(DefFontForm, def_font);
    /*! @brief Define a program/application */
    SPECIAL_FORM_DECL(DefProgramForm, def_program);
    /*!
     * @brief Declare a mode definition for application behaviour and state.
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
     * (defmode game-mode
     *   {:init      (fn [state ctx] initial-state)
     *    :update    (fn [state ctx] (update-game-state state ctx))
     *    :focusable true
     *    :render    (fn [state ctx] nil)
     *    :children  [{:component 'ui/button
     *                 :state {:label "Pause"}}]})
     *
     * (defmode pause-mode
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
     * @return `nil` after registering the mode definition.
     * @since 0.1.0
     */
    SPECIAL_FORM_DECL(DefModeForm, declare_mode);
    /*!
     * @brief Declare a reusable UI component definition.
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
     * (defcomponent save-button
     *   {:focusable true
     *    :render    (fn [state ctx] nil)
     *    :on-click  (fn [state ctx] (save-document state))})
     *
     * (defcomponent toolbar-button
     *   "A button used in toolbars."
     *   {:extend 'save-button
     *    :class :toolbar-button})
     * @endcode
     *
     * A component can inherit from another component with `:extend`; the
     * inherited definition and component UI-state contract are preserved and
     * extended by fields supplied in the derived definition.
     *
     * @return `nil` after registering the component definition.
     * @since 0.1.0
     */
    SPECIAL_FORM_DECL(DefComponentForm, declare_component);
    /*! @brief Define a named theme */
    SPECIAL_FORM_DECL(DefThemeForm, declare_theme);
  } // namespace Macro

  namespace Function
  {
    /*! @brief Roo make-function for Mode/ModeAdapter */
    FUNC(MakeMode, make);
    /*! @brief Roo make-function for Component/ComponentAdapter */
    FUNC(MakeComponent, make);
    /*! @brief Roo make-function for ModeComposition/ModeCompositionAdapter */
    FUNC(MakeModeComposition, make);
    /*! @brief Roo make-function for Dimension/DimensionAdapter */
    FUNC(MakeDimension, make);
    /*! @brief Roo make-function for Display/DisplayAdapter */
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
     * after layout.
     *
     * Mode transitions are message-based. Calling `push-mode!` from a hook is
     * safe; the new root becomes active when the session processes queued mode
     * messages.
     *
     * Usage:
     * @code
     * (pixils/push-mode! 'main/pause-menu {:resumed-from state})
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
     * | Arg       | Description                                                    |
     * |-----------|----------------------------------------------------------------|
     * | mode      | Mode symbol, component symbol shorthand, or inline mode value. |
     * | state     | Optional initial root state. Defaults to nil.                  |
     * | overrides | Optional per-push override and transition metadata map.        |
     *
     * @return The supplied mode reference.
     */
    FUNC(PushModeBangFunction, push_mode);
    /*! @brief Pop active mode */
    FUNC(PopModeBangFunction, pop_mode);
    /*! @brief Request application shutdown */
    FUNC(QuitBangFunction, quit);
    /*! @brief Switch the application-level theme */
    FUNC(SetThemeBangFunction, set_theme);
    /*! @brief Create a theme variable reference for deftheme styles */
    FUNC(ThemeVarFunction, theme_var);
    /*! @brief Roo make-function for Resolution/ResolutionAdapter */
    FUNC(MakeResolution, make_resolution);
    /*! @brief Move the OS mouse pointer to a logical Pixils buffer point */
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
                  view));
  /*! @brief InteractionStateAdapter - engine-computed hover/focus/press state on a view */
  NATIVE_ADAPTER(InteractionStateAdapter,
                 UI::InteractionState,
                 (hovered, focused, focus_within, pressed));
  /*! @brief Roo HostObject Adapter for View */
  NATIVE_ADAPTER(ViewAdapter,
                 Runtime::View,
                 (id,
                  state,
                  ui_state,
                  state_policy,
                  bounds,
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
  };

} // namespace Pixils::Script

#endif
