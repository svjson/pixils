
#include "pixils/runtime/mode.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/context.h>
#include <pixils/geom.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <lisple/exception.h>
#include <lisple/host/accessor.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>
#include <memory>
#include <stdexcept>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(FILE_NAME, "file-name");
    SHKEY(FONTS, "fonts");
    SHKEY(ID, "id");
    SHKEY(IMAGES, "images");
    SHKEY(SIZE, "size");
    SHKEY(SOUNDS, "sounds");
    SHKEY(RESOURCE_SOURCE, "source");
    SHKEY(TRANSPARENCY_COLOR, "transparency-color");
  } // namespace MapKey

  namespace
  {
    struct RenderTargetGuard
    {
      RenderContext& rc;
      SDL_Texture* previous_target;

      RenderTargetGuard(RenderContext& rc, SDL_Texture* target)
        : rc(rc)
        , previous_target(rc.current_render_target)
      {
        rc.set_render_target(target);
      }

      ~RenderTargetGuard() { rc.set_render_target(previous_target); }
    };

    Runtime::ImageDependency parse_image_dependency(Lisple::Context& ctx,
                                                    const std::string& resource_id,
                                                    const Lisple::sptr_val& value)
    {
      if (value->type == Lisple::Value::Type::STRING)
      {
        return Runtime::ImageDependency{resource_id, value->str()};
      }

      if (value->type != Lisple::Value::Type::MAP)
      {
        throw Lisple::TypeError("Image resource dependency must be a file name string "
                                "or a map");
      }

      static Lisple::MapSchema image_schema({{"file-name", &Lisple::Type::STRING}},
                                            {{"transparency-color", &HostType::COLOR}});

      auto opts = image_schema.bind(ctx, *value);
      Runtime::ImageDependency dep{resource_id, opts.str("file-name")};
      if (opts.contains("transparency-color"))
      {
        dep.transparency_color = opts.obj<Color>("transparency-color");
      }
      return dep;
    }

    Runtime::ResourceDependencies parse_resource_dependencies(Lisple::Context& ctx,
                                                              const Lisple::sptr_val& value)
    {
      static Lisple::MapSchema resources_schema({},
                                                {{"images", &Lisple::Type::MAP},
                                                 {"sounds", &Lisple::Type::MAP},
                                                 {"fonts", &Lisple::Type::MAP}});

      auto opts = resources_schema.bind(ctx, *value);

      Runtime::ResourceDependencies deps;

      if (auto img_map = opts.val("images"))
      {
        for (auto& key : Lisple::Dict::map_keys(*img_map))
        {
          auto val = Lisple::Dict::get_property(img_map, *key);
          deps.images.push_back(parse_image_dependency(ctx, key->str(), val));
        }
      }

      if (auto sound_map = opts.val("sounds"))
      {
        for (auto& key : Lisple::Dict::map_keys(*sound_map))
        {
          auto val = Lisple::Dict::get_property(sound_map, *key);
          deps.sounds.push_back({key->str(), val->str()});
        }
      }

      if (auto font_map = opts.val("fonts"))
      {
        for (auto& key : Lisple::Dict::map_keys(*font_map))
        {
          auto val = Lisple::Dict::get_property(font_map, *key);
          deps.fonts.push_back({key->str(), val->str()});
        }
      }

      return deps;
    }

    std::string parse_bundle_keyword(const Lisple::sptr_val& value)
    {
      auto [bundle_id, resource_id] = value->qual();
      if (!bundle_id.empty() && resource_id.empty()) return bundle_id;
      if (bundle_id.empty()) return value->str();
      return bundle_id;
    }

    std::pair<std::string, std::string> parse_resource_keyword(const Lisple::sptr_val& value)
    {
      auto [bundle_id, resource_id] = value->qual();
      if (bundle_id.empty() || resource_id.empty())
      {
        throw Lisple::TypeError("Image resource must be a qualified keyword");
      }
      return {bundle_id, resource_id};
    }

    Lisple::sptr_val image_dependency_map(const std::string& bundle_id,
                                          const Runtime::ImageDependency& dep)
    {
      auto result = Lisple::map({});
      Lisple::Dict::set_property(result,
                                 MapKey::ID,
                                 Lisple::keyword(bundle_id + "/" + dep.resource_id));
      Lisple::Dict::set_property(result, MapKey::FILE_NAME, Lisple::string(dep.file_name));
      return result;
    }

    Lisple::sptr_val generated_image_map(const std::string& bundle_id,
                                         const std::string& resource_id,
                                         const Dimension& size)
    {
      auto result = Lisple::map({});
      Lisple::Dict::set_property(result,
                                 MapKey::ID,
                                 Lisple::keyword(bundle_id + "/" + resource_id));
      Lisple::Dict::set_property(result,
                                 MapKey::RESOURCE_SOURCE,
                                 Lisple::keyword("generated"));
      Lisple::Dict::set_property(result,
                                 MapKey::SIZE,
                                 DimensionAdapter::make_unique(size.w, size.h));
      return result;
    }

    Lisple::sptr_val image_dependency_value(const Runtime::ImageDependency& dep)
    {
      auto result = Lisple::map({});
      Lisple::Dict::set_property(result, MapKey::FILE_NAME, Lisple::string(dep.file_name));
      if (dep.transparency_color)
      {
        const Color& color = *dep.transparency_color;
        Lisple::Dict::set_property(
          result,
          MapKey::TRANSPARENCY_COLOR,
          ColorAdapter::make_unique(color.r, color.g, color.b, color.a));
      }
      return result;
    }

    Lisple::sptr_val image_dependencies_map(
      const std::vector<Runtime::ImageDependency>& images)
    {
      auto result = Lisple::map({});
      for (const auto& dep : images)
      {
        Lisple::Dict::set_property(result,
                                   Lisple::keyword(dep.resource_id),
                                   image_dependency_value(dep));
      }
      return result;
    }

    template <typename T> Lisple::sptr_val file_dependencies_map(const std::vector<T>& deps)
    {
      auto result = Lisple::map({});
      for (const auto& dep : deps)
      {
        Lisple::Dict::set_property(result,
                                   Lisple::keyword(dep.resource_id),
                                   Lisple::string(dep.file_name));
      }
      return result;
    }
  } // namespace

  namespace Function
  {
    /* MakeResourceDependencies */
    FUNC_IMPL(MakeResourceDependencies,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeResourceDependencies::exec_make_deps))));

    EXEC_BODY(MakeResourceDependencies, exec_make_deps)
    {
      return ResourceDependenciesAdapter::make_unique(
        parse_resource_dependencies(ctx, args[0]));
    }

    FUNC_IMPL(CreateBundleBang,
              MULTI_SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                         EXEC_DISPATCH(&CreateBundleBang::exec_create_bundle)),
                        (FN_ARGS((&Lisple::Type::KEYWORD), (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&CreateBundleBang::exec_create_bundle))));

    EXEC_BODY(CreateBundleBang, exec_create_bundle)
    {
      std::string bundle_id = parse_bundle_keyword(args[0]);
      Runtime::ResourceDependencies deps = args.size() > 1
                                             ? parse_resource_dependencies(ctx, args[1])
                                             : Runtime::ResourceDependencies{};

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->create_dynamic_bundle(bundle_id, deps);
      return args[0];
    }

    FUNC_IMPL(AddImageBang,
              MULTI_SIG((FN_ARGS((&Lisple::Type::KEYWORD), (&Lisple::Type::STRING)),
                         EXEC_DISPATCH(&AddImageBang::exec_add_image)),
                        (FN_ARGS((&Lisple::Type::KEYWORD), (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&AddImageBang::exec_add_image))));

    EXEC_BODY(AddImageBang, exec_add_image)
    {
      auto [bundle_id, resource_id] = parse_resource_keyword(args[0]);

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->add_image(bundle_id,
                                   parse_image_dependency(ctx, resource_id, args[1]));
      return args[0];
    }

    FUNC_IMPL(CreateImageBang,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD),
                           (&Lisple::Type::MAP),
                           (&Lisple::Type::FUNCTION)),
                   EXEC_DISPATCH(&CreateImageBang::exec_create_image))));

    EXEC_BODY(CreateImageBang, exec_create_image)
    {
      static Lisple::MapSchema create_image_opts_schema({{"size", &HostType::DIMENSION}},
                                                        {{"clear", &HostType::COLOR}});

      auto [bundle_id, resource_id] = parse_resource_keyword(args[0]);
      auto opts = create_image_opts_schema.bind(ctx, *args[1]);
      const Dimension& size = opts.obj<Dimension>("size");
      if (size.w <= 0 || size.h <= 0)
      {
        throw Lisple::TypeError("Generated image size must be positive");
      }

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      if (!rc.asset_registry->is_dynamic_bundle(bundle_id))
      {
        throw std::runtime_error("Bundle is not dynamic: " + bundle_id);
      }
      if (!rc.renderer)
      {
        throw std::runtime_error("Cannot create image without an SDL renderer");
      }

      std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
        SDL_CreateTexture(rc.renderer,
                          SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET,
                          size.w,
                          size.h),
        SDL_DestroyTexture);
      if (!texture)
      {
        throw std::runtime_error("Failed to create generated image texture: " + bundle_id +
                                 "/" + resource_id);
      }

      SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);

      {
        RenderTargetGuard target_guard(rc, texture.get());

        Color clear = opts.optional_obj<Color>("clear").value_or(Color{0, 0, 0, 0});
        SDL_SetRenderDrawColor(rc.renderer, clear.r, clear.g, clear.b, clear.a);
        SDL_RenderClear(rc.renderer);
        SDL_SetRenderDrawColor(rc.renderer, 0xff, 0xff, 0xff, 0xff);

        Lisple::sptr_val_v callback_args;
        args[2]->exec().execute(ctx, callback_args);
      }

      SDL_Texture* committed_texture = texture.get();
      rc.asset_registry->add_generated_image(bundle_id,
                                             resource_id,
                                             committed_texture,
                                             size);
      texture.release();
      return args[0];
    }

    FUNC_IMPL(RemoveImageBang,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&RemoveImageBang::exec_remove_image))));

    EXEC_BODY(RemoveImageBang, exec_remove_image)
    {
      auto [bundle_id, resource_id] = parse_resource_keyword(args[0]);

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->remove_image(bundle_id, resource_id);
      return args[0];
    }

    FUNC_IMPL(ListImages,
              SIG((FN_ARGS((&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&ListImages::exec_list_images))));

    EXEC_BODY(ListImages, exec_list_images)
    {
      std::string bundle_id = parse_bundle_keyword(args[0]);
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      Lisple::sptr_val_v resources;
      for (const auto& dep : rc.asset_registry->image_dependencies(bundle_id))
      {
        resources.push_back(image_dependency_map(bundle_id, dep));
      }
      for (const auto& [resource_id, size] :
           rc.asset_registry->generated_image_sizes(bundle_id))
      {
        resources.push_back(generated_image_map(bundle_id, resource_id, size));
      }
      return Lisple::vector(resources);
    }

    FUNC_IMPL(CanCreateImages,
              SIG((NO_ARGS, EXEC_DISPATCH(&CanCreateImages::exec_can_create_images))));

    EXEC_BODY(CanCreateImages, exec_can_create_images)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      return (rc.asset_registry && rc.renderer) ? Lisple::Constant::BOOL_TRUE
                                                : Lisple::Constant::BOOL_FALSE;
    }
  } // namespace Function

  /* ResourceDependenciesAdapter */
  NATIVE_ADAPTER_IMPL(ResourceDependenciesAdapter,
                      Pixils::Runtime::ResourceDependencies,
                      &HostType::RESOURCE_DEPENDENCIES,
                      (images));

  NOBJ_PROP_GET(ResourceDependenciesAdapter, images)
  {
    return image_dependencies_map(get_self_object().images);
  }

  NOBJ_PROP_GET(ResourceDependenciesAdapter, sounds)
  {
    return file_dependencies_map(get_self_object().sounds);
  }

  NOBJ_PROP_GET(ResourceDependenciesAdapter, fonts)
  {
    return file_dependencies_map(get_self_object().fonts);
  }

  /* ResourceNamespace */
  ResourceNamespace::ResourceNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__RESOURCE))
  {
    values.emplace(FN__CREATE_BUNDLE_BANG, Function::CreateBundleBang::make());
    values.emplace(FN__ADD_IMAGE_BANG, Function::AddImageBang::make());
    values.emplace(FN__CREATE_IMAGE_BANG, Function::CreateImageBang::make());
    values.emplace(FN__REMOVE_IMAGE_BANG, Function::RemoveImageBang::make());
    values.emplace(FN__LIST_IMAGES, Function::ListImages::make());
    values.emplace(FN__CAN_CREATE_IMAGES, Function::CanCreateImages::make());
    values.emplace(FN__MAKE_RESOURCE_DEPS, Function::MakeResourceDependencies::make());
  }
} // namespace Pixils::Script
