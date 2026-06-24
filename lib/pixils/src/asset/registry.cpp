
#include <pixils/asset/embedded_assets.h>
#include <pixils/asset/loader.h>
#include <pixils/asset/registry.h>
#include <pixils/runtime/mode.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <algorithm>
#include <stdexcept>

namespace Pixils::Asset
{
  namespace
  {
    SDL_Texture* duplicate_texture(Loader& loader, SDL_Surface* source)
    {
      return source ? loader.create_texture(source) : nullptr;
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

    SDL_Texture* create_tint_mask_texture(Loader& loader, SDL_Surface* source)
    {
      if (!source) return nullptr;

      SDL_Surface* mask = SDL_CreateRGBSurfaceWithFormat(0,
                                                         source->w,
                                                         source->h,
                                                         32,
                                                         SDL_PIXELFORMAT_RGBA8888);
      if (!mask) return duplicate_texture(loader, source);

      if (!source->format || !source->pixels || !mask->format || !mask->pixels ||
          source->w <= 0 || source->h <= 0)
      {
        SDL_FreeSurface(mask);
        return duplicate_texture(loader, source);
      }

      if (SDL_LockSurface(source) != 0)
      {
        SDL_FreeSurface(mask);
        return duplicate_texture(loader, source);
      }

      if (SDL_LockSurface(mask) != 0)
      {
        SDL_UnlockSurface(source);
        SDL_FreeSurface(mask);
        return duplicate_texture(loader, source);
      }

      auto* dst_pixels = static_cast<Uint32*>(mask->pixels);
      const int dst_stride = mask->pitch / static_cast<int>(sizeof(Uint32));
      const int pixel_count = source->w * source->h;
      if (pixel_count <= 0)
      {
        SDL_UnlockSurface(mask);
        SDL_UnlockSurface(source);
        SDL_FreeSurface(mask);
        return duplicate_texture(loader, source);
      }

      bool has_transparency = false;
      for (int y = 0; y < source->h && !has_transparency; y++)
      {
        for (int x = 0; x < source->w; x++)
        {
          Uint8 r, g, b, a;
          Uint32 pixel = read_surface_pixel(source, x, y);
          SDL_GetRGBA(pixel, source->format, &r, &g, &b, &a);
          if (a != 0xff)
          {
            has_transparency = true;
            break;
          }
        }
      }

      Uint8 bg_r = 0, bg_g = 0, bg_b = 0, bg_a = 0;
      SDL_GetRGBA(read_surface_pixel(source, 0, 0),
                  source->format,
                  &bg_r,
                  &bg_g,
                  &bg_b,
                  &bg_a);

      for (int y = 0; y < source->h; y++)
      {
        for (int x = 0; x < source->w; x++)
        {
          Uint8 r, g, b, a;
          Uint32 pixel = read_surface_pixel(source, x, y);
          SDL_GetRGBA(pixel, source->format, &r, &g, &b, &a);

          Uint8 out_alpha = 0;
          if (has_transparency)
          {
            out_alpha = a;
          }
          else if (!(r == bg_r && g == bg_g && b == bg_b && a == bg_a))
          {
            out_alpha = 0xff;
          }

          dst_pixels[(y * dst_stride) + x] =
            SDL_MapRGBA(mask->format, 0xff, 0xff, 0xff, out_alpha);
        }
      }

      SDL_UnlockSurface(mask);
      SDL_UnlockSurface(source);

      SDL_Texture* texture = loader.create_texture(mask);
      SDL_FreeSurface(mask);
      return texture;
    }

    void destroy_image_asset(Bundle& bundle, const std::string& asset_id)
    {
      auto image = bundle.images.find(asset_id);
      if (image != bundle.images.end())
      {
        if (image->second) SDL_DestroyTexture(image->second);
        bundle.images.erase(image);
      }

      auto source = bundle.image_sources.find(asset_id);
      if (source != bundle.image_sources.end())
      {
        if (source->second) SDL_FreeSurface(source->second);
        bundle.image_sources.erase(source);
      }

      auto tint_mask = bundle.tint_masks.find(asset_id);
      if (tint_mask != bundle.tint_masks.end())
      {
        if (tint_mask->second) SDL_DestroyTexture(tint_mask->second);
        bundle.tint_masks.erase(tint_mask);
      }
    }

    void upsert_image_dependency(Runtime::ResourceDependencies& deps,
                                 const Runtime::ImageDependency& dependency)
    {
      auto existing = std::find_if(deps.images.begin(),
                                   deps.images.end(),
                                   [&](const Runtime::ImageDependency& image)
                                   { return image.resource_id == dependency.resource_id; });
      if (existing == deps.images.end())
      {
        deps.images.push_back(dependency);
      }
      else
      {
        *existing = dependency;
      }
    }

    void erase_image_dependency(Runtime::ResourceDependencies& deps,
                                const std::string& resource_id)
    {
      auto& images = deps.images;
      images.erase(std::remove_if(images.begin(),
                                  images.end(),
                                  [&](const Runtime::ImageDependency& image)
                                  { return image.resource_id == resource_id; }),
                   images.end());
    }
  } // namespace

  Registry::Registry(RenderContext& ctx, std::string base_path)
    : loader(ctx, std::move(base_path))
  {
    embedded_fonts["pixils"]["autoega-8x14"] = &Assets::autoega_8x14_ttf;
  }

  Registry::~Registry()
  {
    for (auto& [_, record] : bundles)
    {
      auto& bundle = record.bundle;
      for (auto& [__, texture] : bundle.images)
      {
        if (texture) SDL_DestroyTexture(texture);
      }

      for (auto& [__, surface] : bundle.image_sources)
      {
        if (surface) SDL_FreeSurface(surface);
      }

      for (auto& [__, texture] : bundle.tint_masks)
      {
        if (texture) SDL_DestroyTexture(texture);
      }
    }
  }

  void Registry::load_embedded_assets()
  {
    if (this->is_loaded("pixils")) return;
    if (!SDL_WasInit(SDL_INIT_VIDEO)) return;
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) return;

    Bundle bundle;
    auto load_image = [&](const std::string& name, const Assets::EmbeddedAsset& asset)
    {
      if (auto* surface = loader.load_surface_from_memory(asset.data, asset.size))
      {
        bundle.images[name] = loader.create_texture(surface);
        bundle.image_sources[name] = surface;
      }
    };

    load_image("console-font", Assets::consolefont_png);
    load_image("pixils-logo", Assets::pixils_logo_png);
    load_image("win311-checkmark", Assets::win311_checkmark_png);
    load_image("win311-control-button", Assets::win311_control_button_png);
    load_image("win311-minimize-button", Assets::win311_minimize_button_png);
    load_image("win311-system-font", Assets::win311systemfont_png);
    load_image("win95-minimize-button", Assets::win95_minimize_button_png);
    load_image("win95-system-font", Assets::win95systemfont_png);

    bundles.emplace("pixils",
                    BundleRecord{.declaration = {},
                                 .bundle = std::move(bundle),
                                 .generated_images = {},
                                 .loaded = true,
                                 .mutable_bundle = false});
  }

  bool Registry::is_loaded(const std::string& bundle_id)
  {
    auto bundle = this->bundles.find(bundle_id);
    return bundle != this->bundles.end() && bundle->second.loaded;
  }

  bool Registry::is_dynamic_bundle(const std::string& bundle_id) const
  {
    auto bundle = this->bundles.find(bundle_id);
    if (bundle == this->bundles.end())
    {
      throw std::runtime_error("Unknown bundle: " + bundle_id);
    }
    return bundle->second.mutable_bundle;
  }

  void Registry::declare_bundle(const std::string& bundle_id,
                                const Runtime::ResourceDependencies& deps,
                                bool mutable_bundle)
  {
    auto existing = this->bundles.find(bundle_id);
    if (existing != this->bundles.end())
    {
      if (existing->second.mutable_bundle != mutable_bundle)
      {
        throw std::runtime_error("Bundle already declared with different mutability: " +
                                 bundle_id);
      }
      existing->second.declaration = deps;
      return;
    }

    this->bundles.emplace(bundle_id,
                          BundleRecord{.declaration = deps,
                                       .bundle = {},
                                       .generated_images = {},
                                       .loaded = false,
                                       .mutable_bundle = mutable_bundle});
  }

  void Registry::declare_dynamic_bundle(const std::string& bundle_id)
  {
    declare_bundle(bundle_id, Runtime::ResourceDependencies{}, true);
  }

  void Registry::create_dynamic_bundle(const std::string& bundle_id,
                                       const Runtime::ResourceDependencies& deps)
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      this->bundles.emplace(bundle_id,
                            BundleRecord{.declaration = deps,
                                         .bundle = {},
                                         .generated_images = {},
                                         .loaded = false,
                                         .mutable_bundle = true});
      return;
    }
    if (!record->second.mutable_bundle)
    {
      throw std::runtime_error("Bundle is not dynamic: " + bundle_id);
    }

    for (const auto& image : deps.images)
    {
      add_image(bundle_id, image);
    }
    if (!deps.sounds.empty()) record->second.declaration.sounds = deps.sounds;
    if (!deps.fonts.empty()) record->second.declaration.fonts = deps.fonts;
  }

  void Registry::add_image(const std::string& bundle_id,
                           const Runtime::ImageDependency& dependency)
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      throw std::runtime_error("Unknown bundle: " + bundle_id);
    }
    if (!record->second.mutable_bundle)
    {
      throw std::runtime_error("Bundle is not dynamic: " + bundle_id);
    }

    upsert_image_dependency(record->second.declaration, dependency);
    record->second.generated_images.erase(dependency.resource_id);
    if (!record->second.loaded) return;

    destroy_image_asset(record->second.bundle, dependency.resource_id);
    this->loader.load_image_asset(record->second.bundle, dependency);
  }

  void Registry::add_generated_image(const std::string& bundle_id,
                                     const std::string& resource_id,
                                     SDL_Texture* texture,
                                     SDL_Surface* surface,
                                     Dimension size)
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      throw std::runtime_error("Unknown bundle: " + bundle_id);
    }
    if (!record->second.mutable_bundle)
    {
      throw std::runtime_error("Bundle is not dynamic: " + bundle_id);
    }
    if (!texture)
    {
      throw std::runtime_error("Generated image texture is null: " + bundle_id + "/" +
                               resource_id);
    }
    if (!record->second.loaded)
    {
      record->second.bundle = this->loader.load_bundle_assets(record->second.declaration);
      record->second.loaded = true;
    }

    erase_image_dependency(record->second.declaration, resource_id);
    destroy_image_asset(record->second.bundle, resource_id);
    record->second.bundle.images.emplace(resource_id, texture);
    if (surface) record->second.bundle.image_sources.emplace(resource_id, surface);
    record->second.generated_images[resource_id] = size;
  }

  void Registry::remove_image(const std::string& bundle_id, const std::string& resource_id)
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      throw std::runtime_error("Unknown bundle: " + bundle_id);
    }
    if (!record->second.mutable_bundle)
    {
      throw std::runtime_error("Bundle is not dynamic: " + bundle_id);
    }

    erase_image_dependency(record->second.declaration, resource_id);
    record->second.generated_images.erase(resource_id);

    if (record->second.loaded)
    {
      destroy_image_asset(record->second.bundle, resource_id);
    }
  }

  std::vector<Runtime::ImageDependency> Registry::image_dependencies(
    const std::string& bundle_id) const
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      throw std::runtime_error("Unknown bundle: " + bundle_id);
    }
    return record->second.declaration.images;
  }

  std::unordered_map<std::string, Dimension> Registry::generated_image_sizes(
    const std::string& bundle_id) const
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      throw std::runtime_error("Unknown bundle: " + bundle_id);
    }
    return record->second.generated_images;
  }

  void Registry::load(const std::string& bundle_id,
                      const Runtime::ResourceDependencies& deps)
  {
    auto record = this->bundles.find(bundle_id);
    if (record == this->bundles.end())
    {
      this->bundles.emplace(bundle_id,
                            BundleRecord{.declaration = deps,
                                         .bundle = this->loader.load_bundle_assets(deps),
                                         .generated_images = {},
                                         .loaded = true,
                                         .mutable_bundle = false});
      return;
    }

    record->second.bundle = this->loader.load_bundle_assets(deps);
    record->second.loaded = true;
  }

  SDL_Texture* Registry::get_image(const std::string& bundle_id, const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id))
    {
      auto it = this->bundles.find(bundle_id);
      if (it == this->bundles.end()) return nullptr;
      this->load(bundle_id, it->second.declaration);
    }

    Bundle& bundle = this->bundles.at(bundle_id).bundle;

    if (!bundle.images.count(asset_id)) return nullptr;

    return bundle.images.at(asset_id);
  }

  SDL_Surface* Registry::get_image_surface(const std::string& bundle_id,
                                           const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id))
    {
      auto it = this->bundles.find(bundle_id);
      if (it == this->bundles.end()) return nullptr;
      this->load(bundle_id, it->second.declaration);
    }

    Bundle& bundle = this->bundles.at(bundle_id).bundle;

    auto source = bundle.image_sources.find(asset_id);
    if (source == bundle.image_sources.end()) return nullptr;
    return source->second;
  }

  SDL_Texture* Registry::get_tint_mask(const std::string& bundle_id,
                                       const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id))
    {
      auto it = this->bundles.find(bundle_id);
      if (it == this->bundles.end()) return nullptr;
      this->load(bundle_id, it->second.declaration);
    }

    Bundle& bundle = this->bundles.at(bundle_id).bundle;

    auto cached = bundle.tint_masks.find(asset_id);
    if (cached != bundle.tint_masks.end()) return cached->second;

    auto source = bundle.image_sources.find(asset_id);
    if (source == bundle.image_sources.end()) return nullptr;

    SDL_Texture* tint_mask = create_tint_mask_texture(loader, source->second);
    bundle.tint_masks.emplace(asset_id, tint_mask);
    return tint_mask;
  }

  Mix_Chunk* Registry::get_sound(const std::string& bundle_id, const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id))
    {
      auto it = this->bundles.find(bundle_id);
      if (it == this->bundles.end()) return nullptr;
      this->load(bundle_id, it->second.declaration);
    }

    Bundle& bundle = this->bundles.at(bundle_id).bundle;

    if (!bundle.sounds.count(asset_id)) return nullptr;

    return bundle.sounds.at(asset_id);
  }

  std::optional<std::string> Registry::get_font_path(const std::string& bundle_id,
                                                     const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id))
    {
      auto it = this->bundles.find(bundle_id);
      if (it == this->bundles.end()) return std::nullopt;
      this->load(bundle_id, it->second.declaration);
    }

    Bundle& bundle = this->bundles.at(bundle_id).bundle;

    auto font = bundle.fonts.find(asset_id);
    if (font == bundle.fonts.end()) return std::nullopt;

    return font->second;
  }

  const Assets::EmbeddedAsset* Registry::get_embedded_font(const std::string& bundle_id,
                                                           const std::string& asset_id)
  {
    auto embedded_bundle = embedded_fonts.find(bundle_id);
    if (embedded_bundle != embedded_fonts.end())
    {
      auto embedded_font = embedded_bundle->second.find(asset_id);
      if (embedded_font != embedded_bundle->second.end()) return embedded_font->second;
    }

    if (!this->is_loaded(bundle_id))
    {
      auto it = this->bundles.find(bundle_id);
      if (it == this->bundles.end()) return nullptr;
      this->load(bundle_id, it->second.declaration);
    }

    Bundle& bundle = this->bundles.at(bundle_id).bundle;

    auto font = bundle.embedded_fonts.find(asset_id);
    if (font == bundle.embedded_fonts.end()) return nullptr;

    return font->second;
  }
} // namespace Pixils::Asset
