
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

namespace Pixils::UI
{
  struct InteractionState;
} // namespace Pixils::UI

namespace Pixils::Runtime
{
  struct Mode;
  struct ModeComposition;
  struct View;
} // namespace Pixils::Runtime

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils"
   */
  inline const std::string NS_PIXILS = "pixils";

  inline const std::string FN__PIXILS__MAKE_DISPLAY = "pixils/make-display";
  inline const std::string FN__PIXILS__MAKE_MODE = "pixils/make-mode";
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
    HOST_TYPE(MODE_COMPOSITION, "HModeComposition", FN__PIXILS__MAKE_MODE_COMPOSITION)
    HOST_TYPE(PROGRAM, "HProgram")
    HOST_TYPE(INTERACTION_STATE, "HInteractionState")
    HOST_TYPE(RENDER_CONTEXT, "HRenderContext")
    HOST_TYPE(RESOLUTION, "HResolution", FN__MAKE_RESOLUTION)
    HOST_TYPE(VIEW, "HView")
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
    /*! @brief Define a game/application mode */
    SPECIAL_FORM_DECL(DefModeForm, declare_mode);
    /*! @brief Define a named theme */
    SPECIAL_FORM_DECL(DefThemeForm, declare_theme);
  } // namespace Macro

  namespace Function
  {
    /*! @brief Roo make-function for Mode/ModeAdapter */
    FUNC(MakeMode, make);
    /*! @brief Roo make-function for ModeComposition/ModeCompositionAdapter */
    FUNC(MakeModeComposition, make);
    /*! @brief Roo make-function for Dimension/DimensionAdapter */
    FUNC(MakeDimension, make);
    /*! @brief Roo make-function for Display/DisplayAdapter */
    FUNC(MakeDisplay, make);
    /*! @brief Push active mode */
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
                  bounds,
                  external_bounds,
                  interaction,
                  style,
                  effective_style,
                  on_mouse_down,
                  on_mouse_up,
                  on_click,
                  on_double_click));
  /*! @brief ModeAdapter - A Roo HostObject Adapter for Mode */
  NATIVE_ADAPTER(ModeAdapter, Runtime::Mode, (init, update, render));
  /*! @brief ModeCompositionAdapter - A Roo HostObject Adapter for ModeComposition */
  NATIVE_ADAPTER(ModeCompositionAdapter, Runtime::ModeComposition, (render));
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
