
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
                  {:x 0 :y 0 :w 10 :h 10}
                  {:fill true}))
    })
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
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
        {:x 0 :y 0 :w 10 :h 10}
        {:fill true}))
    (pixils/defmode child-mode {:render child-render!})
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
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
                    (merge {:x 0 :y 0} dim)
                    {:fill true})))
    })
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then - child renders into its own viewport (full parent area since only one fill child)
  EXPECT_FALSE(render_target()->render_ops.empty());
}

TEST_F(SessionChildrenTest, child_content_size_hook_informs_layout_bounds)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :content-size (fn [state ctx] {:w 50 :h 20})
      :render (fn [state ctx] nil)
    })
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->bounds.w, 320);
  EXPECT_EQ(child->bounds.h, 20);
}

TEST_F(SessionChildrenTest, child_content_size_hook_can_read_effective_style)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :content-size (fn [state ctx]
                      {:w 0
                       :h (-> ctx :view :effective-style :text :scale)})
      :render (fn [state ctx] nil)
    })
    (pixils/defmode parent-mode {
      :style {:text {:font :font/console :scale 7}}
      :children [{:mode 'child-mode}]
    })
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->effective_style.text, std::nullopt);
  ASSERT_NE(child->effective_style.text->font, std::nullopt);
  ASSERT_NE(child->effective_style.text->scale, std::nullopt);
  EXPECT_EQ(*child->effective_style.text->font, "font/console");
  EXPECT_EQ(*child->effective_style.text->scale, 7);
  EXPECT_EQ(child->bounds.h, 7);
}

TEST_F(SessionStateTreeTest, push_mode_merges_child_init_state_into_parent_state)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {:init (fn [state ctx] {:value 42})})
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
  )");

  // When
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // Then - child view owns the initialized state locally
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->state, nullptr);
  auto value = Lisple::Dict::get_property(child->state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 42);
}

TEST_F(SessionStateTreeTest, child_update_preserves_and_evolves_local_child_state)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :init   (fn [state ctx] {:count 0})
      :update (fn [state ctx] (assoc state :count (+ (:count state) 1)))
    })
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.update_mode();

  // Then - child local state is updated
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->state, nullptr);
  auto count = Lisple::Dict::get_property(child->state, Lisple::RTValue::keyword("count"));
  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 1);
}

TEST_F(SessionStateTreeTest, pop_mode_restores_parent_with_child_states)
{
  // Given - parent has a child with state; push a popup on top, then pop it
  runtime.eval(R"(
    (pixils/defmode child-mode {:init (fn [state ctx] {:value 99})})
    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
    (pixils/defmode popup-mode {})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.push_mode("popup-mode", Lisple::Constant::NIL);
  session.pop_mode();

  // Then - active mode is parent-mode with child local state intact
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "parent-mode");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->state, nullptr);
  auto value = Lisple::Dict::get_property(child->state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 99);
}

TEST_F(SessionStateTreeTest, sibling_children_of_same_mode_keep_distinct_local_states)
{
  // Given - two children using the same mode type with different local initial state
  runtime.eval(R"(
    (pixils/defmode panel-mode {:init (fn [state ctx] state)})
    (pixils/defmode split-mode {:children [{:mode 'panel-mode :state {:slot 0}}
                                           {:mode 'panel-mode :state {:slot 1}}]})
  )");

  // When
  session.push_mode("split-mode", Lisple::Constant::NIL);

  // Then - both children retain distinct local state
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto child0 = session.active_mode->children[0];
  auto child1 = session.active_mode->children[1];
  ASSERT_NE(child0, nullptr);
  ASSERT_NE(child1, nullptr);
  ASSERT_NE(child0->state, nullptr);
  ASSERT_NE(child1->state, nullptr);
  EXPECT_EQ(child0->state->to_string(), "{:slot 0}");
  EXPECT_EQ(child1->state->to_string(), "{:slot 1}");
}

TEST_F(SessionStateTreeTest, explicit_child_id_is_retained_on_view)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode sidebar-mode {:init (fn [state ctx] {:loaded true})})
    (pixils/defmode root-mode {:children [{:mode 'sidebar-mode :id "sidebar"}]})
  )");

  // When
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // Then - explicit id is retained on the child view itself
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->id, "sidebar");
  ASSERT_NE(child->state, nullptr);
  EXPECT_EQ(child->state->to_string(), "{:loaded true}");
}
