
#ifndef __PIXILS__PIXILS_NAMESPACE_
#define __PIXILS__PIXILS_NAMESPACE_

#include "context.h"
#include "frame_events.h"
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

    inline const std::string FN_PIXILS__MAKE_COORDINATE = "pixils/coordinate";

    inline const std::string FN__MAKE_COORDINATE = "coordinate";
    inline const std::string FN__MAKE_DIMENSION = "dimension";
    inline const std::string FN__DRAW_LINE_BANG = "draw-line!";
    inline const std::string FN__DRAW_POLYGON_BANG = "draw-polygon!";
    inline const std::string FN__ROTATE_COORDINATE = "rotate-coordinate";
    inline const std::string FN__COORDINATE_DIVISION = "coordinate-div";
    inline const std::string FN__COORDINATE_PLUS = "coordinate+";
    inline const std::string FN__COORDINATE_MINUS = "coordinate-";
    inline const std::string FN__DISTANCE_BETWEEN = "distance-between";
    inline const std::string FN__USE_COLOR_BANG = "use-color!";

    namespace MapKey
    {
      DECL_SHKEY(BUFFER_SIZE)
      DECL_SHKEY(H)
      DECL_SHKEY(HELD_KEYS)
      DECL_SHKEY(KEY_DOWN)
      DECL_SHKEY(PIXEL_SIZE)
      DECL_SHKEY(W)
      DECL_SHKEY(X)
      DECL_SHKEY(Y)
    } // namespace MapKey

    namespace HostType
    {
      HOST_TYPE(COORDINATE, "HCoordinate", FN_PIXILS__MAKE_COORDINATE)
      HOST_TYPE(DIMENSION, "HDimension", FN__MAKE_DIMENSION)
      HOST_TYPE(FRAME_EVENTS, "HFrameEvents")
      HOST_TYPE(RENDER_CONTEXT, "HRenderContext")

      inline const Lisple::SeqRef VECTOR_OF_COORDINATE(&Lisple::Type::ARRAY, &COORDINATE,
                                                       "[Coordinate]");
    } // namespace HostType

    namespace Function
    {
      /*! @brief Lisple Function that constructs a new instance of Coordinate/CoordinateAdapter */
      FUNC_DECL(MakeCoordinate, make_coordinate_from_ints, make_coordinate_from_map);
      /*! @brief Lisple Function that constructs a new instance of Dimension/DimensionAdapter */
      FUNC_DECL(MakeDimension, make);
      /*! @brief Divide coordinate values  */
      FUNC_DECL(CoordinateDivision, divide_num);
      /*! @brief Combines two coordinates using addition */
      FUNC_DECL(CoordinatePlus, plus);
      /*! @brief Combines two coordinates using subtraction  */
      FUNC_DECL(CoordinateMinus, minus);
      /*! @brief Distance between */
      FUNC_DECL(DistanceBetween, distance_between_points);
      /*! @brief Rotate coordinate around point */
      FUNC_DECL(RotateCoordinate, rotate_coordinate);
      /*! @brief draw-line! function */
      FUNC_DECL(DrawLineBang, draw_line);
      /*! @brief draw-polygon! function */
      FUNC_DECL(DrawPolygonBang, draw_polygon, draw_polygon_with_opts);
      /*! @brief use-color! function */
      FUNC_DECL(UseColorBang, use_color);
    } // namespace Function

    /*! @brief CoordinateAdapter - A Lisple HostObject Adapter for Coordinate */
    HOST_ADAPTER(CoordinateAdapter, Coordinate, (x, y), (x, y));
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
