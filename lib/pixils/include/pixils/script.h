#ifndef __PIXILS__SCRIPT_H_
#define __PIXILS__SCRIPT_H_

#include <roo/namespace_source.h>
#include <roo/runtime.h>
#include <memory>
#include <vector>

namespace Pixils
{
  struct RenderContext;

  struct RuntimeConfiguration
  {
    std::vector<std::unique_ptr<Roo::Namespace>> native_namespaces;
    std::vector<std::string> load_path;
    std::vector<Roo::NamespaceRoot> namespace_roots;
    std::string asset_base_path = ".";
  };

  std::vector<std::unique_ptr<Roo::Namespace>> make_roo_native_namespaces(
    RenderContext& ctx);

  /**
   * @brief Create and initialize the Roo runtime, loading the provided
   * source files.
   *
   * @param ctx The render context to pass to the Roo runtime, which will be
   *   exposed to Roo code through the Pixils namespace.
   * @param default_namespace The default Roo namespace used by the runtime.
   * @param source_files A list of source files to load into the runtime. These
   *   should be relative to the working directory.
   *
   * @return An initialized Roo runtime with the provided source files loaded.
   */
  Roo::Runtime init_roo_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      const std::vector<std::string>& source_files);

  Roo::Runtime init_roo_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      std::function<void(RuntimeConfiguration*)> init_fn,
                                      const std::vector<std::string>& source_files);

  std::unique_ptr<Roo::Runtime> make_roo_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    const std::vector<std::string>& source_files);

  std::unique_ptr<Roo::Runtime> make_roo_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    std::function<void(RuntimeConfiguration*)> init_fn,
    const std::vector<std::string>& source_files);

} // namespace Pixils

#endif /* __PIXILS__SCRIPT_H_ */
