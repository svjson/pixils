#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>

#include <algorithm>

using ToggleButtonTest = RenderFixture;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }
} // namespace

TEST_F(ToggleButtonTest, pressed_button_state_image_stays_centered)
{
  SDLMock::prepared_surfaces["./brush.png"] = {10, 8};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:brush "brush.png"}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/toggle-button
                   :style {:width 28 :height 28}
                   :state {:image :icons/brush
                           :toggled? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
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

TEST_F(ToggleButtonTest, toggle_button_toggles_pressed_state_and_emits_change)
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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

TEST_F(ToggleButtonTest, toggle_button_group_selects_one_button_and_can_force_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:on {:toggle-button-group/change (fn [state event ctx]
                                          (assoc state :last-change (:payload event)))}
       :children [(pixils.ui.toggle-button/make-toggle-button-group
                   {:buttons [{:label "Draw" :value :draw}
                              {:label "Erase" :value :erase}]
                    :selection-required? true
                    :button-row-style {:layout {:direction :row}}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
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
