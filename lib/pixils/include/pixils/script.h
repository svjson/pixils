#ifndef __PIXILS__SCRIPT_H_
#define __PIXILS__SCRIPT_H_

#include <lisple/runtime.h>
#include <memory>
#include <vector>

namespace Pixils
{
  struct RenderContext;

  struct RuntimeConfiguration
  {
    std::vector<std::unique_ptr<Lisple::Namespace>> native_namespaces;
    std::vector<std::string> load_path;
    std::string asset_base_path = ".";
  };

  /**
   * @brief Create and initialize the Lisple runtime, loading the provided
   * source files.
   *
   * @param ctx The render context to pass to the Lisple runtime, which will be
   *   exposed to Lisple code through the Pixils namespace.
   * @param default_namespace The default Lisple namespace used by the runtime.
   * @param source_files A list of source files to load into the runtime. These
   *   should be relative to the working directory.
   *
   * @return An initialized Lisple runtime with the provided source files loaded.
   */
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      const std::vector<std::string>& source_files);

  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      std::function<void(RuntimeConfiguration*)> init_fn,
                                      const std::vector<std::string>& source_files);

  std::unique_ptr<Lisple::Runtime> make_lisple_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    const std::vector<std::string>& source_files);

  std::unique_ptr<Lisple::Runtime> make_lisple_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    std::function<void(RuntimeConfiguration*)> init_fn,
    const std::vector<std::string>& source_files);

} // namespace Pixils

#endif /* __PIXILS__SCRIPT_H_ */
