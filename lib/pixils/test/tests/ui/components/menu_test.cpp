#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>
#include <SDL2/SDL_mouse.h>

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
  auto menu_scale = Lisple::Dict::get_property(menu->state, Lisple::keyword("menu-scale"));
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

TEST_F(MenuTest, opened_popup_without_menu_scale_omits_scale_style)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "Game"
                :items [{:label "New"
                         :action :game/new}]}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.menu/make-menu
                   {}
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
  auto menu_scale = Lisple::Dict::get_property(menu->state, Lisple::keyword("menu-scale"));
  EXPECT_TRUE(!menu_scale || menu_scale->type == Lisple::Value::Type::NIL);
  ASSERT_EQ(menu->children.size(), 1u);
  auto menu_item = menu->children[0];
  ASSERT_NE(menu_item, nullptr);

  input().mouse_down({menu_item->bounds.x + menu_item->bounds.w / 2,
                      menu_item->bounds.y + menu_item->bounds.h / 2});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/popup-menu");
  EXPECT_FALSE(session.active_mode->effective_style.scale.has_value());
}

TEST_F(MenuTest, classic_blue_menus_use_classic_blue_font)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"
                :items [{:label "Open"
                         :action :file/open}]}]})

    (pixils/defmode root-mode
      {:theme 'pixils/classic-blue
       :children [(pixils.ui.menu/make-menu
                   {}
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
  ASSERT_EQ(menu->children.size(), 1u);
  auto menu_item = menu->children[0];
  ASSERT_NE(menu_item, nullptr);
  ASSERT_EQ(menu_item->children.size(), 1u);
  auto menu_text = menu_item->children[0];
  ASSERT_NE(menu_text, nullptr);
  ASSERT_TRUE(menu_text->effective_style.text.has_value());
  ASSERT_TRUE(menu_text->effective_style.text->font.has_value());
  EXPECT_EQ(*menu_text->effective_style.text->font, "font/classic-blue-font");

  input().mouse_down({menu_item->bounds.x + menu_item->bounds.w / 2,
                      menu_item->bounds.y + menu_item->bounds.h / 2});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/popup-menu");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto outer = session.active_mode->children[0];
  ASSERT_NE(outer, nullptr);
  ASSERT_EQ(outer->children.size(), 1u);
  auto inner = outer->children[0];
  ASSERT_NE(inner, nullptr);
  ASSERT_EQ(inner->children.size(), 1u);
  auto popup_item = inner->children[0];
  ASSERT_NE(popup_item, nullptr);
  ASSERT_FALSE(popup_item->children.empty());
  auto popup_text = popup_item->children[0];
  ASSERT_NE(popup_text, nullptr);
  ASSERT_TRUE(popup_text->effective_style.text.has_value());
  ASSERT_TRUE(popup_text->effective_style.text->font.has_value());
  EXPECT_EQ(*popup_text->effective_style.text->font, "font/classic-blue-font");
}

TEST_F(MenuTest, context_menu_opens_popup_at_mouse_position)
{
  runtime.eval(R"(
    (def context-menu
      {:settings {:mnemonics {:enabled false}}
       :items [{:label "Rename"
                :action :layer/rename}
               {:label "Delete"
                :action :layer/delete}]})

    (pixils/defmode context-target
      {:style {:width 100 :height 30}
       :on-mouse-up (fn [state event ctx]
                      (if (= (:button event) :right)
                        (do
                          (pixils.ui.menu/open-context-menu!
                           context-menu
                           (:global-position event)
                           state
                           ctx)
                          state)
                        state))})
  )");

  session.push_mode("context-target", Lisple::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 30};
  session.update_mode();
  session.render_mode();

  input().mouse_down({42, 17}, SDL_BUTTON_RIGHT);
  update_cycle();
  input().mouse_up({42, 17}, SDL_BUTTON_RIGHT);
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/context-menu");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto outer = session.active_mode->children[0];
  ASSERT_NE(outer, nullptr);
  EXPECT_EQ(outer->bounds.x, 42);
  EXPECT_EQ(outer->bounds.y, 17);
}

TEST_F(MenuTest, menu_bar_item_without_children_emits_action_without_opening_popup)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "Quit"
                :action :game/quit
                :payload {:source :menu}}]})

    (pixils/defmode root-mode
      {:on {:game/quit (fn [state event ctx]
                         (assoc (assoc state :quit true)
                                :payload (:payload event)))}
       :children [(pixils.ui.menu/make-menu
                   {}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", runtime.eval("{:quit false}"));
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto menu = session.active_mode->children[0];
  ASSERT_NE(menu, nullptr);
  ASSERT_EQ(menu->children.size(), 1u);
  auto menu_item = menu->children[0];
  ASSERT_NE(menu_item, nullptr);

  input().mouse_down({menu_item->bounds.x + menu_item->bounds.w / 2,
                      menu_item->bounds.y + menu_item->bounds.h / 2});
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
  EXPECT_EQ(session.active_mode->state->to_string(),
            "{:quit true :payload {:source :menu}}");
}

TEST_F(MenuTest, menu_bar_item_without_children_or_action_does_not_open_popup)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "Static"}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.menu/make-menu
                   {}
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
  ASSERT_EQ(menu->children.size(), 1u);
  auto menu_item = menu->children[0];
  ASSERT_NE(menu_item, nullptr);

  input().mouse_down({menu_item->bounds.x + menu_item->bounds.w / 2,
                      menu_item->bounds.y + menu_item->bounds.h / 2});
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
}
