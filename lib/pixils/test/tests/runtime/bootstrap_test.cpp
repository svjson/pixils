#include "fixture.h"

#include <pixils/runtime/mode.h>

#include <gtest/gtest.h>
#include <lisple/host.h>
#include <lisple/runtime/dict.h>

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
  auto window_mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/window"));
  auto menu_bar_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/menu-bar"));
  auto popup_menu_mode =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("ui/popup-menu"));

  ASSERT_NE(text_mode, nullptr);
  ASSERT_NE(text_input_mode, nullptr);
  ASSERT_NE(button_mode, nullptr);
  ASSERT_NE(window_mode, nullptr);
  ASSERT_NE(menu_bar_mode, nullptr);
  ASSERT_NE(popup_menu_mode, nullptr);

  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*text_mode).name, "ui/text");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*text_input_mode).name, "ui/text-input");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*button_mode).name, "ui/button");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*window_mode).name, "ui/window");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*menu_bar_mode).name, "ui/menu-bar");
  EXPECT_EQ(Lisple::obj<Pixils::Runtime::Mode>(*popup_menu_mode).name, "ui/popup-menu");
}
