#include <pixils/asset/registry.h>
#include <pixils/binding/image_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/context.h>
#include <pixils/geom.h>

#include <SDL2/SDL_render.h>
#include <optional>

namespace Pixils::Script
{
  namespace Function
  {
    namespace
    {
      std::optional<Dimension> image_size(Lisple::Context& ctx,
                                          const Lisple::sptr_rtval& image_key)
      {
        if (!image_key || image_key->type != Lisple::RTValue::Type::KEYWORD)
        {
          return std::nullopt;
        }

        auto [bundle_id, asset_id] = image_key->qual();

        RenderContext& rc =
          Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

        SDL_Texture* texture = rc.asset_registry->get_image(bundle_id, asset_id);
        if (!texture) return std::nullopt;

        int width = 0;
        int height = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
        return Dimension{width, height};
      }
    } // namespace

    FUNC_IMPL(ImageSize,
              SIG((FN_ARGS((&Lisple::Type::KEY)), EXEC_DISPATCH(&ImageSize::exec_size))));

    EXEC_BODY(ImageSize, exec_size)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;
      return DimensionAdapter::make_unique(size->w, size->h);
    }

    FUNC_IMPL(ImageWidth,
              SIG((FN_ARGS((&Lisple::Type::KEY)), EXEC_DISPATCH(&ImageWidth::exec_width))));

    EXEC_BODY(ImageWidth, exec_width)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;
      return Lisple::RTValue::number(size->w);
    }

    FUNC_IMPL(ImageHeight,
              SIG((FN_ARGS((&Lisple::Type::KEY)),
                   EXEC_DISPATCH(&ImageHeight::exec_height))));

    EXEC_BODY(ImageHeight, exec_height)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;
      return Lisple::RTValue::number(size->h);
    }

    FUNC_IMPL(ImageRect,
              MULTI_SIG((FN_ARGS((&Lisple::Type::KEY)),
                         EXEC_DISPATCH(&ImageRect::exec_rect)),
                        (FN_ARGS((&Lisple::Type::KEY), (&HostType::POINT)),
                         EXEC_DISPATCH(&ImageRect::exec_rect_with_offset))));

    EXEC_BODY(ImageRect, exec_rect)
    {
      Lisple::sptr_rtval_v fwd_args = {
        args[0],
        PointAdapter::make_unique(POINT__ZERO_ZERO.x, POINT__ZERO_ZERO.y)};
      return this->exec_rect_with_offset(ctx, fwd_args);
    }

    EXEC_BODY(ImageRect, exec_rect_with_offset)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;

      const Point& offset = Lisple::obj<Point>(*args[1]);
      return RectAdapter::make_unique(offset.round_x(), offset.round_y(), size->w, size->h);
    }

  } // namespace Function

  ImageNamespace::ImageNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__IMAGE))
  {
    values.emplace(FN__HEIGHT, Function::ImageHeight::make());
    values.emplace(FN__RECT, Function::ImageRect::make());
    values.emplace(FN__SIZE, Function::ImageSize::make());
    values.emplace(FN__WIDTH, Function::ImageWidth::make());
  }
} // namespace Pixils::Script
