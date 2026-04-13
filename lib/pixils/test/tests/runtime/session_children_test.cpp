
#include "../render_fixture.h"
#include "session_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

class SessionChildrenTest : public RenderFixture
{
};

class SessionStateTreeTest : public SessionFixture
{
};

TEST_F(SessionChildrenTest, child_mode_lambda_render_hook_is_called)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  (pixils.point/point 0 0)
                  (pixils.point/point 10 10)
                  {:fill true}))
    })
    (pixils/defmode parent-mode {:children [{:mode child-mode}]})
  )");

  // When
  session.push_mode("parent-mode", Lisple::Constant::NIL);
  ASSERT_NO_THROW(session.render_mode());

  // Then
  EXPECT_FALSE(render_target()->render_ops.empty());
}

TEST_F(SessionChildrenTest, child_mode_symbol_render_hook_is_resolved_and_called)
{
  // Given
  runtime.eval(R"(
    (defun child-render! [state ctx]
      (pixils.render/rect!
        (pixils.point/point 0 0)
        (pixils.point/point 10 10)
        {:fill true}))
    (pixils/defmode child-mode {:render child-render!})
    (pixils/defmode parent-mode {:children [{:mode child-mode}]})
  )");

  // When
  session.push_mode("parent-mode", Lisple::Constant::NIL);
  ASSERT_NO_THROW(session.render_mode());

  // Then
  EXPECT_FALSE(render_target()->render_ops.empty());
}

TEST_F(SessionChildrenTest, child_mode_render_hook_receives_render_context)
{
  // Given - render_ctx.buffer_dim is {320, 200} per RenderFixture
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :render (fn [state ctx]
                (let [dim (:buffer-size ctx)]
                  (pixils.render/rect!
                    (pixils.point/point 0 0)
                    (pixils.point/point (:w dim) (:h dim))
                    {:fill true})))
    })
    (pixils/defmode parent-mode {:children [{:mode child-mode}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then - child renders into its own viewport (full parent area since only one fill child)
  EXPECT_FALSE(render_target()->render_ops.empty());
}

TEST_F(SessionStateTreeTest, push_mode_merges_child_init_state_into_parent_state)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {:init (fn [state ctx] {:value 42})})
    (pixils/defmode parent-mode {:children [{:mode child-mode}]})
  )");

  // When
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // Then - parent state map has child state at the auto-generated id key
  auto child_state = Lisple::Dict::get_property(session.active_mode.state,
                                                Lisple::RTValue::keyword("child-mode-0"));
  ASSERT_NE(child_state, nullptr);
  auto value = Lisple::Dict::get_property(child_state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 42);
}

TEST_F(SessionStateTreeTest, child_update_reads_state_from_parent_map)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :init   (fn [state ctx] {:count 0})
      :update (fn [state ctx] (assoc state :count (+ (:count state) 1)))
    })
    (pixils/defmode parent-mode {:children [{:mode child-mode}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.update_mode();

  // Then - child state in parent map is updated
  auto child_state = Lisple::Dict::get_property(session.active_mode.state,
                                                Lisple::RTValue::keyword("child-mode-0"));
  ASSERT_NE(child_state, nullptr);
  auto count = Lisple::Dict::get_property(child_state, Lisple::RTValue::keyword("count"));
  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 1);
}

TEST_F(SessionStateTreeTest, pop_mode_restores_parent_with_child_states)
{
  // Given - parent has a child with state; push a popup on top, then pop it
  runtime.eval(R"(
    (pixils/defmode child-mode {:init (fn [state ctx] {:value 99})})
    (pixils/defmode parent-mode {:children [{:mode child-mode}]})
    (pixils/defmode popup-mode {})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.push_mode("popup-mode", Lisple::Constant::NIL);
  session.pop_mode();

  // Then - active mode is parent-mode with child state intact
  auto child_state = Lisple::Dict::get_property(session.active_mode.state,
                                                Lisple::RTValue::keyword("child-mode-0"));
  ASSERT_NE(child_state, nullptr);
  auto value = Lisple::Dict::get_property(child_state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 99);
}

TEST_F(SessionStateTreeTest, sibling_children_of_same_mode_get_distinct_state_slots)
{
  // Given - two children using the same mode type
  runtime.eval(R"(
    (pixils/defmode panel-mode {:init (fn [state ctx] {:panel true})})
    (pixils/defmode split-mode {:children [{:mode panel-mode} {:mode panel-mode}]})
  )");

  // When
  session.push_mode("split-mode", Lisple::Constant::NIL);

  // Then - both children have separate state entries
  auto state0 = Lisple::Dict::get_property(session.active_mode.state,
                                           Lisple::RTValue::keyword("panel-mode-0"));
  auto state1 = Lisple::Dict::get_property(session.active_mode.state,
                                           Lisple::RTValue::keyword("panel-mode-1"));
  EXPECT_NE(state0, nullptr);
  EXPECT_NE(state1, nullptr);
}

TEST_F(SessionStateTreeTest, explicit_child_id_is_used_as_state_key)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode sidebar-mode {:init (fn [state ctx] {:loaded true})})
    (pixils/defmode root-mode {:children [{:mode sidebar-mode :id "sidebar"}]})
  )");

  // When
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // Then - state key uses the explicit id
  auto child_state = Lisple::Dict::get_property(session.active_mode.state,
                                                Lisple::RTValue::keyword("sidebar"));
  ASSERT_NE(child_state, nullptr);
  auto loaded = Lisple::Dict::get_property(child_state, Lisple::RTValue::keyword("loaded"));
  EXPECT_NE(loaded, nullptr);
}
