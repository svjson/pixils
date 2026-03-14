
#ifndef PIXILS__PIXILS_NAMESPACE
#define PIXILS__PIXILS_NAMESPACE

#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/runtime/mode.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils"
   */
  inline const std::string NS_PIXILS = "pixils";

  inline const std::string FN__PIXILS__MAKE_MODE = "pixils/make-mode";
  inline const std::string FN__MAKE_DIMENSION = "dimension";
  inline const std::string FN__POP_MODE_BANG = "pop-mode!";
  inline const std::string FN__PUSH_MODE_BANG = "push-mode!";

  inline const Lisple::Word ID__PIXILS__MODE_STACK("pixils/mode-stack");
  inline const Lisple::Word ID__PIXILS__MODES("pixils/modes");
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
    HOST_TYPE(DIMENSION, "HDimension", FN__MAKE_DIMENSION)
    HOST_TYPE(FRAME_EVENTS, "HFrameEvents")
    HOST_TYPE(MODE, "HMode", FN__PIXILS__MAKE_MODE);
    HOST_TYPE(RENDER_CONTEXT, "HRenderContext")
  } // namespace HostType

  namespace Macro
  {
    /*! @brief Define a game/application mode */
    MACRO_DECL(DefModeMacro, declare_mode);
  } // namespace Macro

  namespace Function
  {
    /*! @brief Lisple make-function for Mode/ModeAdapter */
    FUNC_DECL(MakeMode, make);
    /*! @brief Lisple make-function for Dimension/DimensionAdapter */
    FUNC_DECL(MakeDimension, make);
    /*! @brief Push active mode */
    FUNC_DECL(PushModeBangFunction, push_mode);
    /*! @brief Pop active mode */
    FUNC_DECL(PopModeBangFunction, pop_mode);
  } // namespace Function

  /*! @brief ModeAdapter - A Lisple HostObejct Adapter for Mode */
  HOST_ADAPTER(ModeAdapter, Runtime::Mode, (init, update, render));
  /*! @brief DimensionAdapter - A Lisple HostObject Adapter for Dimension */
  HOST_ADAPTER(DimensionAdapter, Dimension, (w, h), (w, h));
  /*! @brief FrameEventsAdapter - A Lisple HostObject Adapter for FrameEvents */
  HOST_ADAPTER(FrameEventsAdapter, FrameEvents, (key_down, held_keys));
  /*! @brief Lisple HostObject Adapter for RenderContext */
  HOST_ADAPTER(RenderContextAdapter, RenderContext, (pixel_size, buffer_size));

  class PixilsNamespace : public Lisple::Namespace
  {
  public:
    PixilsNamespace(const RenderContext& render_context);
  };

} // namespace Pixils::Script

#endif
