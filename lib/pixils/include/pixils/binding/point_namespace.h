
#ifndef __PIXILS__POINT_NAMESPACE_H_
#define __PIXILS__POINT_NAMESPACE_H_

#include <pixils/geom.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils
{
  namespace Script
  {
    /*!
     * @brief Constant for "pixils.point" lisple namespace name
     */
    inline const std::string NS__PIXILS__POINT = "pixils.point";

    inline const std::string FN__MAKE_POINT = "point";
    inline const std::string FN__PIXILS__POINT__MAKE_POINT = "pixils.point/point";

    inline const std::string FN__DIVIDE = "div";
    inline const std::string FN__PLUS = "+";
    inline const std::string FN__MINUS = "-";
    inline const std::string FN__ROTATE = "rotate";
    inline const std::string FN__DISTANCE = "distance";

    namespace MapKey
    {
      DECL_SHKEY(X)
      DECL_SHKEY(Y)
    } // namespace MapKey

    namespace HostType
    {
      HOST_TYPE(POINT, "HPoint", FN__PIXILS__POINT__MAKE_POINT);

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
      FUNC_DECL(RotatePoint, rotate_point);

    } // namespace Function

    /*! @brief PointAdapter - A Lisple HostObject Adapter for Point */
    HOST_ADAPTER(PointAdapter, Point, (x, y), (x, y));

    class PointNamespace : public Lisple::Namespace
    {
    public:
      PointNamespace();
    };

  } // namespace Script
} // namespace Pixils

#endif /* __PIXILS__POINT_NAMESPACE_H_ */
