#ifndef PIXILS__ASSET__LOADER_H
#define PIXILS__ASSET__LOADER_H

#include <pixils/asset/bundle.h>
#include <pixils/context.h>
#include <pixils/runtime/session.h>

#include <cstddef>
#include <string>

struct Mix_Chunk;

namespace Pixils::Runtime
{
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
    SDL_Texture* load_texture_from_memory(const unsigned char* data, std::size_t size);
    Mix_Chunk* load_sound_from_file(const std::string& file_name);
  };

} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__LOADER_H */
