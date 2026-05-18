#ifndef PIXILS__ASSET__LOADER_H
#define PIXILS__ASSET__LOADER_H

#include <pixils/asset/bundle.h>
#include <pixils/context.h>
#include <pixils/runtime/session.h>

#include <cstddef>
#include <string>

struct Mix_Chunk;
struct SDL_Surface;
struct SDL_Texture;

namespace Pixils::Runtime
{
  struct ImageDependency;
  struct ResourceDependencies;
}

namespace Pixils::Asset
{
  class Loader
  {
    RenderContext& ctx;
    std::string base_path;

   public:
    Loader(RenderContext& ctx, std::string base_path = "");
    Bundle load_bundle_assets(const Runtime::ResourceDependencies& deps);
    void load_image_asset(Bundle& bundle, const Runtime::ImageDependency& dependency);
    SDL_Texture* create_texture(SDL_Surface* surface);
    SDL_Surface* load_surface_from_memory(const unsigned char* data, std::size_t size);
    SDL_Texture* load_texture_from_memory(const unsigned char* data, std::size_t size);
    Mix_Chunk* load_sound_from_file(const std::string& file_name);
  };

} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__LOADER_H */
