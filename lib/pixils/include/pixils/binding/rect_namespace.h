
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

  inline constexpr std::string_view FN__ALIGN = "align";
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
     * RectAlignFunction - pixils.rect/align
     *
     * Aligns a size inside a rect and returns the resulting rect.
     *
     * The numeric form places the inner size at normalized x/y factors. Use `0`
     * for the start edge, `0.5` for the center, and `1` for the end edge.
     *
     * The keyword form accepts one of `:top-left`, `:top-center`, `:top-right`,
     * `:center-left`, `:center`, `:center-right`, `:bottom-left`,
     * `:bottom-center`, or `:bottom-right`.
     *
     * The options-map form separates the point in the outer rect (`:at`) from
     * the point on the aligned inner rect (`:origin`). When `:origin` is omitted,
     * it defaults to the same anchor as `:at`.
     *
     * Usage:
     * @code
     * (pixils.rect/align {:x 10 :y 20 :w 100 :h 50}
     *                    {:w 20 :h 10}
     *                    0.5
     *                    0.5)
     * => {:x 50 :y 40 :w 20 :h 10}
     *
     * (pixils.rect/align {:x 10 :y 20 :w 100 :h 50}
     *                    {:w 20 :h 10}
     *                    :bottom-center)
     * => {:x 50 :y 60 :w 20 :h 10}
     *
     * (pixils.rect/align {:x 10 :y 20 :w 100 :h 50}
     *                    {:w 20 :h 10}
     *                    {:at :center
     *                     :origin :top-left})
     * => {:x 60 :y 45 :w 20 :h 10}
     * @endcode
     *
     * @returns A `pixils.rect` for the aligned size and position.
     *
     * | Param          | Description                                          |
     * | -------------- | ---------------------------------------------------- |
     * | rect           | Rect that receives the aligned size.                 |
     * | size           | Map with numeric `:w` and `:h` entries.              |
     * | x-factor       | Horizontal alignment factor; `0.5` centers.          |
     * | y-factor       | Vertical alignment factor; `0.5` centers.            |
     * | anchor         | Keyword anchor for both `:at` and `:origin`.         |
     * | options        | Map with `:at` anchor and optional `:origin` anchor. |
     */
    FUNC(RectAlignFunction, align_numeric, align_anchor, align_options);

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
