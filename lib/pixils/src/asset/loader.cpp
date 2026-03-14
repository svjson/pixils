
#include "pixils/runtime/mode.h"
#include <pixils/asset/loader.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>

namespace Pixils::Asset
{
  Loader::Loader(RenderContext& ctx)
    : ctx(ctx)
  {
  }

  Bundle Loader::load_bundle_assets(const Runtime::ResourceDependencies& deps)
  {
    Bundle bundle;

    for (auto& img_dep : deps.images)
    {
      SDL_Texture* texture = nullptr;
      SDL_Surface* img_surface = IMG_Load(img_dep.file_name.c_str());
      if (img_surface)
      {
        texture = SDL_CreateTextureFromSurface(ctx.renderer, img_surface);
        SDL_FreeSurface(img_surface);
      }
      else
      {
        texture = SDL_CreateTexture(ctx.renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_STATIC,
                                    16,
                                    16);
      }

      bundle.images.emplace(img_dep.resource_id, texture);
    }

    return bundle;
  }
} // namespace Pixils::Asset
