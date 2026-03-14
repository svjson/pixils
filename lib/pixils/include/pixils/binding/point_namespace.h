
#ifndef PIXILS__POINT_NAMESPACE_H
#define PIXILS__POINT_NAMESPACE_H

#include <pixils/geom.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils.point" lisple namespace name
   */
  inline constexpr std::string_view NS__PIXILS__POINT = "pixils.point";

  inline constexpr std::string_view FN__MAKE_POINT = "point";
  inline const std::string FN__PIXILS__POINT__MAKE_POINT = "pixils.point/point";

  inline constexpr std::string_view FN__DIVIDE = "div";
  inline constexpr std::string_view FN__PLUS = "+";
  inline constexpr std::string_view FN__MINUS = "-";
  inline constexpr std::string_view FN__ROTATE = "rotate";
  inline constexpr std::string_view FN__DISTANCE = "distance";

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
    /*! @brief Lisple Function that constructs a new instance of Point/PointAdapter */
    FUNC_DECL(MakePoint, make_point_from_ints, make_point_from_map);
    /*! @brief Divide point values  */
    FUNC_DECL(PointDivision, divide_num);
    /*! @brief Combines two points using addition */
    FUNC_DECL(PointPlus, plus);
    /*! @brief Combines two points using subtraction  */
    FUNC_DECL(PointMinus, minus);
    /*! @brief Distance between */
    FUNC_DECL(DistanceBetween, distance_between_points);
    /*! @brief Rotate point around point */
    FUNC_DECL(RotatePoint, rotate_point_orig_amount, rotate_point_amount,
              rotate_point_with_opts);
  } // namespace Function

  /*! @brief PointAdapter - A Lisple HostObject Adapter for Point */
  HOST_ADAPTER(PointAdapter, Point, (x, y), (x, y));

  class PointNamespace : public Lisple::Namespace
  {
  public:
    PointNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__POINT_NAMESPACE_H */
