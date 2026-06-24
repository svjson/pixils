#ifndef PIXILS__ASSET__REGISTRY_H
#define PIXILS__ASSET__REGISTRY_H

#include <pixils/asset/bundle.h>
#include <pixils/asset/loader.h>
#include <pixils/context.h>
#include <pixils/geom.h>
#include <pixils/runtime/mode.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Mix_Chunk;
struct SDL_Texture;
struct SDL_Surface;

namespace Pixils::Asset
{
  class Registry
  {
    struct BundleRecord
    {
      Runtime::ResourceDependencies declaration;
      Bundle bundle;
      std::unordered_map<std::string, Dimension> generated_images;
      bool loaded = false;
      bool mutable_bundle = false;
    };

    Loader loader;

    std::unordered_map<std::string, BundleRecord> bundles;
    std::unordered_map<std::string,
                       std::unordered_map<std::string, const Assets::EmbeddedAsset*>>
      embedded_fonts;

   public:
    Registry(RenderContext& ctx, std::string base_path = "");
    ~Registry();

    bool is_loaded(const std::string& bundle_id);
    bool is_dynamic_bundle(const std::string& bundle_id) const;

    /** Register a bundle's resource dependencies without loading them. The
     *  bundle is loaded on demand the first time get_image is called for it. */
    void declare_bundle(const std::string& bundle_id,
                        const Runtime::ResourceDependencies& deps,
                        bool mutable_bundle = false);
    void declare_dynamic_bundle(const std::string& bundle_id);
    void create_dynamic_bundle(const std::string& bundle_id,
                               const Runtime::ResourceDependencies& deps = {});
    void add_image(const std::string& bundle_id, const Runtime::ImageDependency& dependency);
    void add_generated_image(const std::string& bundle_id,
                             const std::string& resource_id,
                             SDL_Texture* texture,
                             SDL_Surface* surface,
                             Dimension size);
    void remove_image(const std::string& bundle_id, const std::string& resource_id);
    std::vector<Runtime::ImageDependency> image_dependencies(
      const std::string& bundle_id) const;
    std::unordered_map<std::string, Dimension> generated_image_sizes(
      const std::string& bundle_id) const;

    void load_embedded_assets();
    void load(const std::string& bundle_id, const Runtime::ResourceDependencies& deps);

    SDL_Texture* get_image(const std::string& bundle, const std::string& asset_id);
    SDL_Surface* get_image_surface(const std::string& bundle, const std::string& asset_id);
    SDL_Texture* get_tint_mask(const std::string& bundle, const std::string& asset_id);
    Mix_Chunk* get_sound(const std::string& bundle, const std::string& asset_id);
    std::optional<std::string> get_font_path(const std::string& bundle,
                                             const std::string& asset_id);
    const Assets::EmbeddedAsset* get_embedded_font(const std::string& bundle,
                                                   const std::string& asset_id);
  };
} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__REGISTRY_H */
