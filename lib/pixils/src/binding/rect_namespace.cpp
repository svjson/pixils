
#include "pixils/binding/rect_namespace.h"

#include "pixils/binding/point_namespace.h"
#include <pixils/binding/polygon_namespace.h>
#include <pixils/geom.h>

#include <roo/exec.h>
#include <roo/host/schema.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace
  {
    bool include_boundary_option(Roo::Context& ctx, const Roo::sptr_val& value)
    {
      static Roo::MapSchema opts_schema({}, {{"include-boundary?", &Roo::Type::BOOL}});
      auto opts = opts_schema.bind(ctx, *value);
      return opts.boolean("include-boundary?", false);
    }
  } // namespace

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

    /** RectContainsFunction - contains? */
    FUNC_IMPL(RectContainsFunction,
              MULTI_SIG((FN_ARGS((&HostType::RECT), (&HostType::RECT)),
                         EXEC_DISPATCH(&RectContainsFunction::exec_contains_rect)),
                        (FN_ARGS((&HostType::RECT), (&HostType::POINT)),
                         EXEC_DISPATCH(&RectContainsFunction::exec_contains_point)),
                        (FN_ARGS((&HostType::RECT), (&HostType::VECTOR_OF_POINT)),
                         EXEC_DISPATCH(&RectContainsFunction::exec_contains_polygon))));

    EXEC_BODY(RectContainsFunction, exec_contains_point)
    {
      return Roo::obj<Rect>(*args[0]).contains(Roo::obj<Point>(*args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    EXEC_BODY(RectContainsFunction, exec_contains_rect)
    {
      return Geometry::rect_contains_rect(Roo::obj<Rect>(*args[0]), Roo::obj<Rect>(*args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    EXEC_BODY(RectContainsFunction, exec_contains_polygon)
    {
      return Geometry::rect_contains_polygon(Roo::obj<Rect>(*args[0]),
                                             Geometry::points_from_value(args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    /** RectIntersectsFunction - intersects? */
    FUNC_IMPL(RectIntersectsFunction,
              MULTI_SIG((FN_ARGS((&HostType::RECT), (&HostType::RECT)),
                         EXEC_DISPATCH(&RectIntersectsFunction::exec_intersects_rect)),
                        (FN_ARGS((&HostType::RECT), (&HostType::RECT), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&RectIntersectsFunction::exec_intersects_with_opts)),
                        (FN_ARGS((&HostType::RECT), (&HostType::VECTOR_OF_POINT)),
                         EXEC_DISPATCH(&RectIntersectsFunction::exec_intersects_polygon))));

    EXEC_BODY(RectIntersectsFunction, exec_intersects_rect)
    {
      return Roo::obj<Rect>(*args[0]).intersects(Roo::obj<Rect>(*args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    EXEC_BODY(RectIntersectsFunction, exec_intersects_with_opts)
    {
      const Rect& rect = Roo::obj<Rect>(*args[0]);
      const bool include_boundary = include_boundary_option(ctx, args[2]);
      return Geometry::rect_intersects_rect(rect, Roo::obj<Rect>(*args[1]), include_boundary)
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    EXEC_BODY(RectIntersectsFunction, exec_intersects_polygon)
    {
      return Geometry::rect_intersects_polygon(Roo::obj<Rect>(*args[0]),
                                               Geometry::points_from_value(args[1]))
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
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
    values.emplace("contains?", Function::RectContainsFunction::make());
    values.emplace("inside?", Function::InsidePFunction::make());
    values.emplace("intersect?", Function::IntersectPFunction::make());
    values.emplace("intersects?", Function::RectIntersectsFunction::make());
    values.emplace("make-rect", Function::MakeRect::make());
  }

} // namespace Pixils::Script
