#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using MenuTest = RenderFixture;

TEST_F(MenuTest, make_menu_accepts_options_map_with_style)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.menu/make-menu
                   {:state {:game (pixils.ui/bind-state :game)}
                    :style {:width 123
                            :height 17}}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", runtime.eval("{:game :ready}"));
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto menu = session.active_mode->children[0];
  ASSERT_NE(menu, nullptr);
  ASSERT_NE(menu->mode, nullptr);
  EXPECT_EQ(menu->mode->name, "ui/menu-bar");
  EXPECT_EQ(menu->bounds.w, 123);
  EXPECT_EQ(menu->bounds.h, 17);
}

TEST_F(MenuTest, make_menu_keeps_legacy_three_argument_shape)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.menu/make-menu
                   {:game (pixils.ui/bind-state :game)}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", runtime.eval("{:game :ready}"));
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto menu = session.active_mode->children[0];
  ASSERT_NE(menu, nullptr);
  ASSERT_NE(menu->mode, nullptr);
  EXPECT_EQ(menu->mode->name, "ui/menu-bar");
}

TEST_F(MenuTest, opened_popup_inherits_menu_scale)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"
                :items [{:label "Open"
                         :action :file/open}]}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.menu/make-menu
                   {:style {:scale 2}}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto menu = session.active_mode->children[0];
  ASSERT_NE(menu, nullptr);
  auto menu_scale =
    Lisple::Dict::get_property(menu->state, Lisple::RTValue::keyword("menu-scale"));
  ASSERT_NE(menu_scale, nullptr);
  EXPECT_EQ(menu_scale->to_string(), "2");
  ASSERT_EQ(menu->children.size(), 1u);
  auto menu_item = menu->children[0];
  ASSERT_NE(menu_item, nullptr);

  input().mouse_down({menu_item->bounds.x * 2 + menu_item->bounds.w,
                      menu_item->bounds.y * 2 + menu_item->bounds.h});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/popup-menu");
  ASSERT_TRUE(session.active_mode->effective_style.scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.scale, 2);
}
