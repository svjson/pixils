#ifndef PIXILS__RESOURCE_NAMESPACE_H
#define PIXILS__RESOURCE_NAMESPACE_H

#include <pixils/runtime/mode.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  inline const std::string_view NS__PIXILS__RESOURCE = "pixils.resource";

  inline constexpr std::string_view FN__PIXILS__MAKE_RESOURCE_DEPS =
    "pixils.resource/make-resource-dependencies";
  inline const std::string FN__MAKE_RESOURCE_DEPS = "make-resource-dependencies";

  namespace HostType
  {
    HOST_TYPE(RESOURCE_DEPENDENCIES,
              "HResourceDependencies",
              std::string(FN__PIXILS__MAKE_RESOURCE_DEPS));
  }

  namespace Function
  {
    FUNC_DECL(MakeResourceDependencies, make_deps);
  }

  HOST_ADAPTER(ResourceDependenciesAdapter, Pixils::Runtime::ResourceDependencies);

  class ResourceNamespace : public Lisple::Namespace
  {
   public:
    ResourceNamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__RESOURCE_NAMESPACE_H */
