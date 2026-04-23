
#include "session_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using StateBindingTest = SessionFixture;

/**
 * Whole-path bind-state: child :state is (ui/bind-state :data), so child.state
 * must equal the value of :data in the parent state after init.
 */
TEST_F(StateBindingTest, whole_path_binding_projects_parent_value_into_child_state)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {})
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:data {:x 42}})
      :children [{:mode 'child-mode :id "child"
                  :state (pixils.ui/bind-state :data)}]
    })
  )");

  // When
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // Then - child state should be {:x 42} (the value at :data in parent)
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_EQ(child->state->type, Lisple::RTValue::Type::MAP);
  EXPECT_EQ(child->state->to_string(), "{:x 42}");
}

/** Map-binding bind-state: child :state is {:board (ui/bind-state :board)}, so
 *  child.state[:board] must equal parent.state[:board] after init. */
TEST_F(StateBindingTest, map_binding_overlays_bound_keys_from_parent_into_child_state)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {})
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:board {:x 7} :other 99})
      :children [{:mode 'child-mode :id "child"
                  :state {:board (pixils.ui/bind-state :board)}}]
    })
  )");

  // When
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // Then - child state should contain {:board {:x 7}}
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->state, nullptr);
  EXPECT_EQ(child->state->to_string(), "{:board {:x 7}}");
}

/**
 * Map-binding merge-back: when the child's update hook mutates :board,
 * the change must be written back to the parent's :board path.
 */
TEST_F(StateBindingTest, map_binding_propagates_child_state_changes_back_to_parent)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :update (fn [state ctx] (assoc state :board {:x 99}))
    })
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:board {:x 1}})
      :children [{:mode 'child-mode :id "child"
                  :state {:board (pixils.ui/bind-state :board)}}]
    })
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // When
  session.update_mode();

  // Then - parent's :board should reflect the child's update
  ASSERT_NE(session.active_mode->state, nullptr);
  EXPECT_EQ(session.active_mode->state->to_string(), "{:board {:x 99}}");
}
