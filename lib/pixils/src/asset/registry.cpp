
#include <pixils/asset/embedded_assets.h>
#include <pixils/asset/loader.h>
#include <pixils/asset/registry.h>
#include <pixils/runtime/mode.h>

namespace Pixils::Asset
{
  Registry::Registry(RenderContext& ctx, std::string base_path)
    : loader(ctx, std::move(base_path))
  {
  }

  void Registry::load_embedded_assets()
  {
    Bundle bundle;
    bundle.images["console-font"] = loader.load_texture_from_memory(
      Assets::consolefont_png.data, Assets::consolefont_png.size);
    bundle.images["pixils-logo"] = loader.load_texture_from_memory(
      Assets::pixils_logo_png.data, Assets::pixils_logo_png.size);
    bundles.emplace("pixils", std::move(bundle));
  }

  bool Registry::is_loaded(const std::string& bundle_id)
  {
    return this->bundles.count(bundle_id);
  }

  void Registry::declare_bundle(const std::string& bundle_id,
                                const Runtime::ResourceDependencies& deps)
  {
    this->declarations.emplace(bundle_id, deps);
  }

  void Registry::load(const std::string& bundle_id,
                      const Runtime::ResourceDependencies& deps)
  {
    Bundle bundle = this->loader.load_bundle_assets(deps);

    this->bundles.emplace(bundle_id, bundle);
  }

  SDL_Texture* Registry::get_image(const std::string& bundle_id, const std::string& asset_id)
  {
    if (!this->is_loaded(bundle_id))
    {
      auto it = this->declarations.find(bundle_id);
      if (it == this->declarations.end()) return nullptr;
      this->load(bundle_id, it->second);
    }

    Bundle& bundle = this->bundles.at(bundle_id);

    if (!bundle.images.count(asset_id)) return nullptr;

    return bundle.images.at(asset_id);
  }
} // namespace Pixils::Asset
