
#include "pixils/binding/arg_collector.h"
#include "pixils/runtime/mode.h"
#include <pixils/binding/resource_namespace.h>

#include <lisple/host.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(IMAGES, "images");
  }

  namespace Function
  {
    /* MakeResourceDependencies */
    FUNC_IMPL(MakeResourceDependencies,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeResourceDependencies::make_deps))));

    ArgCollector res_dep_coll(std::string(FN__PIXILS__MAKE_RESOURCE_DEPS),
                              {},
                              {{*MapKey::IMAGES, &Lisple::Type::MAP}});

    FUNC_BODY(MakeResourceDependencies, make_deps)
    {
      str_key_map_t keys = res_dep_coll.collect_keys(ctx, *args.front());

      Runtime::ResourceDependencies deps;

      if (keys.count(MapKey::IMAGES->value))
      {
        Lisple::Map& img_map = ArgCollector::lmap_value(keys, *MapKey::IMAGES);
        for (auto& key : img_map.keys())
        {
          auto val = img_map.get_sptr_property(*key);
          deps.images.push_back({key->as<Lisple::Value<std::string>>().value,
                                 val->as<Lisple::Value<std::string>>().value});
        }
      }

      return ResourceDependenciesAdapter::make<Runtime::ResourceDependencies>(deps);
    }
  } // namespace Function

  /* ResourceDependenciesAdapter */
  HOST_ADAPTER_IMPL(ResourceDependenciesAdapter,
                    Pixils::Runtime::ResourceDependencies,
                    &HostType::RESOURCE_DEPENDENCIES);

  /* ResourceNamespace */
  ResourceNamespace::ResourceNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__RESOURCE))
  {
    objects.emplace(FN__MAKE_RESOURCE_DEPS,
                    std::make_shared<Function::MakeResourceDependencies>());
  }
} // namespace Pixils::Script
