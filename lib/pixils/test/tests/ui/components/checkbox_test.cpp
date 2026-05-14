#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>

using CheckboxTest = RenderFixture;

TEST_F(CheckboxTest, checkbox_toggles_bound_state_and_emits_change)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:show-grid? true})
       :on {:checkbox/change (fn [state event ctx]
                               (assoc state :last-change (:payload event)))}
       :children [(pixils.ui.checkbox/make
                   {:label "Show grid"
                    :checked? (pixils.ui/bind-state :show-grid?)})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto checkbox = session.active_mode->children[0];
  ASSERT_NE(checkbox, nullptr);
  EXPECT_EQ(checkbox->mode->name, "ui/checkbox");
  ASSERT_EQ(checkbox->children.size(), 2u);
  EXPECT_EQ(checkbox->children[0]->mode->name, "ui/checkbox-box");
  EXPECT_EQ(checkbox->children[1]->mode->name, "ui/checkbox-label");

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_up({5, 5});
  update_cycle();

  auto show_grid = Lisple::Dict::get_property(session.active_mode->state,
                                              Lisple::RTValue::keyword("show-grid?"));
  auto last_change = Lisple::Dict::get_property(session.active_mode->state,
                                                Lisple::RTValue::keyword("last-change"));
  ASSERT_NE(show_grid, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(show_grid->to_string(), "false");
  EXPECT_EQ(last_change->to_string(), "{:checked? false :value nil}");
}
