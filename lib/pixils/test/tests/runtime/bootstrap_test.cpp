#include "fixture.h"
#include <pixils/embedded_lisp_sources.h>
#include <pixils/runtime/component.h>
#include <pixils/runtime/mode.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <string_view>

class BootstrapTest : public BaseFixture
{
};

TEST_F(BootstrapTest, loads_embedded_core_ui_definitions_into_separate_registries)
{
  auto modes = runtime.lookup("pixils/modes");
  auto components = runtime.lookup("pixils/components");
  ASSERT_NE(modes, nullptr);
  ASSERT_NE(components, nullptr);

  auto text_component = Roo::Dict::get_property(components, Roo::symbol("ui/text"));
  auto rich_text_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/rich-text"));
  auto text_input_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/text-input"));
  auto number_input_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/number-input"));
  auto button_component = Roo::Dict::get_property(components, Roo::symbol("ui/button"));
  auto toggle_button_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/toggle-button"));
  auto toggle_button_group_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/toggle-button-group"));
  auto checkbox_component = Roo::Dict::get_property(components, Roo::symbol("ui/checkbox"));
  auto scrollbar_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/scrollbar"));
  auto scrollbar_button_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/scrollbar-button"));
  auto scroll_pane_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/scroll-pane"));
  auto header_panel_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/header-panel"));
  auto group_box_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/group-box"));
  auto dialog_frame_mode = Roo::Dict::get_property(modes, Roo::symbol("ui/dialog-frame"));
  auto file_dialog_body_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/file-dialog-body"));
  auto window_component = Roo::Dict::get_property(components, Roo::symbol("ui/window"));
  auto menu_bar_component = Roo::Dict::get_property(components, Roo::symbol("ui/menu-bar"));
  auto popup_menu_mode = Roo::Dict::get_property(modes, Roo::symbol("ui/popup-menu"));
  auto list_box_component = Roo::Dict::get_property(components, Roo::symbol("ui/list-box"));
  auto combo_box_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/combo-box"));
  auto collapsible_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/collapsible"));
  auto slider_component = Roo::Dict::get_property(components, Roo::symbol("ui/slider"));
  auto split_pane_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/split-pane"));
  auto split_pane_resizer_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/split-pane-resizer"));
  auto icon_component = Roo::Dict::get_property(components, Roo::symbol("ui/icon"));
  auto desktop_icon_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/desktop-icon"));
  auto desktop_icon_preview_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/desktop-icon-preview"));
  auto icon_container_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/icon-container"));
  auto icon_preview_component =
    Roo::Dict::get_property(components, Roo::symbol("ui/icon-preview"));

  ASSERT_NE(text_component, nullptr);
  ASSERT_NE(rich_text_component, nullptr);
  ASSERT_NE(text_input_component, nullptr);
  ASSERT_NE(number_input_component, nullptr);
  ASSERT_NE(button_component, nullptr);
  ASSERT_NE(toggle_button_component, nullptr);
  ASSERT_NE(toggle_button_group_component, nullptr);
  ASSERT_NE(checkbox_component, nullptr);
  ASSERT_NE(scrollbar_component, nullptr);
  ASSERT_NE(scrollbar_button_component, nullptr);
  ASSERT_NE(scroll_pane_component, nullptr);
  ASSERT_NE(header_panel_component, nullptr);
  ASSERT_NE(group_box_component, nullptr);
  ASSERT_NE(dialog_frame_mode, nullptr);
  ASSERT_NE(file_dialog_body_component, nullptr);
  ASSERT_NE(window_component, nullptr);
  ASSERT_NE(menu_bar_component, nullptr);
  ASSERT_NE(popup_menu_mode, nullptr);
  ASSERT_NE(list_box_component, nullptr);
  ASSERT_NE(combo_box_component, nullptr);
  ASSERT_NE(collapsible_component, nullptr);
  ASSERT_NE(slider_component, nullptr);
  ASSERT_NE(split_pane_component, nullptr);
  ASSERT_NE(split_pane_resizer_component, nullptr);
  ASSERT_NE(icon_component, nullptr);
  ASSERT_NE(desktop_icon_component, nullptr);
  ASSERT_NE(desktop_icon_preview_component, nullptr);
  ASSERT_NE(icon_container_component, nullptr);
  ASSERT_NE(icon_preview_component, nullptr);

  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*text_component).name, "ui/text");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*rich_text_component).name, "ui/rich-text");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*text_input_component).name,
            "ui/text-input");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*number_input_component).name,
            "ui/number-input");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*button_component).name, "ui/button");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*toggle_button_component).name,
            "ui/toggle-button");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*toggle_button_group_component).name,
            "ui/toggle-button-group");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*checkbox_component).name, "ui/checkbox");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*scrollbar_component).name, "ui/scrollbar");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*scrollbar_button_component).name,
            "ui/scrollbar-button");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*scroll_pane_component).name,
            "ui/scroll-pane");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*header_panel_component).name,
            "ui/header-panel");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*group_box_component).name, "ui/group-box");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Mode>(*dialog_frame_mode).name, "ui/dialog-frame");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*file_dialog_body_component).name,
            "ui/file-dialog-body");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*window_component).name, "ui/window");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*menu_bar_component).name, "ui/menu-bar");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Mode>(*popup_menu_mode).name, "ui/popup-menu");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*list_box_component).name, "ui/list-box");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*combo_box_component).name, "ui/combo-box");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*collapsible_component).name,
            "ui/collapsible");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*slider_component).name, "ui/slider");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*split_pane_component).name,
            "ui/split-pane");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*split_pane_resizer_component).name,
            "ui/split-pane-resizer");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*icon_component).name, "ui/icon");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*desktop_icon_component).name,
            "ui/desktop-icon");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*desktop_icon_preview_component).name,
            "ui/desktop-icon-preview");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*icon_container_component).name,
            "ui/icon-container");
  EXPECT_EQ(Roo::obj<Pixils::Runtime::Component>(*icon_preview_component).name,
            "ui/icon-preview");

  EXPECT_EQ(Roo::Dict::get_property(modes, Roo::symbol("ui/button"))->type,
            Roo::Value::Type::NIL);
  EXPECT_EQ(Roo::Dict::get_property(components, Roo::symbol("ui/dialog-frame"))->type,
            Roo::Value::Type::NIL);
}

TEST_F(BootstrapTest, loads_embedded_namespaces_through_the_namespace_source)
{
  auto* slider_namespace = runtime.ns("pixils.ui.slider");

  ASSERT_NE(slider_namespace, nullptr);
  EXPECT_EQ(slider_namespace->get_origin().type, Roo::Namespace::Origin::Type::FILE);
  ASSERT_TRUE(slider_namespace->get_origin().source_path.has_value());
  EXPECT_EQ(*slider_namespace->get_origin().source_path, "ui/slider.roo");
}

TEST_F(BootstrapTest, includes_embedded_base_theme_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto base_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/base-theme.roo"; });

  ASSERT_NE(base_theme, sources.end());
  EXPECT_NE(base_theme->contents.find("(ns pixils.ui.base-theme"), std::string_view::npos);
  EXPECT_NE(base_theme->contents.find("(def definition"), std::string_view::npos);
  EXPECT_NE(base_theme->contents.find(":ui/panel"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_dialog_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto dialog = std::find_if(sources.begin(),
                             sources.end(),
                             [](const Pixils::EmbeddedLisp::Source& source)
                             { return std::string_view(source.path) == "ui/dialog.roo"; });

  ASSERT_NE(dialog, sources.end());
  EXPECT_NE(dialog->contents.find("(ns pixils.ui.dialog"), std::string_view::npos);
  EXPECT_NE(dialog->contents.find("(defun open-dialog!"), std::string_view::npos);
  EXPECT_NE(dialog->contents.find(":dialog/ok-cancel"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_rich_text_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto rich_text =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/rich-text.roo"; });

  ASSERT_NE(rich_text, sources.end());
  EXPECT_NE(rich_text->contents.find("(ns pixils.ui.rich-text"), std::string_view::npos);
  EXPECT_NE(rich_text->contents.find("(defun make"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_popover_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto popover = std::find_if(sources.begin(),
                              sources.end(),
                              [](const Pixils::EmbeddedLisp::Source& source)
                              { return std::string_view(source.path) == "ui/popover.roo"; });

  ASSERT_NE(popover, sources.end());
  EXPECT_NE(popover->contents.find("(ns pixils.ui.popover"), std::string_view::npos);
  EXPECT_NE(popover->contents.find("(defun make"), std::string_view::npos);
  EXPECT_NE(popover->contents.find("(defun open!"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_toggle_button_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto toggle_button =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/toggle-button.roo"; });

  ASSERT_NE(toggle_button, sources.end());
  EXPECT_NE(toggle_button->contents.find("(ns pixils.ui.toggle-button"),
            std::string_view::npos);
  EXPECT_NE(toggle_button->contents.find("(defcomponent ui/toggle-button"),
            std::string_view::npos);
  EXPECT_NE(toggle_button->contents.find("(defun make-group"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_file_dialog_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto file_dialog =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/file-dialog.roo"; });

  ASSERT_NE(file_dialog, sources.end());
  EXPECT_NE(file_dialog->contents.find("(ns pixils.ui.file-dialog"), std::string_view::npos);
  EXPECT_NE(file_dialog->contents.find("(defun open-file-dialog!"), std::string_view::npos);
  EXPECT_NE(file_dialog->contents.find("(defun list-directory"), std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_classic_blue_theme_source)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();
  auto classic_blue_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/themes/classic-blue.roo"; });

  ASSERT_NE(classic_blue_theme, sources.end());
  EXPECT_NE(classic_blue_theme->contents.find("(deffont classic-blue-font"),
            std::string_view::npos);
  EXPECT_NE(classic_blue_theme->contents.find(":resource :pixils/autoega-8x14"),
            std::string_view::npos);
  EXPECT_NE(classic_blue_theme->contents.find(":font :font/classic-blue-font"),
            std::string_view::npos);
  EXPECT_NE(classic_blue_theme->contents.find("(deftheme pixils/classic-blue"),
            std::string_view::npos);
}

TEST_F(BootstrapTest, includes_embedded_windows_theme_sources)
{
  const auto& sources = Pixils::EmbeddedLisp::core_sources();

  auto windows_3_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/themes/windows-3.roo"; });
  auto windows_95_theme =
    std::find_if(sources.begin(),
                 sources.end(),
                 [](const Pixils::EmbeddedLisp::Source& source)
                 { return std::string_view(source.path) == "ui/themes/windows-95.roo"; });

  ASSERT_NE(windows_3_theme, sources.end());
  ASSERT_NE(windows_95_theme, sources.end());
  EXPECT_NE(windows_3_theme->contents.find("(deftheme pixils/windows-3"),
            std::string_view::npos);
  EXPECT_NE(windows_95_theme->contents.find("(deftheme pixils/windows-95"),
            std::string_view::npos);
}

TEST_F(BootstrapTest, loads_embedded_core_themes_into_registry)
{
  auto themes = runtime.lookup("pixils/themes");
  ASSERT_NE(themes, nullptr);

  auto classic_blue = Roo::Dict::get_property(themes, Roo::symbol("pixils/classic-blue"));
  auto windows_3 = Roo::Dict::get_property(themes, Roo::symbol("pixils/windows-3"));
  auto windows_95 = Roo::Dict::get_property(themes, Roo::symbol("pixils/windows-95"));

  ASSERT_NE(classic_blue, nullptr);
  ASSERT_NE(windows_3, nullptr);
  ASSERT_NE(windows_95, nullptr);
}
