#include "fixture.h"
#include <pixils/embedded_lisp_sources.h>
#include <pixils/runtime/mode.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <lisple/host.h>
#include <lisple/runtime/dict.h>
#include <string_view>

class BootstrapTest : public BaseFixture
{
};

TEST_F(BootstrapTest, loads_embedded_core_ui_modes_into_pixils_mode_registry)
{
  auto modes = runtime.lookup_value("pixils/modes");
  ASSERT_NE(modes, nullptr);

  auto text_mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/text"));
  auto text_input_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/text-input"));
  auto button_mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/button"));
  auto scrollbar_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/scrollbar"));
  auto scrollbar_button_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/scrollbar-button"));
  auto scroll_pane_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/scroll-pane"));
  auto window_mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/window"));
  auto menu_bar_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/menu-bar"));
  auto popup_menu_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/popup-menu"));

  ASSERT_NE(text_mode, nullptr);
  ASSERT_NE(text_input_mode, nullptr);
  ASSERT_NE(button_mode, nullptr);
  ASSERT_NE(scrollbar_mode, nullptr);
  ASSERT_NE(scrollbar_button_mode, nullptr);
  ASSERT_NE(scroll_pane_mode, nullptr);
  ASSERT_NE(window_mode, nullptr);
  ASSERT_NE(menu_bar_mode, nullptr);
  ASSERT_NE(popup_menu_mode, nullptr);

  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*text_mode).name, "ui/text");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*text_input_mode).name, "ui/text-input");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*button_mode).name, "ui/button");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*scrollbar_mode).name, "ui/scrollbar");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*scrollbar_button_mode).name,
            "ui/scrollbar-button");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*scroll_pane_mode).name, "ui/scroll-pane");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*window_mode).name, "ui/window");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*menu_bar_mode).name, "ui/menu-bar");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*popup_menu_mode).name, "ui/popup-menu");
}

TEST_F(BootstrapTest, includes_embedded_base_theme_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto base_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/base/base-theme.lisple"; });

  ASSERT_NE(base_theme, sources.end());
  EXPECT_NE(std::string_view(base_theme->source).find(":ui/panel"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_classic_blue_theme_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto classic_blue_theme = std::find_if(
    sources.begin(),
    sources.end(),
    [](const Pixils::EmbeddedLisp::Source& source)
    { return std::string_view(source.path) == "ui/themes/classic-blue.lisple"; });

  ASSERT_NE(classic_blue_theme, sources.end());
  EXPECT_NE(
    std::string_view(classic_blue_theme->source).find("(deftheme pixils/classic-blue"),
    std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_windows_theme_sources)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();

  auto windows_3_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/themes/windows-3.lisple"; });
  auto windows_95_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/themes/windows-95.lisple"; });

  ASSERT_NE(windows_3_theme, sources.end());
  ASSERT_NE(windows_95_theme, sources.end());
  EXPECT_NE(std::string_view(windows_3_theme->source).find("(deftheme pixils/windows-3"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(windows_95_theme->source).find("(deftheme pixils/windows-95"),
            std::string_view::npos);
}

TEST_F(BootstrapTest, loads_embedded_core_themes_into_registry)
{
  auto themes = runtime.lookup_value("pixils/themes");
  ASSERT_NE(themes, nullptr);

  auto classic_blue =
    Lisple::Dict::get_property(themes, Lisple::RTValue::symbol("pixils/classic-blue"));
  auto windows_3 =
    Lisple::Dict::get_property(themes, Lisple::RTValue::symbol("pixils/windows-3"));
  auto windows_95 =
    Lisple::Dict::get_property(themes, Lisple::RTValue::symbol("pixils/windows-95"));

  ASSERT_NE(classic_blue, nullptr);
  ASSERT_NE(windows_3, nullptr);
  ASSERT_NE(windows_95, nullptr);
}
