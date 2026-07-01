#include <pixils/binding/line_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/polygon_namespace.h>

namespace Pixils::Script
{
  namespace Function
  {
    FUNC_IMPL(LineClosestPointFunction,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&LineClosestPointFunction::exec_closest_point))));

    EXEC_BODY(LineClosestPointFunction, exec_closest_point)
    {
      return PointAdapter::make_unique(
        Geometry::segment_closest_point(Roo::obj<Point>(*args[0]),
                                        Roo::obj<Point>(*args[1]),
                                        Roo::obj<Point>(*args[2])));
    }
  } // namespace Function

  LineNamespace::LineNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__LINE))
  {
    values.emplace("closest-point", Function::LineClosestPointFunction::make());
  }
} // namespace Pixils::Script
