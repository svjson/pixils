
#include "pixils/binding/rect_namespace.h"

#include "pixils/binding/point_namespace.h"
#include <pixils/geom.h>

#include <roo/exec.h>
#include <roo/host/schema.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace Function
  {
    FUNC_IMPL(MakeRect,
              SIG((FN_ARGS((&Roo::Type::MAP)), EXEC_DISPATCH(&MakeRect::exec_make))))

    EXEC_BODY(MakeRect, exec_make)
    {
      static Roo::MapSchema rect_schema({{"x", &Roo::Type::NUMBER},
                                            {"y", &Roo::Type::NUMBER},
                                            {"w", &Roo::Type::NUMBER},
                                            {"h", &Roo::Type::NUMBER}});

      auto opts = rect_schema.bind(ctx, *args[0]);

      return RectAdapter::make_unique(opts.i32("x"),
                                      opts.i32("y"),
                                      opts.i32("w"),
                                      opts.i32("h"));
    }

    /** InsidePFunction - inside? */
    FUNC_IMPL(InsidePFunction,
              SIG((FN_ARGS((&HostType::RECT), (&HostType::POINT)),
                   EXEC_DISPATCH(&InsidePFunction::exec_inside))))

    EXEC_BODY(InsidePFunction, exec_inside)
    {
      return Roo::obj<Rect>(*args[0]).contains(Roo::obj<Point>(*args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    /** IntersectPFunction - intersect? */
    FUNC_IMPL(IntersectPFunction,
              SIG((FN_ARGS((&HostType::RECT), (&HostType::RECT)),
                   EXEC_DISPATCH(&IntersectPFunction::exec_intersect))))

    EXEC_BODY(IntersectPFunction, exec_intersect)
    {
      return Roo::obj<Rect>(*args[0]).intersects(Roo::obj<Rect>(*args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

  } // namespace Function

  RectNamespace::RectNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__RECT))
  {
    values.emplace("inside?", Function::InsidePFunction::make());
    values.emplace("intersect?", Function::IntersectPFunction::make());
    values.emplace("make-rect", Function::MakeRect::make());
  }

} // namespace Pixils::Script
