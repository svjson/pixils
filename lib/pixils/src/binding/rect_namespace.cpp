
#include "pixils/binding/rect_namespace.h"

#include "pixils/binding/point_namespace.h"
#include <pixils/binding/polygon_namespace.h>
#include <pixils/geom.h>

#include <cmath>
#include <roo/exception.h>
#include <roo/exec.h>
#include <roo/host/schema.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace
  {
    struct AlignmentAnchor
    {
      float x;
      float y;
    };

    AlignmentAnchor parse_alignment_anchor(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL)
      {
        throw Roo::TypeError("Alignment anchor cannot be nil");
      }

      if (value->type != Roo::Value::Type::KEYWORD)
      {
        throw Roo::TypeError("Alignment anchor must be a keyword");
      }

      const std::string anchor = value->str();
      if (anchor == "top-left") return {0.0f, 0.0f};
      if (anchor == "top-center") return {0.5f, 0.0f};
      if (anchor == "top-right") return {1.0f, 0.0f};
      if (anchor == "center-left") return {0.0f, 0.5f};
      if (anchor == "center") return {0.5f, 0.5f};
      if (anchor == "center-right") return {1.0f, 0.5f};
      if (anchor == "bottom-left") return {0.0f, 1.0f};
      if (anchor == "bottom-center") return {0.5f, 1.0f};
      if (anchor == "bottom-right") return {1.0f, 1.0f};

      throw Roo::TypeError("Unknown alignment anchor: " + value->to_string());
    }

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

    // pixils.rect/align
    FUNC_IMPL(
      RectAlignFunction,
      MULTI_SIG((FN_ARGS((&HostType::RECT),
                         (&Roo::Type::MAP),
                         (&Roo::Type::NUMBER),
                         (&Roo::Type::NUMBER)),
                 EXEC_DISPATCH(&RectAlignFunction::exec_align_numeric)),
                (FN_ARGS((&HostType::RECT), (&Roo::Type::MAP), (&Roo::Type::KEYWORD)),
                 EXEC_DISPATCH(&RectAlignFunction::exec_align_anchor)),
                (FN_ARGS((&HostType::RECT), (&Roo::Type::MAP), (&Roo::Type::MAP)),
                 EXEC_DISPATCH(&RectAlignFunction::exec_align_options))));

    static Roo::sptr_val make_aligned_rect(Roo::Context& ctx,
                                           const Roo::sptr_val& rect_value,
                                           const Roo::sptr_val& size_value,
                                           AlignmentAnchor at,
                                           AlignmentAnchor origin)
    {
      static Roo::MapSchema size_schema(
        {{"w", &Roo::Type::NUMBER}, {"h", &Roo::Type::NUMBER}});

      const Rect& rect = Roo::obj<Rect>(*rect_value);
      const auto size = size_schema.bind(ctx, *size_value);
      const int width = size.i32("w");
      const int height = size.i32("h");
      const float x = static_cast<float>(rect.x) + static_cast<float>(rect.w) * at.x -
                      static_cast<float>(width) * origin.x;
      const float y = static_cast<float>(rect.y) + static_cast<float>(rect.h) * at.y -
                      static_cast<float>(height) * origin.y;

      return RectAdapter::make_unique(static_cast<int>(std::floor(x)),
                                      static_cast<int>(std::floor(y)),
                                      width,
                                      height);
    }

    EXEC_BODY(RectAlignFunction, exec_align_numeric)
    {
      const AlignmentAnchor anchor = {args[2]->f32(), args[3]->f32()};
      return make_aligned_rect(ctx, args[0], args[1], anchor, anchor);
    }

    EXEC_BODY(RectAlignFunction, exec_align_anchor)
    {
      const AlignmentAnchor anchor = parse_alignment_anchor(args[2]);
      return make_aligned_rect(ctx, args[0], args[1], anchor, anchor);
    }

    EXEC_BODY(RectAlignFunction, exec_align_options)
    {
      static Roo::MapSchema options_schema({{"at", &Roo::Type::KEYWORD}},
                                           {{"origin", &Roo::Type::KEYWORD}});

      const auto options = options_schema.bind(ctx, *args[2]);
      const AlignmentAnchor at = parse_alignment_anchor(options.val("at"));
      const Roo::sptr_val origin_value = options.val("origin");
      const AlignmentAnchor origin =
        (!origin_value || origin_value->type == Roo::Value::Type::NIL)
          ? at
          : parse_alignment_anchor(origin_value);

      return make_aligned_rect(ctx, args[0], args[1], at, origin);
    }

    /** RectNormalizePointFunction - pixils.rect/normalize-point */
    FUNC_IMPL(RectNormalizePointFunction,
              SIG((FN_ARGS((&HostType::RECT), (&Roo::Type::NUMBER), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&RectNormalizePointFunction::exec_normalize_point))));

    EXEC_BODY(RectNormalizePointFunction, exec_normalize_point)
    {
      const Rect& rect = Roo::obj<Rect>(*args[0]);
      const float x =
        static_cast<float>(rect.x) + static_cast<float>(rect.w) * args[1]->f32();
      const float y =
        static_cast<float>(rect.y) + static_cast<float>(rect.h) * args[2]->f32();
      return PointAdapter::make_unique(x, y);
    }

  } // namespace Function

  RectNamespace::RectNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__RECT))
  {
    values.emplace("contains?", Function::RectContainsFunction::make());
    values.emplace("inside?", Function::InsidePFunction::make());
    values.emplace("intersect?", Function::IntersectPFunction::make());
    values.emplace("intersects?", Function::RectIntersectsFunction::make());
    values.emplace(FN__ALIGN, Function::RectAlignFunction::make());
    values.emplace("make-rect", Function::MakeRect::make());
    values.emplace(FN__NORMALIZE_POINT, Function::RectNormalizePointFunction::make());
  }

} // namespace Pixils::Script
