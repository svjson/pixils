
#ifndef PIXILS__POINT_NAMESPACE_H
#define PIXILS__POINT_NAMESPACE_H

#include <pixils/geom.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils.point" lisple namespace name
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
  inline constexpr std::string_view FN__CLAMP = "clamp";
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

    inline const Lisple::SeqRef VECTOR_OF_POINT(&Lisple::Type::ARRAY, &POINT, "[HPoint]");

  } // namespace HostType

  namespace Function
  {
    /*! @brief IntPointFunction */
    FUNC(IntPointFunction, int);
    /*! @brief Lisple Function that constructs a new instance of Point/PointAdapter */
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
    /*! @brief Clamp point to rect bounds */
    FUNC(ClampPoint, clamp);
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

  /*! @brief PointAdapter - A Lisple HostObject Adapter for Point */
  NATIVE_ADAPTER(PointAdapter, Point, (x, y), (x, y));

  class PointNamespace : public Lisple::Namespace
  {
   public:
    PointNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__POINT_NAMESPACE_H */
