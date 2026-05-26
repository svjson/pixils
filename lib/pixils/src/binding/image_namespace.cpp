#include <pixils/asset/registry.h>
#include <pixils/binding/image_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/context.h>
#include <pixils/geom.h>

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <lisple/runtime/dict.h>
#include <optional>

namespace Pixils::Script
{
  namespace Function
  {
    namespace
    {
      std::optional<Dimension> image_size(Lisple::Context& ctx,
                                          const Lisple::sptr_val& image_key)
      {
        if (!image_key || image_key->type != Lisple::Value::Type::KEYWORD)
        {
          return std::nullopt;
        }

        auto [bundle_id, asset_id] = image_key->qual();

        RenderContext& rc =
          Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

        SDL_Texture* texture = rc.asset_registry->get_image(bundle_id, asset_id);
        if (!texture) return std::nullopt;

        int width = 0;
        int height = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
        return Dimension{width, height};
      }

      Uint32 read_surface_pixel(SDL_Surface* surface, int x, int y)
      {
        const int bpp = surface->format->BytesPerPixel;
        auto* row = static_cast<Uint8*>(surface->pixels) + (y * surface->pitch);
        Uint8* pixel = row + (x * bpp);

        switch (bpp)
        {
        case 1:
          return *pixel;
        case 2:
          return *reinterpret_cast<Uint16*>(pixel);
        case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
          return (pixel[0] << 16) | (pixel[1] << 8) | pixel[2];
#else
          return pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
#endif
        case 4:
          return *reinterpret_cast<Uint32*>(pixel);
        default:
          return 0;
        }
      }

      Lisple::sptr_val color_map(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
      {
        auto result = Lisple::map({});
        Lisple::Dict::set_property(result, Lisple::keyword("r"), Lisple::number(r));
        Lisple::Dict::set_property(result, Lisple::keyword("g"), Lisple::number(g));
        Lisple::Dict::set_property(result, Lisple::keyword("b"), Lisple::number(b));
        Lisple::Dict::set_property(result, Lisple::keyword("a"), Lisple::number(a));
        return result;
      }
    } // namespace

    FUNC_IMPL(ImageColorAt,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD), (&HostType::POINT)),
                   EXEC_DISPATCH(&ImageColorAt::exec_color_at))));

    EXEC_BODY(ImageColorAt, exec_color_at)
    {
      auto [bundle_id, asset_id] = args[0]->qual();
      if (bundle_id.empty() || asset_id.empty()) return Lisple::Constant::NIL;

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      SDL_Surface* surface = rc.asset_registry->get_image_surface(bundle_id, asset_id);
      if (!surface || !surface->format || !surface->pixels) return Lisple::Constant::NIL;

      const Point& point = Lisple::obj<Point>(*args[1]);
      const int x = point.round_x();
      const int y = point.round_y();
      if (x < 0 || y < 0 || x >= surface->w || y >= surface->h)
        return Lisple::Constant::NIL;

      if (SDL_LockSurface(surface) != 0) return Lisple::Constant::NIL;

      Uint8 r, g, b, a;
      Uint32 pixel = read_surface_pixel(surface, x, y);
      SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

      SDL_UnlockSurface(surface);
      return color_map(r, g, b, a);
    }

    FUNC_IMPL(ImageSize,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&ImageSize::exec_size))));

    EXEC_BODY(ImageSize, exec_size)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;
      return DimensionAdapter::make_unique(size->w, size->h);
    }

    FUNC_IMPL(ImageWidth,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&ImageWidth::exec_width))));

    EXEC_BODY(ImageWidth, exec_width)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;
      return Lisple::number(size->w);
    }

    FUNC_IMPL(ImageHeight,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&ImageHeight::exec_height))));

    EXEC_BODY(ImageHeight, exec_height)
    {
      auto size = image_size(ctx, args[0]);
      if (!size) return Lisple::Constant::NIL;
      return Lisple::number(size->h);
    }

    FUNC_IMPL(ImageRect,
              MULTI_SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                         EXEC_DISPATCH(&ImageRect::exec_rect)),
                        (FN_ARGS((&Lisple::Type::KEYWORD), (&HostType::POINT)),
                         EXEC_DISPATCH(&ImageRect::exec_rect_with_offset))));

    EXEC_BODY(ImageRect, exec_rect)
    {
      Lisple::sptr_val_v fwd_args = {
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
    values.emplace(FN__COLOR_AT, Function::ImageColorAt::make());
    values.emplace(FN__HEIGHT, Function::ImageHeight::make());
    values.emplace(FN__RECT, Function::ImageRect::make());
    values.emplace(FN__SIZE, Function::ImageSize::make());
    values.emplace(FN__WIDTH, Function::ImageWidth::make());
  }
} // namespace Pixils::Script
