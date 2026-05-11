#include "pixils/ui/base_theme.h"

#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/embedded_lisp_sources.h>

#include <lisple/context.h>
#include <lisple/runtime.h>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace Pixils::UI
{
  namespace
  {
    const char* default_base_theme_source()
    {
      for (const auto& embedded_source : EmbeddedLisp::core_sources())
      {
        if (std::string_view(embedded_source.path) == "ui/base/base-theme.lisple")
        {
          return embedded_source.source;
        }
      }

      throw std::runtime_error("Embedded ui/base/base-theme.lisple source was not found");
    }

  } // namespace

  const Theme& default_base_theme(Lisple::Runtime& runtime)
  {
    static std::optional<Theme> cached_theme = std::nullopt;

    if (!cached_theme)
    {
      Lisple::Context ctx(runtime);
      auto definition = ctx.eval(default_base_theme_source());
      cached_theme =
        Script::build_theme_from_definition(ctx, "pixils/base-theme", definition);
    }

    return *cached_theme;
  }
} // namespace Pixils::UI
