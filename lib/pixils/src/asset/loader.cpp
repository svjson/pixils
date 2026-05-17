
#include "pixils/asset/loader.h"

#include <pixils/runtime/mode.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_surface.h>
#include <filesystem>
#include <optional>

namespace Pixils::Asset
{
  namespace
  {
    std::string resolve_asset_path(const std::string& base_path,
                                   const std::string& file_name)
    {
      if (!base_path.empty() && !std::filesystem::path(file_name).is_absolute())
      {
        return base_path + "/" + file_name;
      }

      return file_name;
    }

    void apply_transparency_color(SDL_Surface* surface,
                                  const std::optional<Color>& transparency_color)
    {
      if (!surface || !transparency_color) return;
      if (!surface->format) return;

      const Color& color = *transparency_color;
      Uint32 color_key = SDL_MapRGB(surface->format, color.r, color.g, color.b);
      SDL_SetColorKey(surface, SDL_TRUE, color_key);
    }
  } // namespace

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
      SDL_Texture* texture = nullptr;
      std::string resolved = resolve_asset_path(base_path, img_dep.file_name);
      SDL_Surface* img_surface = IMG_Load(resolved.c_str());
      if (img_surface)
      {
        apply_transparency_color(img_surface, img_dep.transparency_color);
        texture = create_texture(img_surface);
        bundle.image_sources.emplace(img_dep.resource_id, img_surface);
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

    for (auto& sound_dep : deps.sounds)
    {
      bundle.sounds.emplace(sound_dep.resource_id,
                            load_sound_from_file(sound_dep.file_name));
    }

    for (auto& font_dep : deps.fonts)
    {
      bundle.fonts.emplace(font_dep.resource_id,
                           resolve_asset_path(base_path, font_dep.file_name));
    }

    return bundle;
  }

  SDL_Texture* Loader::create_texture(SDL_Surface* surface)
  {
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(ctx.renderer, surface);
    if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
  }

  SDL_Surface* Loader::load_surface_from_memory(const unsigned char* data, std::size_t size)
  {
    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(size));
    if (!rw) return nullptr;

    return IMG_Load_RW(rw, 1);
  }

  SDL_Texture* Loader::load_texture_from_memory(const unsigned char* data, std::size_t size)
  {
    SDL_Surface* surface = load_surface_from_memory(data, size);
    if (!surface) return nullptr;

    SDL_Texture* texture = create_texture(surface);
    SDL_FreeSurface(surface);
    return texture;
  }

  Mix_Chunk* Loader::load_sound_from_file(const std::string& file_name)
  {
    std::string resolved = resolve_asset_path(base_path, file_name);
    return Mix_LoadWAV(resolved.c_str());
  }
} // namespace Pixils::Asset
