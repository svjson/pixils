
#include <pixils/asset/loader.h>
#include <pixils/asset/registry.h>
#include <pixils/runtime/mode.h>

namespace Pixils::Asset
{
  Registry::Registry(RenderContext& ctx)
    : loader(ctx)
  {
  }

  bool Registry::is_loaded(const std::string& bundle_id)
  {
    return this->bundles.count(bundle_id);
  }

  void Registry::load(const std::string& bundle_id,
                      const Runtime::ResourceDependencies& deps)
  {
    Bundle bundle = this->loader.load_bundle_assets(deps);

    this->bundles.emplace(bundle_id, bundle);
  }

  SDL_Texture* Registry::get_image(const std::string& bundle_id, const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id)) return nullptr;

    Bundle& bundle = this->bundles.at(bundle_id);

    if (!bundle.images.count(asset_id)) return nullptr;

    return bundle.images.at(asset_id);
  }
} // namespace Pixils::Asset
