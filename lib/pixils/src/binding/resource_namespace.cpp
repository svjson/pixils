
#include "pixils/runtime/mode.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/context.h>

#include <lisple/host/accessor.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(FILE_NAME, "file-name");
    SHKEY(FONTS, "fonts");
    SHKEY(ID, "id");
    SHKEY(IMAGES, "images");
    SHKEY(SOUNDS, "sounds");
    SHKEY(TRANSPARENCY_COLOR, "transparency-color");
  } // namespace MapKey

  namespace
  {
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

      static Lisple::MapSchema image_schema(
        {{"file-name", &Lisple::Type::STRING}},
        {{"transparency-color", &HostType::COLOR}});

      auto opts = image_schema.bind(ctx, *value);
      Runtime::ImageDependency dep{resource_id, opts.str("file-name")};
      if (opts.contains("transparency-color"))
      {
        dep.transparency_color = opts.obj<Color>("transparency-color");
      }
      return dep;
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
  } // namespace

  namespace Function
  {
    /* MakeResourceDependencies */
    FUNC_IMPL(MakeResourceDependencies,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeResourceDependencies::exec_make_deps))));

    EXEC_BODY(MakeResourceDependencies, exec_make_deps)
    {
      static Lisple::MapSchema resources_schema({},
                                                {{"images", &Lisple::Type::MAP},
                                                 {"sounds", &Lisple::Type::MAP},
                                                 {"fonts", &Lisple::Type::MAP}});

      auto opts = resources_schema.bind(ctx, *args[0]);

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

      return ResourceDependenciesAdapter::make_unique(deps);
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
      return Lisple::vector(resources);
    }
  } // namespace Function

  /* ResourceDependenciesAdapter */
  NATIVE_ADAPTER_IMPL(ResourceDependenciesAdapter,
                      Pixils::Runtime::ResourceDependencies,
                      &HostType::RESOURCE_DEPENDENCIES,
                      (images));

  NOBJ_PROP_GET(ResourceDependenciesAdapter, images)
  {
    return Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(ResourceDependenciesAdapter, sounds)
  {
    return Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(ResourceDependenciesAdapter, fonts)
  {
    return Lisple::Constant::NIL;
  }

  /* ResourceNamespace */
  ResourceNamespace::ResourceNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__RESOURCE))
  {
    values.emplace(FN__ADD_IMAGE_BANG, Function::AddImageBang::make());
    values.emplace(FN__REMOVE_IMAGE_BANG, Function::RemoveImageBang::make());
    values.emplace(FN__LIST_IMAGES, Function::ListImages::make());
    values.emplace(FN__MAKE_RESOURCE_DEPS, Function::MakeResourceDependencies::make());
  }
} // namespace Pixils::Script
