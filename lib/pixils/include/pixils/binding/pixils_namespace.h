
#ifndef __PIXILS__PIXILS_NAMESPACE_
#define __PIXILS__PIXILS_NAMESPACE_

#include <pixils/context.h>
#include <pixils/frame_events.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils
{
  namespace Script
  {
    /*!
     * @brief Constant for "pixils"
     */
    inline const std::string NS_PIXILS = "pixils";

    inline const std::string FN__MAKE_COORDINATE = "coordinate";
    inline const std::string FN__MAKE_DIMENSION = "dimension";

    inline const Lisple::Word ID__PIXILS__RENDER_CONTEXT("pixils/render-context");

    namespace MapKey
    {
      DECL_SHKEY(BUFFER_SIZE)
      DECL_SHKEY(H)
      DECL_SHKEY(HELD_KEYS)
      DECL_SHKEY(KEY_DOWN)
      DECL_SHKEY(PIXEL_SIZE)
      DECL_SHKEY(W)
    } // namespace MapKey

    namespace HostType
    {
      HOST_TYPE(DIMENSION, "HDimension", FN__MAKE_DIMENSION)
      HOST_TYPE(FRAME_EVENTS, "HFrameEvents")
      HOST_TYPE(RENDER_CONTEXT, "HRenderContext")
    } // namespace HostType

    namespace Function
    {
      /*! @brief Lisple Function that constructs a new instance of Dimension/DimensionAdapter */
      FUNC_DECL(MakeDimension, make);
    } // namespace Function

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

  } // namespace Script

} // namespace Pixils

#endif
