#include "../../render_fixture.h"

#include <algorithm>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto menu = session.active_mode->children[0];
  ASSERT_NE(menu, nullptr);
  auto menu_scale = Roo::Dict::get_property(menu->state, Roo::keyword("menu-scale"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto menu = session.active_mode->children[0];
  ASSERT_NE(menu, nullptr);
  auto menu_scale = Roo::Dict::get_property(menu->state, Roo::keyword("menu-scale"));
  EXPECT_TRUE(!menu_scale || menu_scale->type == Roo::Value::Type::NIL);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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

TEST_F(MenuTest, classic_blue_menu_option_indicator_uses_theme_text)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/classic-blue
       :style {:layout {:direction :row}}
       :children [{:mode 'ui/menu-option-indicator
                   :state {:selected true}}
                  {:mode 'ui/menu-option-indicator
                   :state {:selected false}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto selected_indicator = session.active_mode->children[0];
  auto unselected_indicator = session.active_mode->children[1];
  ASSERT_NE(selected_indicator, nullptr);
  ASSERT_NE(unselected_indicator, nullptr);
  EXPECT_EQ(selected_indicator->mode->name, "ui/menu-option-indicator");
  EXPECT_EQ(unselected_indicator->mode->name, "ui/menu-option-indicator");
  ASSERT_TRUE(selected_indicator->effective_style.text.has_value());
  ASSERT_TRUE(selected_indicator->effective_style.text->font.has_value());
  EXPECT_EQ(*selected_indicator->effective_style.text->font, "font/classic-blue-font");
  ASSERT_TRUE(selected_indicator->effective_theme.default_variant.has_value());
  EXPECT_EQ(*selected_indicator->effective_theme.default_variant, "dark");
  ASSERT_TRUE(selected_indicator->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(selected_indicator->effective_theme.vars.at("dark").count(
                "menu-option-indicator") > 0);
  auto indicator_var =
    selected_indicator->effective_theme.vars.at("dark").at("menu-option-indicator");
  ASSERT_NE(indicator_var, nullptr);
  EXPECT_EQ(indicator_var->to_string(), "{:selected-text \"[x]\" :unselected-text \"[ ]\"}");
}

TEST_F(MenuTest, base_theme_generates_stock_menu_option_checkmark_image)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/menu-option-indicator
                   :state {:selected true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto id = runtime.eval(R"(
    (:id
     (head
      (filter (pixils.resource/list-images :base-theme)
              (fn [resource]
                (= (:id resource) :base-theme/menu-option-checkmark)))))
  )");
  auto source = runtime.eval(R"(
    (:source
     (head
      (filter (pixils.resource/list-images :base-theme)
              (fn [resource]
                (= (:id resource) :base-theme/menu-option-checkmark)))))
  )");
  auto width = runtime.eval(R"(
    (:w
     (:size
      (head
       (filter (pixils.resource/list-images :base-theme)
               (fn [resource]
                 (= (:id resource) :base-theme/menu-option-checkmark))))))
  )");
  auto height = runtime.eval(R"(
    (:h
     (:size
      (head
       (filter (pixils.resource/list-images :base-theme)
               (fn [resource]
                 (= (:id resource) :base-theme/menu-option-checkmark))))))
  )");

  ASSERT_NE(id, nullptr);
  ASSERT_NE(source, nullptr);
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  EXPECT_EQ(id->to_string(), ":base-theme/menu-option-checkmark");
  EXPECT_EQ(source->to_string(), ":generated");
  EXPECT_EQ(width->num().get_int(), 8);
  EXPECT_EQ(height->num().get_int(), 10);

  ASSERT_FALSE(render_target()->render_ops.empty());
  EXPECT_EQ(render_target()->render_ops.back().type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(render_target()->render_ops.back().rendered_rect.w, 8);
  EXPECT_EQ(render_target()->render_ops.back().rendered_rect.h, 10);
}

TEST_F(MenuTest, windows_3_menu_option_indicator_uses_styled_checkmark_symbol)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/menu-option-indicator
                   :state {:selected true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto indicator = session.active_mode->children[0];
  ASSERT_NE(indicator, nullptr);
  ASSERT_TRUE(indicator->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(
    indicator->effective_theme.vars.at("dark").count("menu-option-indicator") > 0);
  auto indicator_var =
    indicator->effective_theme.vars.at("dark").at("menu-option-indicator");
  ASSERT_NE(indicator_var, nullptr);
  EXPECT_EQ(indicator_var->to_string(), "{:selected-symbol :checkmark}");

  auto copy_ops = std::count_if(render_target()->render_ops.begin(),
                                render_target()->render_ops.end(),
                                [](const auto& op)
                                { return op.type == RenderOpType::RENDER_COPY; });
  EXPECT_EQ(copy_ops, 0);
}

TEST_F(MenuTest, popup_submenu_items_receive_theme_indicator)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"
                :items [{:label "Recent"
                         :items [{:label "Map"
                                  :action :file/open-recent}]}
                        {:label "Open"
                         :action :file/open}]}]})

    (pixils/defmode root-mode
      {:theme 'pixils/classic-blue
       :children [(pixils.ui.menu/make-menu
                   {}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto menu_item = session.active_mode->children[0]->children[0];
  input().mouse_down({menu_item->bounds.x + menu_item->bounds.w / 2,
                      menu_item->bounds.y + menu_item->bounds.h / 2});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "ui/popup-menu");
  auto inner = session.active_mode->children[0]->children[0];
  ASSERT_EQ(inner->children.size(), 2u);

  auto submenu_item = inner->children[0];
  auto leaf_item = inner->children[1];
  ASSERT_EQ(submenu_item->children.size(), 3u);
  ASSERT_EQ(leaf_item->children.size(), 3u);

  auto submenu_trailing = submenu_item->children[2];
  auto leaf_trailing = leaf_item->children[2];
  ASSERT_NE(submenu_trailing, nullptr);
  ASSERT_NE(leaf_trailing, nullptr);
  EXPECT_EQ(submenu_trailing->mode->name, "ui/menu-item-trailing");
  EXPECT_EQ(leaf_trailing->mode->name, "ui/menu-item-trailing");
  ASSERT_EQ(submenu_trailing->children.size(), 2u);
  ASSERT_EQ(leaf_trailing->children.size(), 2u);

  auto submenu_indicator = submenu_trailing->children[1];
  auto leaf_indicator = leaf_trailing->children[1];
  ASSERT_NE(submenu_indicator, nullptr);
  ASSERT_NE(leaf_indicator, nullptr);
  EXPECT_EQ(submenu_indicator->mode->name, "ui/menu-submenu-indicator");
  EXPECT_EQ(leaf_indicator->mode->name, "ui/menu-submenu-indicator");

  auto submenu_state =
    Roo::Dict::get_property(submenu_indicator->state, Roo::keyword("has-submenu"));
  auto leaf_state =
    Roo::Dict::get_property(leaf_indicator->state, Roo::keyword("has-submenu"));
  ASSERT_NE(submenu_state, nullptr);
  ASSERT_NE(leaf_state, nullptr);
  EXPECT_EQ(submenu_state->to_string(), "true");
  EXPECT_EQ(leaf_state->to_string(), "false");

  ASSERT_TRUE(submenu_indicator->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(submenu_indicator->effective_theme.vars.at("dark").count(
                "menu-submenu-indicator") > 0);
  auto indicator_var =
    submenu_indicator->effective_theme.vars.at("dark").at("menu-submenu-indicator");
  ASSERT_NE(indicator_var, nullptr);
  EXPECT_EQ(indicator_var->to_string(), "{:text \">\"}");
}

TEST_F(MenuTest, popup_items_share_marker_label_and_trailing_columns)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"
                :items [{:label "Open"
                         :action :file/open
                         :shortcut [:key/ctrl :key/o]}
                        {:label "Snap"
                         :type :toggle
                         :selected-path [:snap]
                         :action :view/snap
                         :shortcut :key/s}
                        {:label "Recent"
                         :items [{:label "Map"
                                  :action :file/open-recent}]}]}]})

    (pixils/defmode root-mode
      {:theme 'pixils/classic-blue
       :children [(pixils.ui.menu/make-menu
                   {}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", runtime.eval("{:snap true}"));
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  auto menu_item = session.active_mode->children[0]->children[0];
  input().mouse_down({menu_item->bounds.x + menu_item->bounds.w / 2,
                      menu_item->bounds.y + menu_item->bounds.h / 2});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "ui/popup-menu");
  auto inner = session.active_mode->children[0]->children[0];
  ASSERT_EQ(inner->children.size(), 3u);

  auto plain_item = inner->children[0];
  auto option_item = inner->children[1];
  auto submenu_item = inner->children[2];
  ASSERT_EQ(plain_item->children.size(), 3u);
  ASSERT_EQ(option_item->children.size(), 3u);
  ASSERT_EQ(submenu_item->children.size(), 3u);

  auto plain_marker = plain_item->children[0];
  auto option_marker = option_item->children[0];
  auto submenu_marker = submenu_item->children[0];
  auto plain_label = plain_item->children[1];
  auto option_label = option_item->children[1];
  auto submenu_label = submenu_item->children[1];
  auto plain_trailing = plain_item->children[2];
  auto option_trailing = option_item->children[2];
  auto submenu_trailing = submenu_item->children[2];

  EXPECT_EQ(plain_marker->mode->name, "ui/menu-option-indicator");
  EXPECT_EQ(option_marker->mode->name, "ui/menu-option-indicator");
  EXPECT_EQ(submenu_marker->mode->name, "ui/menu-option-indicator");
  EXPECT_EQ(plain_trailing->mode->name, "ui/menu-item-trailing");
  EXPECT_EQ(option_trailing->mode->name, "ui/menu-item-trailing");
  EXPECT_EQ(submenu_trailing->mode->name, "ui/menu-item-trailing");

  EXPECT_EQ(plain_marker->bounds.x, option_marker->bounds.x);
  EXPECT_EQ(plain_marker->bounds.x, submenu_marker->bounds.x);
  EXPECT_EQ(plain_marker->bounds.w, option_marker->bounds.w);
  EXPECT_EQ(plain_marker->bounds.w, submenu_marker->bounds.w);
  EXPECT_EQ(option_marker->bounds.w, 24);
  EXPECT_EQ(plain_marker->bounds.x - plain_item->bounds.x, 2);
  EXPECT_EQ(option_marker->bounds.x - option_item->bounds.x, 2);
  EXPECT_EQ(submenu_marker->bounds.x - submenu_item->bounds.x, 2);
  EXPECT_EQ(option_marker->bounds.y, option_label->bounds.y);
  EXPECT_EQ(option_marker->bounds.h, option_label->bounds.h);

  EXPECT_EQ(plain_label->bounds.x, option_label->bounds.x);
  EXPECT_EQ(plain_label->bounds.x, submenu_label->bounds.x);
  EXPECT_EQ(plain_label->bounds.x, plain_marker->bounds.x + plain_marker->bounds.w);
  EXPECT_EQ(option_label->bounds.x, option_marker->bounds.x + option_marker->bounds.w);
  EXPECT_EQ(submenu_label->bounds.x, submenu_marker->bounds.x + submenu_marker->bounds.w);

  const int plain_trailing_right = plain_trailing->bounds.x + plain_trailing->bounds.w;
  const int option_trailing_right = option_trailing->bounds.x + option_trailing->bounds.w;
  const int submenu_trailing_right = submenu_trailing->bounds.x + submenu_trailing->bounds.w;
  EXPECT_EQ(plain_trailing_right, option_trailing_right);
  EXPECT_EQ(plain_trailing_right, submenu_trailing_right);

  ASSERT_EQ(plain_trailing->children.size(), 2u);
  ASSERT_EQ(option_trailing->children.size(), 2u);
  ASSERT_EQ(submenu_trailing->children.size(), 2u);
  auto plain_shortcut = plain_trailing->children[0];
  auto option_shortcut = option_trailing->children[0];
  auto submenu_indicator = submenu_trailing->children[1];
  EXPECT_EQ(plain_shortcut->bounds.x + plain_shortcut->bounds.w, plain_trailing_right);
  EXPECT_EQ(option_shortcut->bounds.x + option_shortcut->bounds.w, option_trailing_right);
  EXPECT_EQ(submenu_indicator->bounds.x + submenu_indicator->bounds.w,
            submenu_trailing_right);

  EXPECT_GE(plain_trailing->bounds.x - (plain_label->bounds.x + plain_label->bounds.w),
            16);
  EXPECT_GE(option_trailing->bounds.x - (option_label->bounds.x + option_label->bounds.w),
            16);
  EXPECT_GE(submenu_trailing->bounds.x -
              (submenu_label->bounds.x + submenu_label->bounds.w),
            16);
}

TEST_F(MenuTest, windows_95_submenu_indicator_generates_chevron_images)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :style {:layout {:direction :row}}
       :children [{:mode 'ui/menu-submenu-indicator
                   :state {:has-submenu true
                           :highlighted false}}
                  {:mode 'ui/menu-submenu-indicator
                   :state {:has-submenu true
                           :highlighted true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  runtime.eval(R"(
    (defun resource-size [bundle id]
      (:size
       (head
        (filter (pixils.resource/list-images bundle)
                (fn [resource]
                  (= (:id resource) id))))))
  )");

  auto normal_source = runtime.eval(R"(
    (:source
     (head
      (filter (pixils.resource/list-images :windows-95-theme)
              (fn [resource]
                (= (:id resource) :windows-95-theme/scrollbar-arrow-right)))))
  )");
  auto highlighted_source = runtime.eval(R"(
    (:source
     (head
      (filter (pixils.resource/list-images :windows-95-theme)
              (fn [resource]
                (= (:id resource) :windows-95-theme/submenu-chevron-highlighted)))))
  )");

  ASSERT_NE(normal_source, nullptr);
  ASSERT_NE(highlighted_source, nullptr);
  EXPECT_EQ(normal_source->to_string(), ":generated");
  EXPECT_EQ(highlighted_source->to_string(), ":generated");
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-95-theme "
                         ":windows-95-theme/submenu-chevron-highlighted))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-95-theme "
                         ":windows-95-theme/submenu-chevron-highlighted))")
              ->num()
              .get_int(),
            7);
}

TEST_F(MenuTest, windows_95_dark_submenu_indicator_uses_bright_image_by_default)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :theme-variant :dark
       :children [{:mode 'ui/menu-submenu-indicator
                   :state {:has-submenu true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto indicator = session.active_mode->children[0];
  ASSERT_NE(indicator, nullptr);
  ASSERT_TRUE(indicator->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(indicator->effective_theme.vars.at("dark").count(
                "menu-submenu-indicator") > 0);
  auto indicator_var =
    indicator->effective_theme.vars.at("dark").at("menu-submenu-indicator");
  ASSERT_NE(indicator_var, nullptr);
  EXPECT_EQ(indicator_var->to_string(),
            "{:image :windows-95-theme/submenu-chevron-highlighted "
            ":highlighted-image :windows-95-theme/submenu-chevron-highlighted}");
}

TEST_F(MenuTest, windows_3_submenu_indicator_generates_chevron_images)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :style {:layout {:direction :row}}
       :children [{:mode 'ui/menu-submenu-indicator
                   :state {:has-submenu true
                           :highlighted false}}
                  {:mode 'ui/menu-submenu-indicator
                   :state {:has-submenu true
                           :highlighted true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  runtime.eval(R"(
    (defun resource-size [bundle id]
      (:size
       (head
        (filter (pixils.resource/list-images bundle)
                (fn [resource]
                  (= (:id resource) id))))))
  )");

  auto normal_source = runtime.eval(R"(
    (:source
     (head
      (filter (pixils.resource/list-images :windows-3-theme)
              (fn [resource]
                (= (:id resource) :windows-3-theme/chevron-right)))))
  )");
  auto highlighted_source = runtime.eval(R"(
    (:source
     (head
      (filter (pixils.resource/list-images :windows-3-theme)
              (fn [resource]
                (= (:id resource) :windows-3-theme/submenu-chevron-highlighted)))))
  )");

  ASSERT_NE(normal_source, nullptr);
  ASSERT_NE(highlighted_source, nullptr);
  EXPECT_EQ(normal_source->to_string(), ":generated");
  EXPECT_EQ(highlighted_source->to_string(), ":generated");
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-3-theme "
                         ":windows-3-theme/chevron-right))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-3-theme "
                         ":windows-3-theme/chevron-right))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-3-theme "
                         ":windows-3-theme/submenu-chevron-highlighted))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-3-theme "
                         ":windows-3-theme/submenu-chevron-highlighted))")
              ->num()
              .get_int(),
            7);
}

TEST_F(MenuTest, windows_3_dark_submenu_indicator_uses_bright_image_by_default)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/menu-submenu-indicator
                   :state {:has-submenu true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto indicator = session.active_mode->children[0];
  ASSERT_NE(indicator, nullptr);
  ASSERT_TRUE(indicator->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(indicator->effective_theme.vars.at("dark").count(
                "menu-submenu-indicator") > 0);
  auto indicator_var =
    indicator->effective_theme.vars.at("dark").at("menu-submenu-indicator");
  ASSERT_NE(indicator_var, nullptr);
  EXPECT_EQ(indicator_var->to_string(),
            "{:image :windows-3-theme/submenu-chevron-highlighted "
            ":highlighted-image :windows-3-theme/submenu-chevron-highlighted}");
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

  session.push_mode("context-target", Roo::Constant::NIL);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
