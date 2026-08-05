
#ifndef PIXILS__RECT_NAMESPACE_H
#define PIXILS__RECT_NAMESPACE_H

#include <roo/exec.h>
#include <roo/host/object.h>
#include <string>

namespace Pixils
{
  struct Rect;
}

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils.rect" roo namespace name
   */
  inline constexpr std::string_view NS__PIXILS__RECT = "pixils.rect";

  inline const std::string FN__PIXILS__MAKE_RECT = "pixils.rect/make-rect";
  inline constexpr std::string_view FN__NORMALIZE_POINT = "normalize-point";

  namespace HostType
  {
    HOST_TYPE(RECT, "HRect", FN__PIXILS__MAKE_RECT);
  }

  namespace Function
  {
    /*! @brief Roo make-function for Rect/RectAdapter */
    FUNC(MakeRect, make);
    /*! @brief Checks if a Rect contains a Point, Rect, or polygon */
    FUNC(RectContainsFunction, contains_point, contains_rect, contains_polygon);
    /*! @brief Checks if a Rect intersects a Rect or polygon */
    FUNC(RectIntersectsFunction, intersects_rect, intersects_polygon, intersects_with_opts);
    /*! @brief Checks if a Point is inside the bounds of a Rect */
    FUNC(InsidePFunction, inside);
    /*! @brief Checks if to Rects intersect each other */
    FUNC(IntersectPFunction, intersect);
    /*!
     * @brief Return a point at normalized coordinates inside a rect.
     *
     * `{:x 0 :y 0}` addresses the top-left corner of the rect and
     * `{:x 1 :y 1}` addresses the bottom-right corner. Factors are not clamped,
     * so callers may extrapolate outside the rect. Returned coordinates are
     * fractional when the inputs produce fractional positions; use
     * `pixils.point/int` when pixel-grid coordinates are required.
     *
     * Usage:
     * @code
     * (:x (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 0.25 0.5))
     * => 12.0
     *
     * (:y (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 1.25 -0.5))
     * => 17.0
     * @endcode
     *
     * @returns A `pixils.point` at the requested normalized rect position.
     *
     * | Param    | Description                                                |
     * | -------- | ---------------------------------------------------------- |
     * | rect     | Rect to sample.                                            |
     * | x-factor | Horizontal factor; `0` is left and `1` is right.           |
     * | y-factor | Vertical factor; `0` is top and `1` is bottom.             |
     */
    FUNC(RectNormalizePointFunction, normalize_point);
  } // namespace Function

  /*! @brief RectAdapter - A Roo HostObject Adapter for Rect */
  NATIVE_ADAPTER(RectAdapter, Rect, (x, y, w, h), (x, y, w, h));

  class RectNamespace : public Roo::Namespace
  {
   public:
    RectNamespace();
  };

} // namespace Pixils::Script

#endif /* RECT_NAMESPACE_H */
