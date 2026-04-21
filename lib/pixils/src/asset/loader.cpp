
#include "pixils/asset/loader.h"

#include <pixils/runtime/mode.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <filesystem>

namespace Pixils::Asset
{
  Loader::Loader(RenderContext& ctx, std::string base_path)
    : ctx(ctx)
    , base_path(std::move(base_path))
  {
  }

  Bundle Loader::load_bundle_assets(const Runtime::ResourceDependencies& deps)
  {
    Bundle bundle;

    for (auto& img_dep : deps.images)
    {
      std::string resolved = img_dep.file_name;
      if (!base_path.empty() && !std::filesystem::path(resolved).is_absolute())
        resolved = base_path + "/" + resolved;

      SDL_Texture* texture = nullptr;
      SDL_Surface* img_surface = IMG_Load(resolved.c_str());
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
  SDL_Texture* Loader::load_texture_from_memory(const unsigned char* data, std::size_t size)
  {
    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(size));
    if (!rw) return nullptr;

    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(ctx.renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
  }
} // namespace Pixils::Asset
