#include <pixils/asset/registry.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/script.h>

#include <lisple-package/native_abi.h>

#include <exception>
#include <memory>
#include <string>

namespace
{
  std::unique_ptr<Pixils::RenderContext> package_context;
  std::string package_last_error;

  int load_native_package(const LispleNativeHostV1* host)
  {
    try
    {
      package_context = std::make_unique<Pixils::RenderContext>();
      package_context->asset_registry =
        std::make_unique<Pixils::Asset::Registry>(*package_context);
      package_context->font_registry = std::make_unique<Pixils::FontRegistry>();

      auto namespaces = Pixils::make_lisple_native_namespaces(*package_context);
      for (auto& ns : namespaces)
      {
        ns->set_origin(Lisple::Namespace::Origin::native());
        if (host->register_namespace(host->user, ns.release()) != 0)
        {
          package_context.reset();
          return 1;
        }
      }
      return 0;
    }
    catch (const std::exception& e)
    {
      package_last_error = e.what();
      package_context.reset();
      return 1;
    }
  }

  void unload_native_package()
  {
    package_context.reset();
    package_last_error.clear();
  }

  const char* last_error()
  {
    return package_last_error.c_str();
  }
} // namespace

extern "C" LISPLE_NATIVE_EXPORT const LispleNativePackageV1*
lisple_native_package_v1()
{
  static const LispleNativePackageV1 package{
    LISPLE_NATIVE_ABI_VERSION,
    sizeof(LispleNativePackageV1),
    "pixils-native",
    "0.1.0",
    LISPLE_NATIVE_CXX_ABI,
    load_native_package,
    unload_native_package,
    last_error,
  };
  return &package;
}
