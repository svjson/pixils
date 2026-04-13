#ifndef PIXILS__ASSET__REGISTRY_H
#define PIXILS__ASSET__REGISTRY_H

#include <pixils/asset/bundle.h>
#include <pixils/asset/loader.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>

#include <string>
#include <unordered_map>

namespace Pixils::Asset
{
  class Registry
  {
    Loader loader;

    std::unordered_map<std::string, Bundle> bundles;

   public:
    Registry(RenderContext& ctx);

    bool is_loaded(const std::string& bundle_id);

    void load_embedded_assets();
    void load(const std::string& bundle_id, const Runtime::ResourceDependencies& deps);

    SDL_Texture* get_image(const std::string& bundle, const std::string& asset_id);
  };
} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__REGISTRY_H */
