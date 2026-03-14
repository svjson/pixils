
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/render_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/script.h>

#include <lisple/dir_root_file_system.h>

namespace Pixils
{
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      const std::vector<std::string>& source_files)
  {
    std::string path = ".";

    std::unique_ptr<Lisple::DirRootFileSystem> fs =
      std::make_unique<Lisple::DirRootFileSystem>(path);

    std::map<const std::string, Lisple::Namespace> namespaces;
    namespaces.emplace(Pixils::Script::NS_PIXILS, Pixils::Script::PixilsNamespace(ctx));
    namespaces.emplace(Pixils::Script::NS__PIXILS__RESOURCE,
                       Pixils::Script::ResourceNamespace());
    namespaces.emplace(Pixils::Script::NS__PIXILS__COLOR, Pixils::Script::ColorNamespace());
    namespaces.emplace(Pixils::Script::NS__PIXILS__POINT, Pixils::Script::PointNamespace());
    namespaces.emplace(Pixils::Script::NS__PIXILS__RENDER,
                       Pixils::Script::RenderNamespace());

    Lisple::Runtime lisple_runtime(default_namespace, namespaces, std::move(fs.release()));
    for (auto& file_name : source_files)
    {
      lisple_runtime.read_file(path + file_name);
    }

    return lisple_runtime;
  }
} // namespace Pixils
