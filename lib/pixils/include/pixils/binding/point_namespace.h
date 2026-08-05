
#ifndef PIXILS__POINT_NAMESPACE_H
#define PIXILS__POINT_NAMESPACE_H

#include <pixils/binding/shkey.h>
#include <pixils/geom.h>

#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/namespace.h>
#include <string>

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils.point" roo namespace name
   */
  inline constexpr std::string_view NS__PIXILS__POINT = "pixils.point";

  inline constexpr std::string_view FN__MAKE_POINT = "make-point";
  inline const std::string FN__PIXILS__POINT__MAKE_POINT = "pixils.point/make-point";

  inline constexpr std::string_view FN__DIVIDE = "div";
  inline constexpr std::string_view FN__INT_POINT = "int";
  inline constexpr std::string_view FN__PLUS = "+";
  inline constexpr std::string_view FN__MINUS = "-";
  inline constexpr std::string_view FN__MULTIPLY = "*";
  inline constexpr std::string_view FN__ROTATE = "rotate";
  inline constexpr std::string_view FN__DISTANCE = "distance";
  inline constexpr std::string_view FN__DISTANCE_SQUARED = "distance-squared";
  inline constexpr std::string_view FN__CLAMP = "clamp";
  inline constexpr std::string_view FN__MAX = "max";
  inline constexpr std::string_view FN__MIN = "min";
  inline constexpr std::string_view FN__TRANSLATE = "translate";
  inline constexpr std::string_view FN__TRANSLATE_X = "translate-x";
  inline constexpr std::string_view FN__TRANSLATE_Y = "translate-y";
  inline constexpr std::string_view FN__WRAP = "wrap";

  namespace MapKey
  {
    DECL_SHKEY(ORIGIN)
    DECL_SHKEY(RADIANS)
    DECL_SHKEY(X)
    DECL_SHKEY(Y)
  } // namespace MapKey

  namespace HostType
  {
    HOST_TYPE(POINT, "HPoint", std::string(FN__PIXILS__POINT__MAKE_POINT));

    inline const Roo::MultiRef NUMBER_OR_POINT({&Roo::Type::NUMBER, &POINT},
                                               "number|HPoint");
    inline const Roo::SeqRef VECTOR_OF_POINT(&Roo::Type::VECTOR, &POINT, "[HPoint]");

  } // namespace HostType

  namespace Function
  {
    /*! @brief IntPointFunction */
    FUNC(IntPointFunction, int);
    /*! @brief Roo Function that constructs a new instance of Point/PointAdapter */
    FUNC(MakePoint, point_from_ints, point_from_map);
    /*! @brief Multiply point values  */
    FUNC(PointMultiplication, multiply_num);
    /*! @brief Divide point values  */
    FUNC(PointDivision, divide_num);
    /*! @brief Combines two points using addition */
    FUNC(PointPlus, plus);
    /*! @brief Combines two points using subtraction  */
    FUNC(PointMinus, minus);
    /*! @brief Distance between */
    FUNC(DistanceBetween, distance);
    /*! @brief Squared Euclidean distance between two points */
    FUNC(DistanceSquared, distance_squared);
    /*!
     * Returns a point where each axis is the lesser of the input point axis and
     * the matching bound axis. The bound may be a point or a scalar number; a
     * scalar applies to both axes.
     *
     * Usage:
     * @code
     * (pixils.point/min {:x 10 :y 20} 15)
     * => {:x 10 :y 15}
     *
     * (pixils.point/min {:x 10 :y 20} {:x 8 :y 30})
     * => {:x 8 :y 20}
     * @endcode
     *
     * @returns A `pixils.point` with per-axis minimum values.
     *
     * | Param | Description                               |
     * | ----- | ----------------------------------------- |
     * | point | Point to compare.                         |
     * | bound | Point or scalar upper bound for each axis. |
     */
    FUNC(PointMinimum, point_min);
    /*!
     * Returns a point where each axis is the greater of the input point axis and
     * the matching bound axis. The bound may be a point or a scalar number; a
     * scalar applies to both axes.
     *
     * Usage:
     * @code
     * (pixils.point/max {:x -5 :y 20} 0)
     * => {:x 0 :y 20}
     *
     * (pixils.point/max {:x -5 :y 20} {:x -2 :y 30})
     * => {:x -2 :y 30}
     * @endcode
     *
     * @returns A `pixils.point` with per-axis maximum values.
     *
     * | Param | Description                               |
     * | ----- | ----------------------------------------- |
     * | point | Point to compare.                         |
     * | bound | Point or scalar lower bound for each axis. |
     */
    FUNC(PointMaximum, point_max);
    /*!
     * Clamps a point to either a rect or explicit per-axis bounds.
     *
     * The two-argument form accepts a rect and clamps `point` to the rect
     * bounds. The three-argument form accepts minimum and maximum bounds, where
     * each bound may be a point or a scalar number. A scalar applies to both
     * axes.
     *
     * Usage:
     * @code
     * (pixils.point/clamp {:x -5 :y 30} {:x 10 :y 20 :w 100 :h 50})
     * => {:x 10 :y 30}
     *
     * (pixils.point/clamp {:x -5 :y 20} 0 15)
     * => {:x 0 :y 15}
     *
     * (pixils.point/clamp {:x -20 :y 80}
     *                     {:x -15 :y -30}
     *                     {:x 15 :y 55})
     * => {:x -15 :y 55}
     * @endcode
     *
     * @returns A `pixils.point` clamped to the requested bounds.
     *
     * | Param | Description                                         |
     * | ----- | --------------------------------------------------- |
     * | point | Point to clamp.                                     |
     * | rect  | Rect bounds for the two-argument form.              |
     * | min   | Point or scalar minimum bound for each axis.        |
     * | max   | Point or scalar maximum bound for each axis.        |
     */
    FUNC(ClampPoint, clamp_rect, clamp_bounds);
    /*! @brief Translate point by x/y delta */
    FUNC(TranslatePoint, translate);
    /*! @brief Translate point along x axis */
    FUNC(TranslatePointX, translate_x);
    /*! @brief Translate point along y axis */
    FUNC(TranslatePointY, translate_y);
    /*! @brief Wrap point around rect bounds */
    FUNC(WrapPoint, wrap);
    /*! @brief Rotate point around point */
    FUNC(RotatePoint, orig_amount, amount, with_opts);
  } // namespace Function

  /*! @brief PointAdapter - A Roo HostObject Adapter for Point */
  NATIVE_ADAPTER(PointAdapter, Point, (x, y), (x, y));

  class PointNamespace : public Roo::Namespace
  {
   public:
    PointNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__POINT_NAMESPACE_H */
