#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>

using ButtonTest = RenderFixture;

namespace
{
  Lisple::sptr_val get_key(const Lisple::sptr_val& value, const std::string& key)
  {
    return Lisple::Dict::get_property(value, Lisple::keyword(key));
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

  session.push_mode("root-mode", Lisple::Constant::NIL);

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
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
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

  session.push_mode("root-mode", Lisple::Constant::NIL);
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

TEST_F(ButtonTest, toggle_button_toggles_pressed_state_and_emits_change)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:on {:toggle-button/change (fn [state event ctx]
                                    (assoc state :last-change (:payload event)))}
       :children [{:mode 'ui/toggle-button
                   :style {:width 64 :height 24}
                   :state {:label "Grid"
                           :value :grid}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  EXPECT_EQ(button->mode->name, "ui/toggle-button");
  ASSERT_GE(button->mode->selector_modes.size(), 2u);
  EXPECT_EQ(button->mode->selector_modes[1], "ui/button");

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_up({10, 10});
  update_cycle();
  update_cycle();

  auto toggled = get_key(button->state, "toggled?");
  auto pressed = get_key(button->state, "pressed");
  auto last_change = get_key(session.active_mode->state, "last-change");
  ASSERT_NE(toggled, nullptr);
  ASSERT_NE(pressed, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(toggled->to_string(), "true");
  EXPECT_EQ(pressed->to_string(), "true");
  EXPECT_EQ(last_change->to_string(), "{:toggled? true :value :grid :index nil}");

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_up({10, 10});
  update_cycle();
  update_cycle();

  toggled = get_key(button->state, "toggled?");
  pressed = get_key(button->state, "pressed");
  ASSERT_NE(toggled, nullptr);
  ASSERT_NE(pressed, nullptr);
  EXPECT_EQ(toggled->to_string(), "false");
  EXPECT_EQ(pressed->to_string(), "false");
}

TEST_F(ButtonTest, toggle_button_group_selects_one_button_and_can_force_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:on {:toggle-button-group/change (fn [state event ctx]
                                          (assoc state :last-change (:payload event)))}
       :children [(pixils.ui.button/make-toggle-button-group
                   {:buttons [{:label "Draw" :value :draw}
                              {:label "Erase" :value :erase}]
                    :selection-required? true
                    :button-row-style {:layout {:direction :row}}})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto group = session.active_mode->children[0];
  ASSERT_NE(group, nullptr);
  EXPECT_EQ(group->mode->name, "ui/toggle-button-group");
  ASSERT_EQ(group->children.size(), 1u);
  auto row = group->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);

  auto draw = row->children[0];
  auto erase = row->children[1];
  ASSERT_NE(draw, nullptr);
  ASSERT_NE(erase, nullptr);
  EXPECT_EQ(get_key(draw->state, "toggled?")->to_string(), "true");
  EXPECT_EQ(get_key(erase->state, "toggled?")->to_string(), "false");

  input().mouse_down(
    {erase->bounds.x + (erase->bounds.w / 2), erase->bounds.y + (erase->bounds.h / 2)});
  update_cycle();
  input().mouse_up(
    {erase->bounds.x + (erase->bounds.w / 2), erase->bounds.y + (erase->bounds.h / 2)});
  update_cycle();
  update_cycle();

  auto selected = get_key(group->state, "selected");
  auto last_change = get_key(session.active_mode->state, "last-change");
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(selected->to_string(), ":erase");
  EXPECT_EQ(last_change->to_string(), "{:selected :erase :value :erase :index 1}");

  row = group->children[0];
  draw = row->children[0];
  erase = row->children[1];
  EXPECT_EQ(get_key(draw->state, "toggled?")->to_string(), "false");
  EXPECT_EQ(get_key(erase->state, "toggled?")->to_string(), "true");

  input().mouse_down(
    {erase->bounds.x + (erase->bounds.w / 2), erase->bounds.y + (erase->bounds.h / 2)});
  update_cycle();
  input().mouse_up(
    {erase->bounds.x + (erase->bounds.w / 2), erase->bounds.y + (erase->bounds.h / 2)});
  update_cycle();
  update_cycle();

  selected = get_key(group->state, "selected");
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":erase");
  EXPECT_EQ(get_key(erase->state, "toggled?")->to_string(), "true");
}

TEST_F(ButtonTest, button_label_has_natural_size_inside_window_body)
{
  runtime.eval(R"(
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
                   :state {:label "Create"}}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Window"}
                    :style {:width 300}
                    :body [{:mode 'form-body}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
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
  runtime.eval(R"(
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
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

  session.push_mode("root-mode", Lisple::Constant::NIL);
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
