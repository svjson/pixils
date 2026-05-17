#include "pixils/ui/base_theme.h"

#include <pixils/binding/ui/style/theme_definition.h>

#include <lisple/context.h>
#include <lisple/namespace.h>
#include <lisple/runtime.h>
#include <optional>
#include <string>

namespace Pixils::UI
{
  namespace
  {
    constexpr const char* BASE_THEME_DEFINITION = "pixils.ui.base-theme/definition";

    void require_base_theme_namespace(Lisple::Runtime& runtime)
    {
      const std::string previous_namespace = runtime.get_current_namespace().get_name();

      try
      {
        runtime.eval("(ns pixils.ui.base-theme-loader (:require pixils.ui.base-theme))");
      }
      catch (...)
      {
        runtime.switch_namespace(previous_namespace);
        throw;
      }

      runtime.switch_namespace(previous_namespace);
    }

  } // namespace

  const Theme& default_base_theme(Lisple::Runtime& runtime)
  {
    static std::optional<Theme> cached_theme = std::nullopt;

    if (!cached_theme)
    {
      Lisple::Context ctx(runtime);
      require_base_theme_namespace(runtime);
      auto definition = runtime.lookup(BASE_THEME_DEFINITION);
      cached_theme =
        Script::build_theme_from_definition(ctx, "pixils/base-theme", definition);
    }

    return *cached_theme;
  }
} // namespace Pixils::UI
