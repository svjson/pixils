
#include "pixils/binding/style_namespace.h"
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/render_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/script.h>

#include <lisple/dir_root_file_system.h>

namespace Pixils
{
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      const std::vector<std::string>& source_files)
  {
    return init_lisple_runtime(ctx, default_namespace, nullptr, source_files);
  }
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      std::function<void(RuntimeConfiguration*)> init_fn,
                                      const std::vector<std::string>& source_files)
  {
    std::vector<std::unique_ptr<Lisple::Namespace>> namespaces;
    namespaces.push_back(std::make_unique<Pixils::Script::PixilsNamespace>(ctx));
    namespaces.push_back(std::make_unique<Pixils::Script::ResourceNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::ColorNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::PointNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::RenderNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::StyleNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::UINamespace>());

    RuntimeConfiguration rtconfig{.native_namespaces = std::move(namespaces),
                                  .load_path = {"."}};

    if (init_fn)
    {
      init_fn(&rtconfig);
    }

    std::unique_ptr<Lisple::DirRootFileSystem> fs =
      std::make_unique<Lisple::DirRootFileSystem>(rtconfig.load_path);

    Lisple::Runtime lisple_runtime(default_namespace,
                                   std::move(rtconfig.native_namespaces),
                                   std::move(fs.release()));
    for (auto& file_name : source_files)
    {
      lisple_runtime.read_file(file_name);
    }

    return lisple_runtime;
  }
} // namespace Pixils
