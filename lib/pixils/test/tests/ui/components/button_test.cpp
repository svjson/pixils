#include "../../render_fixture.h"

#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>

#include <algorithm>
#include <array>

using ButtonTest = RenderFixture;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

  void layout_active_mode(Roo::Runtime& runtime, Pixils::Runtime::Session& session)
  {
    Pixils::UI::layout_view_tree(session.active_mode,
                                 {0,
                                  0,
                                  session.render_ctx.buffer_dim.w,
                                  session.render_ctx.buffer_dim.h},
                                 runtime,
                                 session.hook_args.render_args[1]);
  }
} // namespace

TEST_F(ButtonTest, button_can_use_fitted_background_image_from_style)
{
  SDLMock::prepared_surfaces["./brush.png"] = {8, 16};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:brush "brush.png"}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 24
                           :height 24
                           :background {:image :icons/brush
                                        :fit :contain
                                        :align :center}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_GE(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 6);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 12);
  EXPECT_EQ(ops[0].rendered_rect.h, 24);
}

TEST_F(ButtonTest, button_label_uses_base_theme_padding)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"O" {:x 0 :y 0 :w 4 :h 8}
                "K" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:text {:font :font/test-font}}
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  ASSERT_EQ(button->children.size(), 1u);
  auto button_inner = button->children[0];
  ASSERT_NE(button_inner, nullptr);
  ASSERT_FALSE(button_inner->mode->selector_modes.empty());
  EXPECT_EQ(button_inner->mode->selector_modes[0], "ui/button-inner");
  ASSERT_EQ(button_inner->children.size(), 1u);
  auto label = button_inner->children[0];
  ASSERT_NE(label, nullptr);
  ASSERT_EQ(label->mode->name, "ui/text");
  ASSERT_FALSE(label->mode->selector_modes.empty());
  EXPECT_EQ(label->mode->selector_modes[0], "ui/text");
  ASSERT_TRUE(label->effective_style.padding.has_value());
  EXPECT_GT(label->effective_style.padding->t, 0);
  EXPECT_GT(label->effective_style.padding->r, 0);
  EXPECT_GT(label->effective_style.padding->b, 0);
  EXPECT_GT(label->effective_style.padding->l, 0);
  auto label_value = get_key(label->state, "value");
  ASSERT_NE(label_value, nullptr);
  EXPECT_EQ(label_value->str(), "OK");
  EXPECT_GT(button->bounds.w, 20);
  EXPECT_GT(button->bounds.h, 10);
}

TEST_F(ButtonTest, button_label_natural_height_uses_effective_font_and_theme_padding)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont tall-font
      {:type :bitmap
       :resource :fonts/atlas
       :line-height 30
       :glyphs {"O" {:x 0 :y 0 :w 4 :h 8}
                "K" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:text {:font :font/tall-font}}
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  ASSERT_EQ(button->children.size(), 1u);
  auto inner = button->children[0];
  ASSERT_NE(inner, nullptr);
  ASSERT_EQ(inner->children.size(), 1u);
  auto label = inner->children[0];
  ASSERT_NE(label, nullptr);

  ASSERT_TRUE(label->effective_style.padding.has_value());
  ASSERT_TRUE(inner->effective_style.border.has_value());
  EXPECT_EQ(label->bounds.h,
            30 + label->effective_style.padding->t + label->effective_style.padding->b);
  EXPECT_EQ(inner->bounds.h,
            label->bounds.h + inner->effective_style.border->top_thickness() +
              inner->effective_style.border->bottom_thickness());
  EXPECT_EQ(button->bounds.h, inner->bounds.h);
  EXPECT_GT(button->bounds.h, 22);
}

TEST_F(ButtonTest, button_label_remeasures_when_theme_default_ttf_font_is_redeclared)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/deftheme ttf-button-theme
      {:defaults {:text {:font :font/test-font}}})
    (pixils/defmode root-mode
      {:theme 'ttf-button-theme
       :children [{:mode 'ui/button
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  auto small_button_bounds = button->bounds;
  auto small_label_bounds = button->children[0]->children[0]->bounds;

  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24})
  )");
  session.render_mode();

  auto large_button_bounds = button->bounds;
  auto large_label_bounds = button->children[0]->children[0]->bounds;

  EXPECT_GT(large_label_bounds.w, small_label_bounds.w);
  EXPECT_GT(large_label_bounds.h, small_label_bounds.h);
  EXPECT_GT(large_button_bounds.w, small_button_bounds.w);
  EXPECT_GT(large_button_bounds.h, small_button_bounds.h);
}

TEST_F(ButtonTest, windows3_button_label_remeasures_when_default_ttf_font_is_redeclared)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/deftheme ttf-default-theme
      {:defaults {:text {:font :font/test-font}}})
    (pixils/defmode root-mode
      {:theme ['pixils/windows-3 'ttf-default-theme]
       :children [{:mode 'ui/button
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  auto small_button_bounds = button->bounds;
  auto small_label_bounds = button->children[0]->children[0]->bounds;

  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24})
  )");
  session.render_mode();

  auto large_button_bounds = button->bounds;
  auto large_label_bounds = button->children[0]->children[0]->bounds;

  EXPECT_GT(large_label_bounds.w, small_label_bounds.w);
  EXPECT_GT(large_label_bounds.h, small_label_bounds.h);
  EXPECT_GT(large_button_bounds.w, small_button_bounds.w);
  EXPECT_GT(large_button_bounds.h, small_button_bounds.h);
}

TEST_F(ButtonTest, fixed_height_button_clips_oversized_label_to_inner_surface)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont tall-font
      {:type :bitmap
       :resource :fonts/atlas
       :line-height 30
       :glyphs {"O" {:x 0 :y 0 :w 4 :h 8}
                "K" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 32
                           :height 18
                           :max-height 18
                           :text {:font :font/tall-font}}
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  ASSERT_EQ(button->children.size(), 1u);
  auto inner = button->children[0];
  ASSERT_NE(inner, nullptr);
  ASSERT_EQ(inner->children.size(), 1u);
  auto label = inner->children[0];
  ASSERT_NE(label, nullptr);

  ASSERT_TRUE(inner->effective_style.clip.has_value());
  EXPECT_TRUE(*inner->effective_style.clip);

  auto inner_content = inner->effective_style.content_rect(inner->bounds);
  EXPECT_EQ(button->bounds.h, 18);
  EXPECT_EQ(inner->bounds.h, 18);
  EXPECT_GT(label->bounds.h, inner_content.h);
  EXPECT_GT(label->bounds.y + label->bounds.h, inner_content.y + inner_content.h);
}

TEST_F(ButtonTest, button_state_image_is_centered_inside_button_inner)
{
  SDLMock::prepared_surfaces["./brush.png"] = {10, 8};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:brush "brush.png"}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 28 :height 28}
                   :state {:image :icons/brush}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto& ops = render_target()->render_ops;
  auto copy = std::find_if(ops.begin(), ops.end(), [](const auto& op) {
    return op.type == RenderOpType::RENDER_COPY;
  });
  ASSERT_NE(copy, ops.end());
  EXPECT_EQ(copy->rendered_rect.x, 8);
  EXPECT_EQ(copy->rendered_rect.y, 9);
  EXPECT_EQ(copy->rendered_rect.w, 10);
  EXPECT_EQ(copy->rendered_rect.h, 8);
}

TEST_F(ButtonTest, button_state_image_map_accepts_source_rect_and_offset)
{
  SDLMock::prepared_surfaces["./icons.png"] = {32, 16};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:sheet "icons.png"}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 28 :height 28}
                   :state {:image {:image :icons/sheet
                                    :source {:x 16 :y 0 :w 8 :h 8}
                                    :offset {:x 2 :y -1}}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto& ops = render_target()->render_ops;
  auto copy = std::find_if(ops.begin(), ops.end(), [](const auto& op) {
    return op.type == RenderOpType::RENDER_COPY;
  });
  ASSERT_NE(copy, ops.end());
  EXPECT_EQ(copy->rendered_rect.x, 11);
  EXPECT_EQ(copy->rendered_rect.y, 8);
  EXPECT_EQ(copy->rendered_rect.w, 8);
  EXPECT_EQ(copy->rendered_rect.h, 8);
}

TEST_F(ButtonTest, disabled_button_can_style_only_state_image_opacity)
{
  SDLMock::prepared_surfaces["./brush.png"] = {10, 8};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:brush "brush.png"}})
    (pixils/deftheme dim-disabled-button-theme
      {:styles {'ui/button-inner:disabled {:image {:opacity 0.35}}}})
    (pixils/defmode root-mode
      {:theme 'dim-disabled-button-theme
       :children [{:mode 'ui/button
                   :style {:width 28 :height 28}
                   :state {:image :icons/brush
                           :disabled? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  ASSERT_EQ(button->children.size(), 1u);
  auto button_inner = button->children[0];
  ASSERT_NE(button_inner, nullptr);
  ASSERT_NE(button_inner->effective_style.image, std::nullopt);
  ASSERT_NE(button_inner->effective_style.image->opacity, std::nullopt);
  EXPECT_FLOAT_EQ(*button_inner->effective_style.image->opacity, 0.35f);
  EXPECT_EQ(button_inner->effective_style.opacity, std::nullopt);
}

TEST_F(ButtonTest, button_state_image_map_can_override_pressed_offset)
{
  SDLMock::prepared_surfaces["./icons.png"] = {32, 16};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:sheet "icons.png"}})
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 28 :height 28}
                   :state {:pressed true
                           :image {:image :icons/sheet
                                    :source {:x 0 :y 0 :w 8 :h 8}
                                    :pressed-offset {:x 3 :y 4}}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto& ops = render_target()->render_ops;
  auto copy = std::find_if(ops.begin(), ops.end(), [](const auto& op) {
    return op.type == RenderOpType::RENDER_COPY;
  });
  ASSERT_NE(copy, ops.end());
  EXPECT_EQ(copy->rendered_rect.x, 11);
  EXPECT_EQ(copy->rendered_rect.y, 12);
  EXPECT_EQ(copy->rendered_rect.w, 8);
  EXPECT_EQ(copy->rendered_rect.h, 8);
}

TEST_F(ButtonTest, disabled_button_does_not_fire_click_handler)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 40 :height 24}
                   :state {:label "OK"
                           :clicks 0
                           :disabled? true}
                   :on-click (fn [state event ctx]
                               (assoc state :clicks (+ (:clicks state) 1)))}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_up({10, 10});
  update_cycle();

  auto clicks = get_key(session.active_mode->children[0]->state, "clicks");
  ASSERT_NE(clicks, nullptr);
  EXPECT_EQ(clicks->to_string(), "0");
}

TEST_F(ButtonTest, focused_button_enter_fires_click_handler_as_left_click)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:focused? false})
       :update (fn [state ctx]
                 (do
                   (when (not (:focused? state))
                     (pixils.ui/focus! (head (pixils.ui/children ctx))))
                   (assoc state :focused? true)))
       :children [{:mode 'ui/button
                   :style {:width 40 :height 24}
                   :state {:label "OK"
                           :clicks 0
                           :last-button nil}
                   :on-click (fn [state event ctx]
                               (assoc (assoc state :clicks (+ (:clicks state) 1))
                                      :last-button
                                      (:button event)))}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  EXPECT_TRUE(button->interaction.focused);

  input().key_down(SDLK_RETURN);
  update_cycle();
  input().key_up(SDLK_RETURN);
  update_cycle();

  auto clicks = get_key(button->state, "clicks");
  auto last_button = get_key(button->state, "last-button");
  ASSERT_NE(clicks, nullptr);
  ASSERT_NE(last_button, nullptr);
  EXPECT_EQ(clicks->to_string(), "1");
  EXPECT_EQ(last_button->to_string(), ":left");
}

TEST_F(ButtonTest, focused_button_keypad_enter_fires_click_handler)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:focused? false})
       :update (fn [state ctx]
                 (do
                   (when (not (:focused? state))
                     (pixils.ui/focus! (head (pixils.ui/children ctx))))
                   (assoc state :focused? true)))
       :children [{:mode 'ui/button
                   :style {:width 40 :height 24}
                   :state {:label "OK"
                           :clicks 0}
                   :on-click (fn [state event ctx]
                               (assoc state :clicks (+ (:clicks state) 1)))}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  input().key_down(SDLK_KP_ENTER);
  update_cycle();
  input().key_up(SDLK_KP_ENTER);
  update_cycle();

  auto button = session.active_mode->children[0];
  auto clicks = get_key(button->state, "clicks");
  ASSERT_NE(clicks, nullptr);
  EXPECT_EQ(clicks->to_string(), "1");
}

TEST_F(ButtonTest, focused_disabled_button_enter_does_not_fire_click_handler)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:focused? false})
       :update (fn [state ctx]
                 (do
                   (when (not (:focused? state))
                     (pixils.ui/focus! (head (pixils.ui/children ctx))))
                   (assoc state :focused? true)))
       :children [{:mode 'ui/button
                   :style {:width 40 :height 24}
                   :state {:label "OK"
                           :clicks 0
                           :disabled? true}
                   :on-click (fn [state event ctx]
                               (assoc state :clicks (+ (:clicks state) 1)))}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  input().key_down(SDLK_RETURN);
  update_cycle();
  input().key_up(SDLK_RETURN);
  update_cycle();

  auto button = session.active_mode->children[0];
  auto clicks = get_key(button->state, "clicks");
  ASSERT_NE(clicks, nullptr);
  EXPECT_EQ(clicks->to_string(), "0");
}

TEST_F(ButtonTest, make_button_builds_button_child_with_state_and_handlers)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.button/make
                   {:label "Run"
                    :value :run
                    :class :ui/toolbar-button
                    :style {:width 44 :height 24}
                    :state {:clicks 0}
                    :on-click (fn [state event ctx]
                                (assoc state :clicks (+ (:clicks state) 1)))})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  EXPECT_EQ(button->mode->name, "ui/button");
  ASSERT_FALSE(button->mode->class_names.empty());
  EXPECT_EQ(button->mode->class_names[0], "ui/toolbar-button");
  EXPECT_EQ(button->bounds.w, 44);
  EXPECT_EQ(button->bounds.h, 24);
  EXPECT_EQ(get_key(button->state, "label")->str(), "Run");
  EXPECT_EQ(get_key(button->state, "value")->to_string(), ":run");

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_up({10, 10});
  update_cycle();

  auto clicks = get_key(button->state, "clicks");
  ASSERT_NE(clicks, nullptr);
  EXPECT_EQ(clicks->to_string(), "1");
}

TEST_F(ButtonTest, windows_3_pressed_button_keeps_row_layout_stable)
{
  runtime.eval(R"(
    (defun test-button-row [pressed-index]
      {:style {:width 260
               :layout {:direction :row
                        :gap :space-between}}
       :children [(pixils.ui.button/make
                   {:label "Start New Game"
                    :state {:pressed (= pressed-index 0)}})
                  (pixils.ui.button/make
                   {:label "Load Saved Game"
                    :state {:pressed (= pressed-index 1)}})
                  (pixils.ui.button/make
                   {:label "Overview"
                    :state {:pressed (= pressed-index 2)}})]})

    (pixils/defmode unpressed-mode
      {:theme 'pixils/windows-3
       :style {:width 320
               :height 80}
       :children [(test-button-row nil)]})

    (pixils/defmode first-pressed-mode
      {:theme 'pixils/windows-3
       :style {:width 320
               :height 80}
       :children [(test-button-row 0)]})

    (pixils/defmode second-pressed-mode
      {:theme 'pixils/windows-3
       :style {:width 320
               :height 80}
       :children [(test-button-row 1)]})

    (pixils/defmode third-pressed-mode
      {:theme 'pixils/windows-3
       :style {:width 320
               :height 80}
       :children [(test-button-row 2)]})
  )");

  struct RowSnapshot
  {
    std::array<Pixils::Rect, 3> button_bounds;
    std::array<Pixils::Rect, 3> label_bounds;
    std::array<Pixils::UI::Style::Insets, 3> label_padding;
  };

  auto capture = [&](const std::string& mode_name) -> RowSnapshot
  {
    session.push_mode(mode_name, Roo::Constant::NIL);
    layout_active_mode(runtime, session);

    RowSnapshot snapshot;
    EXPECT_EQ(session.active_mode->children.size(), 1u);
    if (session.active_mode->children.size() != 1u) return snapshot;
    auto row = session.active_mode->children[0];
    EXPECT_NE(row, nullptr);
    if (!row) return snapshot;
    EXPECT_EQ(row->children.size(), 3u);
    if (row->children.size() != 3u) return snapshot;
    for (size_t i = 0; i < row->children.size(); i++)
    {
      auto button = row->children[i];
      EXPECT_NE(button, nullptr);
      if (!button) return snapshot;
      EXPECT_EQ(button->children.size(), 1u);
      if (button->children.size() != 1u) return snapshot;
      auto inner = button->children[0];
      EXPECT_NE(inner, nullptr);
      if (!inner) return snapshot;
      EXPECT_EQ(inner->children.size(), 1u);
      if (inner->children.size() != 1u) return snapshot;
      auto label = button->children[0]->children[0];
      EXPECT_NE(label, nullptr);
      if (!label) return snapshot;
      EXPECT_TRUE(label->effective_style.padding.has_value());
      if (!label->effective_style.padding) return snapshot;
      snapshot.button_bounds[i] = button->bounds;
      snapshot.label_bounds[i] = label->bounds;
      snapshot.label_padding[i] = *label->effective_style.padding;
    }
    return snapshot;
  };

  auto unpressed = capture("unpressed-mode");
  std::array<std::string, 3> pressed_modes = {
    "first-pressed-mode",
    "second-pressed-mode",
    "third-pressed-mode",
  };

  for (size_t pressed_index = 0; pressed_index < pressed_modes.size(); pressed_index++)
  {
    auto pressed = capture(pressed_modes[pressed_index]);
    for (size_t i = 0; i < pressed.button_bounds.size(); i++)
    {
      EXPECT_EQ(pressed.button_bounds[i].x, unpressed.button_bounds[i].x);
      EXPECT_EQ(pressed.button_bounds[i].y, unpressed.button_bounds[i].y);
      EXPECT_EQ(pressed.button_bounds[i].w, unpressed.button_bounds[i].w);
      EXPECT_EQ(pressed.button_bounds[i].h, unpressed.button_bounds[i].h);
      if (i == pressed_index)
      {
        EXPECT_EQ(pressed.label_bounds[i].x + pressed.label_padding[i].l,
                  unpressed.label_bounds[i].x + unpressed.label_padding[i].l + 1);
        EXPECT_EQ(pressed.label_bounds[i].y + pressed.label_padding[i].t,
                  unpressed.label_bounds[i].y + unpressed.label_padding[i].t + 1);
        EXPECT_EQ(pressed.label_padding[i].t, unpressed.label_padding[i].t + 2);
        EXPECT_EQ(pressed.label_padding[i].r, unpressed.label_padding[i].r - 2);
        EXPECT_EQ(pressed.label_padding[i].b, unpressed.label_padding[i].b - 2);
        EXPECT_EQ(pressed.label_padding[i].l, unpressed.label_padding[i].l + 2);
      }
      else
      {
        EXPECT_EQ(pressed.label_bounds[i].x, unpressed.label_bounds[i].x);
        EXPECT_EQ(pressed.label_bounds[i].y, unpressed.label_bounds[i].y);
        EXPECT_EQ(pressed.label_padding[i].t, unpressed.label_padding[i].t);
        EXPECT_EQ(pressed.label_padding[i].r, unpressed.label_padding[i].r);
        EXPECT_EQ(pressed.label_padding[i].b, unpressed.label_padding[i].b);
        EXPECT_EQ(pressed.label_padding[i].l, unpressed.label_padding[i].l);
      }
    }
  }
}

TEST_F(ButtonTest, windows_3_pressed_button_offsets_state_image)
{
  SDLMock::prepared_surfaces["./brush.png"] = {10, 8};
  SDLMock::prepared_surfaces["./face.png"] = {20, 20};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:brush "brush.png"
                                      :face "face.png"}})
    (pixils/defmode unpressed-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 28 :height 28}
                   :state {:image :icons/brush}}]})

    (pixils/defmode pressed-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 28 :height 28}
                   :state {:image :icons/brush
                           :pressed true}}]})

    (pixils/defmode unpressed-face-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 24 :height 24}
                   :state {:image :icons/face}}]})

    (pixils/defmode pressed-face-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 24 :height 24}
                   :state {:image :icons/face
                           :pressed true}}]})

    (pixils/defmode interactive-face-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/button
                   :style {:width 24 :height 24}
                   :state {:image :icons/face}}]})
  )");

  auto rendered_image_rect = [&]() -> SDL_Rect
  {
    render_target()->render_ops.clear();
    session.render_mode();

    auto& ops = render_target()->render_ops;
    auto copy = std::find_if(ops.begin(), ops.end(), [](const auto& op) {
      return op.type == RenderOpType::RENDER_COPY;
    });
    EXPECT_NE(copy, ops.end());
    if (copy == ops.end()) return {0, 0, 0, 0};
    auto button = session.active_mode->children[0];
    if (!button || button->children.empty()) return copy->rendered_rect;
    auto inner = button->children[0];
    if (!inner) return copy->rendered_rect;
    auto content = inner->effective_style.content_rect(inner->bounds);
    return {content.x + copy->rendered_rect.x,
            content.y + copy->rendered_rect.y,
            copy->rendered_rect.w,
            copy->rendered_rect.h};
  };

  auto render_image_rect = [&](const std::string& mode_name) -> SDL_Rect
  {
    session.push_mode(mode_name, Roo::Constant::NIL);
    return rendered_image_rect();
  };

  auto unpressed = render_image_rect("unpressed-mode");
  auto pressed = render_image_rect("pressed-mode");

  EXPECT_EQ(pressed.x, unpressed.x + 1);
  EXPECT_EQ(pressed.y, unpressed.y + 1);
  EXPECT_EQ(pressed.w, unpressed.w);
  EXPECT_EQ(pressed.h, unpressed.h);

  auto unpressed_face = render_image_rect("unpressed-face-mode");
  auto pressed_face = render_image_rect("pressed-face-mode");

  EXPECT_EQ(pressed_face.x, unpressed_face.x + 1);
  EXPECT_EQ(pressed_face.y, unpressed_face.y + 1);
  EXPECT_EQ(pressed_face.w, unpressed_face.w);
  EXPECT_EQ(pressed_face.h, unpressed_face.h);

  session.push_mode("interactive-face-mode", Roo::Constant::NIL);
  auto interactive_unpressed = rendered_image_rect();
  input().mouse_down({10, 10});
  update_cycle();
  auto interactive_pressed = rendered_image_rect();

  EXPECT_EQ(interactive_pressed.x, interactive_unpressed.x + 1);
  EXPECT_EQ(interactive_pressed.y, interactive_unpressed.y + 1);
  EXPECT_EQ(interactive_pressed.w, interactive_unpressed.w);
  EXPECT_EQ(interactive_pressed.h, interactive_unpressed.h);
}

TEST_F(ButtonTest, windows_3_window_minimize_button_keeps_inner_button_border_only)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.window/make
                   {:title-bar {:title "Window"}
                    :style {:width 120}
                    :body []})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  layout_active_mode(runtime, session);

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto window = session.active_mode->children[0];
  ASSERT_NE(window, nullptr);
  ASSERT_GE(window->children.size(), 1u);
  auto title_bar = window->children[0];
  ASSERT_NE(title_bar, nullptr);
  ASSERT_EQ(title_bar->children.size(), 3u);
  auto minimize = title_bar->children[2];
  ASSERT_NE(minimize, nullptr);
  EXPECT_EQ(minimize->mode->name, "ui/window-minimize-button");
  ASSERT_TRUE(minimize->effective_style.border.has_value());
  EXPECT_EQ(minimize->effective_style.border->top_thickness(), 0);
  EXPECT_EQ(minimize->effective_style.border->right_thickness(), 0);
  EXPECT_EQ(minimize->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(minimize->effective_style.border->left_thickness(), 1);
  ASSERT_TRUE(minimize->effective_style.border->left_color().has_value());
  EXPECT_EQ(*minimize->effective_style.border->left_color(), (Pixils::Color{0, 0, 0, 255}));

  ASSERT_EQ(minimize->children.size(), 1u);
  auto inner = minimize->children[0];
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(inner->mode->name, "ui/button-inner");
  ASSERT_TRUE(inner->effective_style.background.has_value());
  const auto& background = *inner->effective_style.background;
  ASSERT_TRUE(background.image.has_value());
  EXPECT_EQ(background.image->first, "pixils");
  EXPECT_EQ(background.image->second, "win311-minimize-button");
  ASSERT_TRUE(background.align_x.has_value());
  ASSERT_TRUE(background.align_y.has_value());
  EXPECT_EQ(*background.align_x, Pixils::UI::Style::Background::Align::CENTER);
  EXPECT_EQ(*background.align_y, Pixils::UI::Style::Background::Align::CENTER);
  ASSERT_TRUE(inner->effective_style.border.has_value());
  EXPECT_EQ(inner->effective_style.border->top_thickness(), 2);
  EXPECT_EQ(inner->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(inner->effective_style.border->bottom_thickness(), 2);
  EXPECT_EQ(inner->effective_style.border->left_thickness(), 2);

  runtime.eval(R"(
    (pixils/defmode pressed-root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/window-minimize-button
                   :state {:force-pressed? true}}]})
  )");

  session.push_mode("pressed-root-mode", Roo::Constant::NIL);
  update_cycle();
  layout_active_mode(runtime, session);

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pressed_minimize = session.active_mode->children[0];
  ASSERT_NE(pressed_minimize, nullptr);
  ASSERT_EQ(pressed_minimize->children.size(), 1u);
  auto pressed_inner = pressed_minimize->children[0];
  ASSERT_NE(pressed_inner, nullptr);
  ASSERT_TRUE(pressed_inner->effective_style.background.has_value());
  const auto& pressed_background = *pressed_inner->effective_style.background;
  ASSERT_TRUE(pressed_background.image.has_value());
  EXPECT_EQ(pressed_background.image->first, "pixils");
  EXPECT_EQ(pressed_background.image->second, "win311-minimize-button");
  ASSERT_TRUE(pressed_background.offset.has_value());
  EXPECT_EQ(pressed_background.offset->round_x(), 1);
  EXPECT_EQ(pressed_background.offset->round_y(), 1);
}

TEST_F(ButtonTest, button_label_has_natural_size_inside_window_body)
{
  SDLMock::prepared_surfaces["./font.png"] = {32, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"C" {:x 0 :y 0 :w 4 :h 8}
                "r" {:x 4 :y 0 :w 4 :h 8}
                "e" {:x 8 :y 0 :w 4 :h 8}
                "a" {:x 12 :y 0 :w 4 :h 8}
                "t" {:x 16 :y 0 :w 4 :h 8}}})
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
                   :style {:text {:font :font/test-font}}
                   :state {:label "Create"}}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Window"}
                    :style {:width 300}
                    :body [{:mode 'form-body}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto window = session.active_mode->children[0];
  auto body = window->children[1];
  auto form = body->children[0];
  auto button = form->children[0];
  EXPECT_GT(button->bounds.w, 40);
  EXPECT_GT(button->bounds.h, 10);
}

TEST_F(ButtonTest, button_label_has_natural_size_inside_pushed_dialog_frame)
{
  SDLMock::prepared_surfaces["./font.png"] = {32, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"C" {:x 0 :y 0 :w 4 :h 8}
                "r" {:x 4 :y 0 :w 4 :h 8}
                "e" {:x 8 :y 0 :w 4 :h 8}
                "a" {:x 12 :y 0 :w 4 :h 8}
                "t" {:x 16 :y 0 :w 4 :h 8}}})
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
                   :style {:text {:font :font/test-font}}
                   :state {:label "Create"}}]})

    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils/push-mode!
                  'ui/dialog-frame
                  {}
                  {:children [(pixils.ui.window/make
                               {:title-bar {:title "Window"}
                                :style {:width 300}
                                :body [{:mode 'form-body}]})]})
                 state))})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.process_messages();
  session.render_mode();

  auto window = session.active_mode->children[0];
  auto body = window->children[1];
  auto form = body->children[0];
  auto button = form->children[0];
  EXPECT_GT(window->bounds.h, 0);
  EXPECT_GT(button->bounds.w, 40);
  EXPECT_GT(button->bounds.h, 10);
}
