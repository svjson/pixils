#include "pixils/ui/base_theme.h"

#include <pixils/binding/pixils_namespace.h>

#include <roo/exception.h>
#include <roo/namespace.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <string>

namespace Pixils::UI
{
  namespace
  {
    constexpr const char* BASE_THEME_NAME = "pixils/base-theme";

    void require_base_theme_namespace(Roo::Runtime& runtime)
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

  const Theme& default_base_theme(Roo::Runtime& runtime)
  {
    auto themes = runtime.lookup(Script::ID__PIXILS__THEMES);
    auto theme = Roo::Dict::get_property(themes, Roo::symbol(BASE_THEME_NAME));
    if (!theme || theme->type == Roo::Value::Type::NIL)
    {
      require_base_theme_namespace(runtime);
      theme = Roo::Dict::get_property(themes, Roo::symbol(BASE_THEME_NAME));
    }
    if (!theme || theme->type == Roo::Value::Type::NIL)
    {
      throw Roo::RooException("Built-in base theme was not registered");
    }
    return Roo::obj<Theme>(*theme);
  }
} // namespace Pixils::UI
