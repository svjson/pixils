
#include <pixils/pixils_namespace.h>
#include <pixils/script.h>

#include <lisple/dir_root_file_system.h>

namespace Pixils
{
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::vector<std::string>& source_files)
  {
    std::string path = ".";

    std::unique_ptr<Lisple::DirRootFileSystem> fs =
        std::make_unique<Lisple::DirRootFileSystem>(path);

    std::map<const std::string, Lisple::Namespace> namespaces;
    namespaces.emplace(Pixils::Script::NS_PIXILS, Pixils::Script::PixilsNamespace(ctx));

    Lisple::Runtime lisple_runtime("asteroids", namespaces, std::move(fs.release()));
    for (auto& file_name : source_files)
    {
      lisple_runtime.read_file(path + file_name);
    }

    return lisple_runtime;
  }
} // namespace Pixils
