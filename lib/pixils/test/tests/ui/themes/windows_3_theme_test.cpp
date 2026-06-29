#include "../../render_fixture.h"
#include <pixils/color.h>
#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>

using Windows3ThemeTest = RenderFixture;

namespace
{
  std::shared_ptr<Pixils::Runtime::View> find_first_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;

    for (const auto& child : view->children)
    {
      if (auto found = find_first_mode(child, mode_name)) return found;
    }
    return nullptr;
  }

  void layout_active_mode(Roo::Runtime& runtime, Pixils::Runtime::Session& session)
  {
    Pixils::UI::layout_view_tree(
      session.active_mode,
      {0, 0, session.render_ctx.buffer_dim.w, session.render_ctx.buffer_dim.h},
      runtime,
      session.hook_args.render_args[1]);
  }

  void expect_border_color(const std::shared_ptr<Pixils::Runtime::View>& view,
                           const Pixils::Color& expected)
  {
    ASSERT_NE(view, nullptr);
    ASSERT_TRUE(view->effective_style.border.has_value());
    EXPECT_EQ(view->effective_style.border->top_color(), expected);
    EXPECT_EQ(view->effective_style.border->right_color(), expected);
    EXPECT_EQ(view->effective_style.border->bottom_color(), expected);
    EXPECT_EQ(view->effective_style.border->left_color(), expected);
  }

  void expect_border_trim(const std::shared_ptr<Pixils::Runtime::View>& view,
                          const Pixils::UI::Style::Trim& expected)
  {
    ASSERT_NE(view, nullptr);
    ASSERT_TRUE(view->effective_style.border.has_value());
    EXPECT_EQ(view->effective_style.border->top_trim(), expected);
    EXPECT_EQ(view->effective_style.border->right_trim(), expected);
    EXPECT_EQ(view->effective_style.border->bottom_trim(), expected);
    EXPECT_EQ(view->effective_style.border->left_trim(), expected);
  }

  void expect_border_thickness(const std::shared_ptr<Pixils::Runtime::View>& view,
                               int expected)
  {
    ASSERT_NE(view, nullptr);
    ASSERT_TRUE(view->effective_style.border.has_value());
    EXPECT_EQ(view->effective_style.border->top_thickness(), expected);
    EXPECT_EQ(view->effective_style.border->right_thickness(), expected);
    EXPECT_EQ(view->effective_style.border->bottom_thickness(), expected);
    EXPECT_EQ(view->effective_style.border->left_thickness(), expected);
  }

  Pixils::Color background_color(const std::shared_ptr<Pixils::Runtime::View>& view)
  {
    EXPECT_NE(view, nullptr);
    EXPECT_TRUE(view->effective_style.background.has_value());
    EXPECT_TRUE(view->effective_style.background->color.has_value());
    if (!view || !view->effective_style.background ||
        !view->effective_style.background->color)
    {
      return {};
    }
    return *view->effective_style.background->color;
  }
} // namespace

TEST_F(Windows3ThemeTest, dark_variant_uses_bright_default_border_for_fields)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/text-input
                   :state {:value "field"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto text_input = find_first_mode(session.active_mode, "ui/text-input");
  const Pixils::Color expected{0x69, 0x70, 0x76, 255};
  expect_border_color(text_input, expected);
}

TEST_F(Windows3ThemeTest, dark_variant_uses_bright_group_box_border)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [(pixils.ui.group-box/make
                   {:title "Options"
                    :style {:width 120 :height 80}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto frame = find_first_mode(session.active_mode, "ui/group-box-frame");
  const Pixils::Color expected{0x69, 0x70, 0x76, 255};
  expect_border_color(frame, expected);
}

TEST_F(Windows3ThemeTest, dark_variant_keeps_control_outer_borders_black)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/button
                   :style {:width 80 :height 24}
                   :state {:label "OK"}}
                  {:mode 'ui/scrollbar
                   :style {:width 17 :height 80}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  const Pixils::Color expected{0x00, 0x00, 0x00, 255};
  expect_border_color(find_first_mode(session.active_mode, "ui/button"), expected);
  expect_border_color(find_first_mode(session.active_mode, "ui/scrollbar"), expected);
}

TEST_F(Windows3ThemeTest, button_outer_border_trims_corner_pixels)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 80 :height 24}
                   :state {:label "OK"}}
                  {:mode 'ui/scrollbar
                   :style {:width 17 :height 80}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  expect_border_trim(find_first_mode(session.active_mode, "ui/button"), {1, 1});
  expect_border_trim(find_first_mode(session.active_mode, "ui/scrollbar"), {0, 0});
}

TEST_F(Windows3ThemeTest, dialog_frame_chrome_uses_blue_two_pixel_frame)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.window/make
                   {:kind :dialog-frame
                    :style {:width 80 :height 40}
                    :body [{:mode 'ui/text
                            :state {:value "Dialog frame"}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto window = find_first_mode(session.active_mode, "ui/window");
  const Pixils::Color expected{0x00, 0x0b, 0xc8, 255};
  expect_border_color(window, expected);
  expect_border_thickness(window, 2);
}

TEST_F(Windows3ThemeTest, progress_bar_uses_black_border_and_windows_blue_fill)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.progress-bar/make
                   {:style {:width 100 :height 14}
                    :value 42})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto progress = find_first_mode(session.active_mode, "ui/progress-bar");
  ASSERT_NE(progress, nullptr);
  expect_border_color(progress, {0x00, 0x00, 0x00, 255});
  expect_border_thickness(progress, 1);
  ASSERT_TRUE(progress->effective_style.text.has_value());
  ASSERT_TRUE(progress->effective_style.text->color.has_value());
  EXPECT_EQ(*progress->effective_style.text->color,
            (Pixils::Color{0x00, 0x0b, 0xc8, 255}));
}

TEST_F(Windows3ThemeTest, dark_variant_uses_bright_option_box_indicator)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/option-box-indicator
                   :style {:width 12 :height 12}
                   :state {:selected true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto indicator = find_first_mode(session.active_mode, "ui/option-box-indicator");
  ASSERT_NE(indicator, nullptr);
  ASSERT_TRUE(indicator->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(indicator->effective_theme.vars.at("dark").count("option-box-indicator") > 0);

  auto indicator_var = indicator->effective_theme.vars.at("dark").at("option-box-indicator");
  ASSERT_NE(indicator_var, nullptr);
  EXPECT_EQ(indicator_var->to_string(),
            "{:color {:__pixils-theme-var :text} :radius 5 :inner-radius 2 "
            ":pressed-thickness 2}");
}

TEST_F(Windows3ThemeTest, dark_variant_uses_bright_checkbox_mark_color)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [(pixils.ui.checkbox/make
                   {:label "Enabled"
                    :checked? true})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto box = find_first_mode(session.active_mode, "ui/checkbox-box");
  ASSERT_NE(box, nullptr);
  ASSERT_TRUE(box->effective_style.text.has_value());
  ASSERT_TRUE(box->effective_style.text->color.has_value());
  EXPECT_EQ(*box->effective_style.text->color,
            (Pixils::Color{0xdc, 0xe0, 0xe4, 255}));
}

TEST_F(Windows3ThemeTest, dark_variant_uses_bright_scrollbar_arrow_images)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/scrollbar
                   :style {:height 60}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto scrollbar = find_first_mode(session.active_mode, "ui/scrollbar");
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_TRUE(scrollbar->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(scrollbar->effective_theme.vars.at("dark").count("scrollbar-button-symbols") >
              0);

  auto symbols = scrollbar->effective_theme.vars.at("dark").at("scrollbar-button-symbols");
  ASSERT_NE(symbols, nullptr);
  EXPECT_EQ(symbols->to_string(),
            "{:up-image :windows-3-theme/scrollbar-arrow-up-dark "
            ":down-image :windows-3-theme/scrollbar-arrow-down-dark "
            ":left-image :windows-3-theme/scrollbar-arrow-left-dark "
            ":right-image :windows-3-theme/scrollbar-arrow-right-dark}");
}

TEST_F(Windows3ThemeTest,
       dark_variant_pressed_scrollbar_and_combo_controls_match_button_background)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :children [{:mode 'ui/button
                   :style {:width 80 :height 24}
                   :state {:label "OK" :pressed true}}
                  {:mode 'ui/scrollbar-button
                   :style {:width 17 :height 17}
                   :state {:direction :up :pressed true}}
                  {:mode 'ui/scrollbar-handle
                   :style {:width 17 :height 24}
                   :state {:axis :y :pressed true}}
                  {:mode 'ui/combo-box-button
                   :style {:width 17 :height 24}
                   :state {:direction :down :pressed true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  layout_active_mode(runtime, session);

  auto button_inner = find_first_mode(session.active_mode, "ui/button-inner");
  auto scrollbar_button = find_first_mode(session.active_mode, "ui/scrollbar-button");
  auto scrollbar_handle = find_first_mode(session.active_mode, "ui/scrollbar-handle");
  auto combo_button = find_first_mode(session.active_mode, "ui/combo-box-button");
  const Pixils::Color expected = background_color(button_inner);

  EXPECT_EQ(background_color(scrollbar_button), expected);
  EXPECT_EQ(background_color(scrollbar_handle), expected);
  EXPECT_EQ(background_color(combo_button), expected);
}
