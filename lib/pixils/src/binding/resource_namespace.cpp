
#include "pixils/runtime/mode.h"
#include <pixils/binding/resource_namespace.h>

#include <lisple/host.h>
#include <lisple/host/accessor.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(IMAGES, "images");
    SHKEY(SOUNDS, "sounds");
  }

  namespace Function
  {
    /* MakeResourceDependencies */
    FUNC_IMPL(MakeResourceDependencies,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeResourceDependencies::exec_make_deps))));

    EXEC_BODY(MakeResourceDependencies, exec_make_deps)
    {
      static Lisple::MapSchema resources_schema(
        {},
        {{"images", &Lisple::Type::MAP}, {"sounds", &Lisple::Type::MAP}});

      auto opts = resources_schema.bind(ctx, *args[0]);

      Runtime::ResourceDependencies deps;

      if (auto img_map = opts.val("images"))
      {
        for (auto& key : Lisple::Dict::map_keys(*img_map))
        {
          auto val = Lisple::Dict::get_property(img_map, *key);
          deps.images.push_back({key->str(), val->str()});
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

      return ResourceDependenciesAdapter::make_unique(deps);
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

  /* ResourceNamespace */
  ResourceNamespace::ResourceNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__RESOURCE))
  {
    values.emplace(FN__MAKE_RESOURCE_DEPS, Function::MakeResourceDependencies::make());
  }
} // namespace Pixils::Script
