
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
  inline constexpr std::string_view FN__EQUAL = "=";
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
    /*!
     * @brief Converts a point's coordinates to integers by truncating
     * their fractional parts.
     *
     * Usage:
     * @code
     * (pixils.point/int {:x 10.75 :y -4.25})
     * => {:x 10 :y -4}
     * @endcode
     *
     * | Arg   | Description       |
     * | ----- | ----------------- |
     * | point | Point to convert. |
     *
     * @return A `pixils.point` with integer coordinates.
     *
     * @since 0.1.0
     */
    FUNC(IntPointFunction, int);

    /*!
     * @brief Constructs a point from x and y coordinates or from a map
     * containing `:x` and `:y`.
     *
     * Usage:
     * @code
     * (pixils.point/make-point 10 20)
     * => {:x 10 :y 20}
     *
     * (pixils.point/make-point {:x 10 :y 20})
     * => {:x 10 :y 20}
     * @endcode
     *
     * | Arg   | Description                                      |
     * | ----- | ------------------------------------------------ |
     * | x     | Horizontal coordinate for the numeric form.      |
     * | y     | Vertical coordinate for the numeric form.        |
     * | point | Map with numeric `:x` and `:y` entries.           |
     *
     * @return A new `pixils.point` with the supplied coordinates.
     *
     * @since 0.1.0
     */
    FUNC(MakePoint, point_from_ints, point_from_map);

    /*!
     * @brief Multiplies both coordinates of a point by a scalar.
     *
     * Usage:
     * @code
     * (pixils.point/\* {:x 3 :y -4} 2)
     * => {:x 6 :y -8}
     * @endcode
     *
     * | Arg    | Description              |
     * | ------ | ------------------------ |
     * | point  | Point to scale.          |
     * | scalar | Numeric scale factor.    |
     *
     * @return A scaled `pixils.point`.
     *
     * @since 0.1.0
     */
    FUNC(PointMultiplication, multiply_num);

    /*!
     * @brief Divides both coordinates of a point by a scalar.
     *
     * Usage:
     * @code
     * (pixils.point/div {:x 6 :y -8} 2)
     * => {:x 3 :y -4}
     * @endcode
     *
     * | Arg     | Description             |
     * | ------- | ----------------------- |
     * | point   | Point to divide.        |
     * | divisor | Numeric divisor.        |
     *
     * @return A divided `pixils.point`.
     *
     * @since 0.1.0
     */
    FUNC(PointDivision, divide_num);

    /*!
     * @brief Tests whether two points have exactly equal x and y coordinates.
     *
     * Usage:
     * @code
     * (pixils.point/= {:x 10 :y 20} {:x 10 :y 20})
     * => true
     * @endcode
     *
     * | Arg | Description              |
     * | --- | ------------------------ |
     * | a   | First point to compare.  |
     * | b   | Second point to compare. |
     *
     * @return `true` when both coordinates are equal; otherwise `false`.
     *
     * @since 0.1.0
     */
    FUNC(PointEquality, equal);

    /*!
     * @brief Adds the corresponding coordinates of two points.
     *
     * Usage:
     * @code
     * (pixils.point/+ {:x 10 :y 20} {:x 3 :y -4})
     * => {:x 13 :y 16}
     * @endcode
     *
     * | Arg | Description          |
     * | --- | -------------------- |
     * | a   | First point to add.  |
     * | b   | Second point to add. |
     *
     * @return A `pixils.point` containing the coordinate-wise sum.
     *
     * @since 0.1.0
     */
    FUNC(PointPlus, plus);

    /*!
     * @brief Subtracts the coordinates of one point from another.
     *
     * Usage:
     * @code
     * (pixils.point/- {:x 10 :y 20} {:x 3 :y -4})
     * => {:x 7 :y 24}
     * @endcode
     *
     * | Arg | Description                |
     * | --- | -------------------------- |
     * | a   | Point to subtract from.    |
     * | b   | Point whose coordinates are subtracted. |
     *
     * @return A `pixils.point` containing the coordinate-wise difference.
     *
     * @since 0.1.0
     */
    FUNC(PointMinus, minus);

    /*!
     * Calculates the Chebyshev distance between two points: the greater of
     * their absolute x and y separations.
     *
     * Usage:
     * @code
     * (pixils.point/distance {:x 0 :y 0} {:x 3 :y 4})
     * => 4
     * @endcode
     *
     * | Arg | Description              |
     * | --- | ------------------------ |
     * | a   | First point.             |
     * | b   | Second point.            |
     *
     * @return The integer Chebyshev distance between the points.
     *
     * @since 0.1.0
     */
    FUNC(DistanceBetween, distance);

    /*!
     * @brief Calculates the squared Euclidean distance between two points
     * without taking a square root.
     *
     * Usage:
     * @code
     * (pixils.point/distance-squared {:x 0 :y 0} {:x 3 :y 4})
     * => 25
     * @endcode
     *
     * | Arg | Description              |
     * | --- | ------------------------ |
     * | a   | First point.             |
     * | b   | Second point.            |
     *
     * @return The squared Euclidean distance between the points.
     *
     * @since 0.1.0
     */
    FUNC(DistanceSquared, distance_squared);

    /*!
     * @brief Calculate the squared distance between two points.
     *
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
     * | Arg   | Description                                |
     * | ----- | ------------------------------------------ |
     * | point | Point to compare.                          |
     * | bound | Point or scalar upper bound for each axis. |
     *
     * @return A `pixils.point` with per-axis minimum values.
     *
     * @since 0.1.0
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
     * | Arg   | Description                                |
     * | ----- | ------------------------------------------ |
     * | point | Point to compare.                          |
     * | bound | Point or scalar lower bound for each axis. |
     *
     * @return A `pixils.point` with per-axis maximum values.
     *
     * @since 0.1.0
     */
    FUNC(PointMaximum, point_max);

    /*!
     * @brief Clamps a point to either a rect or explicit per-axis bounds.
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
     * | Arg   | Description                                  |
     * | ----- | -------------------------------------------- |
     * | point | Point to clamp.                              |
     * | rect  | Rect bounds for the two-argument form.       |
     * | min   | Point or scalar minimum bound for each axis. |
     * | max   | Point or scalar maximum bound for each axis. |
     *
     * @return A `pixils.point` clamped to the requested bounds.
     *
     * @since 0.1.0
     */
    FUNC(ClampPoint, clamp_rect, clamp_bounds);

    /*!
     * @brief Translates a point by separate x and y deltas.
     *
     * Usage:
     * @code
     * (pixils.point/translate {:x 10 :y 20} 3 -4)
     * => {:x 13 :y 16}
     * @endcode
     *
     * | Arg   | Description            |
     * | ----- | ---------------------- |
     * | point | Point to translate.    |
     * | dx    | Horizontal delta.      |
     * | dy    | Vertical delta.        |
     *
     * @return A translated `pixils.point`.
     *
     * @since 0.1.0
     */
    FUNC(TranslatePoint, translate);

    /*!
     * Translates a point along the x axis.
     *
     * Usage:
     * @code
     * (pixils.point/translate-x {:x 10 :y 20} -5)
     * => {:x 5 :y 20}
     * @endcode
     *
     * | Arg   | Description          |
     * | ----- | -------------------- |
     * | point | Point to translate.  |
     * | dx    | Horizontal delta.    |
     *
     * @return A horizontally translated `pixils.point`.
     *
     * @since 0.1.0
     */
    FUNC(TranslatePointX, translate_x);

    /*!
     * @brief Translates a point along the y axis.
     *
     * Usage:
     * @code
     * (pixils.point/translate-y {:x 10 :y 20} 7)
     * => {:x 10 :y 27}
     * @endcode
     *
     * | Arg   | Description          |
     * | ----- | -------------------- |
     * | point | Point to translate.  |
     * | dy    | Vertical delta.      |
     *
     * @return A vertically translated `pixils.point`.
     *
     * @since 0.1.0
     */
    FUNC(TranslatePointY, translate_y);

    /*!
     * Wraps coordinates outside a rect to the corresponding opposite edge.
     * Coordinates already inside the rect remain unchanged.
     *
     * Usage:
     * @code
     * (pixils.point/wrap {:x 111 :y 30} {:x 10 :y 20 :w 100 :h 50})
     * => {:x 10 :y 30}
     * @endcode
     *
     * | Arg   | Description             |
     * | ----- | ----------------------- |
     * | point | Point to wrap.          |
     * | rect  | Rect defining the wrap bounds. |
     *
     * @return A `pixils.point` wrapped to the rect bounds.
     *
     * @since 0.1.0
     */
    FUNC(WrapPoint, wrap);

    /*!
     * Rotates a point by radians around the origin or a supplied origin point.
     * The options form accepts `:radians` and an optional `:origin`.
     *
     * Usage:
     * @code
     * (pixils.point/rotate {:x 1 :y 0} 1.5707963)
     *
     * (pixils.point/rotate {:x 2 :y 1}
     *                      {:origin {:x 1 :y 1}
     *                       :radians 1.5707963})
     * @endcode
     *
     * | Arg     | Description                                      |
     * | ------- | ------------------------------------------------ |
     * | point   | Point to rotate.                                 |
     * | origin  | Point around which to rotate.                    |
     * | radians | Rotation angle in radians.                       |
     * | options | Map containing `:radians` and optional `:origin`. |
     *
     * @return A rotated `pixils.point`.
     *
     * @since 0.1.0
     */
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
