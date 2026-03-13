#ifndef __PIXILS__SCRIPT_H_
#define __PIXILS__SCRIPT_H_

#include <lisple/runtime.h>
#include <vector>

namespace Pixils
{
  struct RenderContext;

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
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx, const std::string& default_namespace,
                                      const std::vector<std::string>& source_files);
} // namespace Pixils

#endif /* __PIXILS__SCRIPT_H_ */
