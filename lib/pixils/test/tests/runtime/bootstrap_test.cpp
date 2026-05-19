#include "fixture.h"
#include <pixils/embedded_lisp_sources.h>
#include <pixils/runtime/mode.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <string_view>

class BootstrapTest : public BaseFixture
{
};

TEST_F(BootstrapTest, loads_embedded_core_ui_modes_into_pixils_mode_registry)
{
  auto modes = runtime.lookup("pixils/modes");
  ASSERT_NE(modes, nullptr);

  auto text_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/text"));
  auto text_input_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/text-input"));
  auto number_input_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/number-input"));
  auto button_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/button"));
  auto checkbox_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/checkbox"));
  auto scrollbar_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/scrollbar"));
  auto scrollbar_button_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/scrollbar-button"));
  auto scroll_pane_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/scroll-pane"));
  auto header_panel_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/header-panel"));
  auto dialog_frame_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/dialog-frame"));
  auto file_dialog_body_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/file-dialog-body"));
  auto window_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/window"));
  auto menu_bar_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/menu-bar"));
  auto popup_menu_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/popup-menu"));
  auto list_box_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/list-box"));
  auto combo_box_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/combo-box"));
  auto slider_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/slider"));
  auto split_pane_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/split-pane"));
  auto split_pane_resizer_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/split-pane-resizer"));
  auto icon_mode = Lisple::Dict::get_property(modes, Lisple::symbol("ui/icon"));
  auto icon_container_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/icon-container"));
  auto icon_preview_mode =
    Lisple::Dict::get_property(modes, Lisple::symbol("ui/icon-preview"));

  ASSERT_NE(text_mode, nullptr);
  ASSERT_NE(text_input_mode, nullptr);
  ASSERT_NE(number_input_mode, nullptr);
  ASSERT_NE(button_mode, nullptr);
  ASSERT_NE(checkbox_mode, nullptr);
  ASSERT_NE(scrollbar_mode, nullptr);
  ASSERT_NE(scrollbar_button_mode, nullptr);
  ASSERT_NE(scroll_pane_mode, nullptr);
  ASSERT_NE(header_panel_mode, nullptr);
  ASSERT_NE(dialog_frame_mode, nullptr);
  ASSERT_NE(file_dialog_body_mode, nullptr);
  ASSERT_NE(window_mode, nullptr);
  ASSERT_NE(menu_bar_mode, nullptr);
  ASSERT_NE(popup_menu_mode, nullptr);
  ASSERT_NE(list_box_mode, nullptr);
  ASSERT_NE(combo_box_mode, nullptr);
  ASSERT_NE(slider_mode, nullptr);
  ASSERT_NE(split_pane_mode, nullptr);
  ASSERT_NE(split_pane_resizer_mode, nullptr);
  ASSERT_NE(icon_mode, nullptr);
  ASSERT_NE(icon_container_mode, nullptr);
  ASSERT_NE(icon_preview_mode, nullptr);

  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*text_mode).name, "ui/text");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*text_input_mode).name, "ui/text-input");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*number_input_mode).name, "ui/number-input");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*button_mode).name, "ui/button");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*checkbox_mode).name, "ui/checkbox");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*scrollbar_mode).name, "ui/scrollbar");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*scrollbar_button_mode).name,
            "ui/scrollbar-button");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*scroll_pane_mode).name, "ui/scroll-pane");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*header_panel_mode).name, "ui/header-panel");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*dialog_frame_mode).name, "ui/dialog-frame");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*file_dialog_body_mode).name,
            "ui/file-dialog-body");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*window_mode).name, "ui/window");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*menu_bar_mode).name, "ui/menu-bar");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*popup_menu_mode).name, "ui/popup-menu");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*list_box_mode).name, "ui/list-box");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*combo_box_mode).name, "ui/combo-box");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*slider_mode).name, "ui/slider");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*split_pane_mode).name, "ui/split-pane");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*split_pane_resizer_mode).name,
            "ui/split-pane-resizer");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*icon_mode).name, "ui/icon");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*icon_container_mode).name,
            "ui/icon-container");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*icon_preview_mode).name, "ui/icon-preview");
}

TEST_F(BootstrapTest, includes_embedded_base_theme_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto base_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/base-theme.lisple"; });

  ASSERT_NE(base_theme, sources.end());
  EXPECT_NE(std::string_view(base_theme->source).find("(ns pixils.ui.base-theme"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(base_theme->source).find("(def definition"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(base_theme->source).find(":ui/panel"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_dialog_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto dialog =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/dialog.lisple"; });

  ASSERT_NE(dialog, sources.end());
  EXPECT_NE(std::string_view(dialog->source).find("(ns pixils.ui.dialog"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(dialog->source).find("(defun open-dialog!"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(dialog->source).find(":dialog/ok-cancel"),
            std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_file_dialog_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto file_dialog =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/file-dialog.lisple"; });

  ASSERT_NE(file_dialog, sources.end());
  EXPECT_NE(std::string_view(file_dialog->source).find("(ns pixils.ui.file-dialog"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(file_dialog->source).find("(defun open-file-dialog!"),
            std::string_view::npos);
  EXPECT_NE(std::string_view(file_dialog->source).find("(defun list-directory"),
            std::string_view::npos);
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
  EXPECT_NE(std::string_view(classic_blue_theme->source).find("(deffont classic-blue-font"),
            std::string_view::npos);
  EXPECT_NE(
    std::string_view(classic_blue_theme->source).find(":resource :pixils/autoega-8x14"),
    std::string_view::npos);
  EXPECT_NE(
    std::string_view(classic_blue_theme->source).find(":font :font/classic-blue-font"),
    std::string_view::npos);
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
  auto themes = runtime.lookup("pixils/themes");
  ASSERT_NE(themes, nullptr);

  auto classic_blue =
    Lisple::Dict::get_property(themes, Lisple::symbol("pixils/classic-blue"));
  auto windows_3 = Lisple::Dict::get_property(themes, Lisple::symbol("pixils/windows-3"));
  auto windows_95 = Lisple::Dict::get_property(themes, Lisple::symbol("pixils/windows-95"));

  ASSERT_NE(classic_blue, nullptr);
  ASSERT_NE(windows_3, nullptr);
  ASSERT_NE(windows_95, nullptr);
}
