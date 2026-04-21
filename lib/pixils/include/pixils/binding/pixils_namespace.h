
#ifndef PIXILS__PIXILS_NAMESPACE
#define PIXILS__PIXILS_NAMESPACE

#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/program.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>

namespace Pixils::Runtime
{
  struct Mode;
  struct ModeComposition;
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
  inline const std::string FN__PIXILS__MAKE_RECT = "pixils/make-rect";
  inline const std::string FN__MAKE_DISPLAY = "display";
  inline const std::string FN__MAKE_RESOLUTION = "pixils/make-resolution";
  inline const std::string FN__POP_MODE_BANG = "pop-mode!";
  inline const std::string FN__PUSH_MODE_BANG = "push-mode!";

  inline const Lisple::Word ID__PIXILS__MODE_STACK("pixils/mode-stack");
  inline const Lisple::Word ID__PIXILS__MODE_STACK_MESSAGES("pixils/mode-stack-messages");
  inline const Lisple::Word ID__PIXILS__MODES("pixils/modes");
  inline const Lisple::Word ID__PIXILS__PROGRAMS("pixils/programs");
  inline const Lisple::Word ID__PIXILS__RENDER_CONTEXT("pixils/render-context");

  namespace MapKey
  {
    DECL_SHKEY(BUFFER_SIZE)
    DECL_SHKEY(H)
    DECL_SHKEY(HELD_KEYS)
    DECL_SHKEY(INIT)
    DECL_SHKEY(KEY_DOWN)
    DECL_SHKEY(PIXEL_SIZE)
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
    HOST_TYPE(RECT, "HRect", FN__PIXILS__MAKE_RECT);
    HOST_TYPE(RENDER_CONTEXT, "HRenderContext")
    HOST_TYPE(RESOLUTION, "HResolution", FN__MAKE_RESOLUTION)
  } // namespace HostType

  namespace Macro
  {
    /*! @brief Define a custom font */
    SPECIAL_FORM_DECL(DefFontForm, def_font);
    /*! @brief Define a program/application */
    SPECIAL_FORM_DECL(DefProgramForm, def_program);
    /*! @brief Define a game/application mode */
    SPECIAL_FORM_DECL(DefModeForm, declare_mode);
  } // namespace Macro

  namespace Function
  {
    /*! @brief Lisple make-function for Mode/ModeAdapter */
    FUNC(MakeMode, make);
    /*! @brief Lisple make-function for ModeComposition/ModeCompositionAdapter */
    FUNC(MakeModeComposition, make);
    /*! @brief Lisple make-function for Dimension/DimensionAdapter */
    FUNC(MakeDimension, make);
    /*! @brief Lisple make-function for Display/DisplayAdapter */
    FUNC(MakeDisplay, make);
    /*! @brief Lisple make-function for Rect/RectAdapter */
    FUNC(MakeRect, make);
    /*! @brief Push active mode */
    FUNC(PushModeBangFunction, push_mode);
    /*! @brief Pop active mode */
    FUNC(PopModeBangFunction, pop_mode);
    /*! @brief Lisple make-function for Resolution/ResolutionAdapter */
    FUNC(MakeResolution, make_resolution);
  } // namespace Function

  /*! @brief DimensionAdapter - A Lisple HostObject Adapter for Dimension */
  NATIVE_ADAPTER(DimensionAdapter, Dimension, (w, h), (w, h));
  /*! @brief RectAdapter - A Lisple HostObject Adapter for Rect */
  NATIVE_ADAPTER(RectAdapter, Rect, (x, y, w, h), (x, y, w, h));
  /*! @brief Lisple HostObject Adapter for Display */
  NATIVE_ADAPTER(DisplayAdapter, Display, (resolution), (resolution));
  /*! @brief FrameEventsAdapter - A Lisple HostObject Adapter for FrameEvents */
  NATIVE_ADAPTER(FrameEventsAdapter, FrameEvents, (key_down, held_keys));
  /*! @brief HookContextAdapter - unified context passed as second arg to all mode hooks */
  NATIVE_ADAPTER(
    HookContextAdapter,
    HookContext,
    (key_down, held_keys, mouse_pos, mouse_button_down, mouse_held, pixel_size, buffer_dim));
  /*! @brief ModeAdapter - A Lisple HostObject Adapter for Mode */
  NATIVE_ADAPTER(ModeAdapter, Runtime::Mode, (init, update, render));
  /*! @brief ModeCompositionAdapter - A Lisple HostObject Adapter for ModeComposition */
  NATIVE_ADAPTER(ModeCompositionAdapter, Runtime::ModeComposition, (render));
  /*! @brief Lisple HostObject Adapter for Program */
  NATIVE_ADAPTER(ProgramAdapter, Program, (name, display, initial_mode), (display));
  /*! @brief Lisple HostObject Adapter for RenderContext */
  NATIVE_ADAPTER(RenderContextAdapter, RenderContext, (pixel_size, buffer_dim));
  /*! @brief Lisple HostObject Adapter for Resolution */
  NATIVE_ADAPTER(ResolutionAdapter, Resolution, (dimension));

  class PixilsNamespace : public Lisple::Namespace
  {
   public:
    PixilsNamespace(const RenderContext& render_context);
  };

} // namespace Pixils::Script

#endif
