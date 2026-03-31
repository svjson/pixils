#ifndef PIXILS__ASSET__LOADER_H
#define PIXILS__ASSET__LOADER_H

#include <pixils/asset/bundle.h>
#include <pixils/context.h>
#include <pixils/runtime/session.h>

namespace Pixils::Runtime
{
  struct ResourceDependencies;
}

namespace Pixils::Asset
{
  class Loader
  {
    RenderContext& ctx;

   public:
    Loader(RenderContext& ctx);
    Bundle load_bundle_assets(const Runtime::ResourceDependencies& deps);
  };

} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__LOADER_H */
