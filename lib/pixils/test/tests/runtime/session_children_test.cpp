
#include "../render_fixture.h"
#include "session_fixture.h"
#include <pixils/program.h>

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

TEST_F(SessionChildrenTest, root_mode_without_explicit_theme_uses_builtin_base_theme)
{
  runtime.eval("(pixils/defmode window-control-button {})");

  session.push_mode("window-control-button", Lisple::Constant::NIL);
  session.render_mode();

  EXPECT_EQ(session.active_mode->effective_theme.name, "pixils/base-theme");
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
  EXPECT_EQ(child->bounds.w, 50);
  EXPECT_EQ(child->bounds.h, 20);
}

TEST_F(SessionChildrenTest, child_content_size_hook_with_fill_width_uses_available_width)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :style {:width :fill}
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

TEST_F(SessionChildrenTest, mode_theme_and_child_theme_override_apply_to_effective_style)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme base-theme
      {:styles {'child-mode {:text {:scale 2}}}})

    (pixils/deftheme local-theme
      {:styles {'child-mode {:text {:font :font/console}}}})

    (pixils/defmode child-mode {:render (fn [state ctx] nil)})
    (pixils/defmode parent-mode
      {:theme 'base-theme
       :children [{:mode 'child-mode
                   :theme 'local-theme}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_TRUE(child->effective_style.text.has_value());
  ASSERT_TRUE(child->effective_style.text->font.has_value());
  ASSERT_TRUE(child->effective_style.text->scale.has_value());
  EXPECT_EQ(*child->effective_style.text->font, "font/console");
  EXPECT_EQ(*child->effective_style.text->scale, 2);
}

TEST_F(SessionChildrenTest, mode_and_child_theme_vectors_apply_in_order)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme visual-theme
      {:styles {'child-mode {:text {:font :font/visual}}}})

    (pixils/deftheme compact-layout-theme
      {:styles {'child-mode {:text {:scale 2}}}})

    (pixils/deftheme roomy-layout-theme
      {:styles {'child-mode {:text {:scale 5}}}})

    (pixils/defmode child-mode {:render (fn [state ctx] nil)})
    (pixils/defmode parent-mode
      {:theme ['visual-theme]
       :children [{:mode 'child-mode
                   :theme ['compact-layout-theme 'roomy-layout-theme]}]})
  )");
  session.push_mode("parent-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_TRUE(child->effective_style.text.has_value());
  ASSERT_TRUE(child->effective_style.text->font.has_value());
  ASSERT_TRUE(child->effective_style.text->scale.has_value());
  EXPECT_EQ(*child->effective_style.text->font, "font/visual");
  EXPECT_EQ(*child->effective_style.text->scale, 5);
}

TEST_F(SessionChildrenTest, program_theme_vector_applies_to_root_view)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme visual-theme
      {:styles {:ui/panel {:background {:r 1 :g 2 :b 3 :a 255}}}})

    (pixils/deftheme compact-layout-theme
      {:styles {:ui/panel {:padding [2 4]}}})

    (pixils/defmode root-mode
      {:class :ui/panel
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme ['visual-theme 'compact-layout-theme]})
  )");
  Pixils::load_program(runtime, session);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.background.has_value());
  ASSERT_TRUE(style.background->color.has_value());
  EXPECT_EQ(*style.background->color, (Pixils::Color{1, 2, 3, 255}));

  ASSERT_TRUE(style.padding.has_value());
  EXPECT_EQ(style.padding->t, 2);
  EXPECT_EQ(style.padding->r, 4);
  EXPECT_EQ(style.padding->b, 2);
  EXPECT_EQ(style.padding->l, 4);
}

TEST_F(SessionChildrenTest, root_mode_theme_applies_component_selector_to_active_view)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:styles {'root-mode {:text {:font :font/console
                                   :scale 3}}}})

    (pixils/defmode root-mode
      {:theme 'root-theme
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/console");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 3);
}

TEST_F(SessionChildrenTest, root_mode_theme_applies_compound_state_selector_to_active_view)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:styles {'button {:text {:font :font/console}}
                '(button {:pressed true}) {:text {:scale 3}}}})

    (pixils/defmode button
      {:theme 'root-theme
       :init (fn [state ctx] {:pressed true})
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("button", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/console");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 3);
}

TEST_F(SessionChildrenTest,
       root_mode_theme_component_selector_applies_to_mode_that_extends_component)
{
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:styles {'button {:text {:font :font/console
                                :scale 2}}}})

    (pixils/defcomponent button
      {:render (fn [state ctx] nil)})

    (pixils/defcomponent board-button
      {:extend 'button
       :theme 'root-theme
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("board-button", Lisple::Constant::NIL);

  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/console");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 2);
}

TEST_F(SessionChildrenTest, root_mode_theme_applies_class_selector_to_active_view)
{
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:styles {:ui/panel {:text {:font :font/console
                                  :scale 6}}}})

    (pixils/defmode root-mode
      {:theme 'root-theme
       :class :ui/panel
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/console");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 6);
}

TEST_F(SessionChildrenTest, child_override_class_applies_class_selector_to_child_view)
{
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:styles {:ui/panel {:text {:font :font/console
                                  :scale 7}}}})

    (pixils/defmode child-mode
      {:render (fn [state ctx] nil)})

    (pixils/defmode root-mode
      {:theme 'root-theme
       :children [{:mode 'child-mode
                   :class :ui/panel}]})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_TRUE(child->effective_style.text.has_value());
  ASSERT_TRUE(child->effective_style.text->font.has_value());
  ASSERT_TRUE(child->effective_style.text->scale.has_value());
  EXPECT_EQ(*child->effective_style.text->font, "font/console");
  EXPECT_EQ(*child->effective_style.text->scale, 7);
}

TEST_F(SessionChildrenTest,
       root_mode_theme_override_applies_component_selector_to_active_view)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:styles {'root-mode {:text {:font :font/console
                                   :scale 4}}}})

    (pixils/defmode root-mode
      {:render (fn [state ctx] nil)})
  )");
  auto overrides = Lisple::RTValue::map(
    {Lisple::RTValue::keyword("theme"), Lisple::RTValue::symbol("root-theme")});
  session.push_mode("root-mode", Lisple::Constant::NIL, overrides);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/console");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 4);
}

TEST_F(SessionChildrenTest,
       pushed_root_mode_inherits_parent_effective_theme_for_component_selectors)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme app-theme
      {:styles {'popup-mode {:text {:font :font/console
                                    :scale 5}}}})

    (pixils/defmode popup-mode
      {:render (fn [state ctx] nil)})

    (pixils/defmode root-mode
      {:theme 'app-theme
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  // When
  session.push_mode("popup-mode", Lisple::Constant::NIL);
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "popup-mode");
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/console");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 5);
}

TEST_F(SessionChildrenTest, scrollbar_lays_out_button_children_from_axis)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 10}
                   :state {:axis :x :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto start_button = scrollbar->children[0];
  auto spacer = scrollbar->children[1];
  auto end_button = scrollbar->children[2];
  ASSERT_NE(start_button, nullptr);
  ASSERT_NE(spacer, nullptr);
  ASSERT_NE(end_button, nullptr);

  EXPECT_EQ(start_button->bounds.x, 0);
  EXPECT_EQ(start_button->bounds.y, 0);
  EXPECT_EQ(start_button->bounds.w, 10);
  EXPECT_EQ(start_button->bounds.h, 10);

  EXPECT_EQ(spacer->bounds.x, 10);
  EXPECT_EQ(spacer->bounds.y, 0);
  EXPECT_EQ(spacer->bounds.w, 30);
  EXPECT_EQ(spacer->bounds.h, 10);

  EXPECT_EQ(end_button->bounds.x, 40);
  EXPECT_EQ(end_button->bounds.y, 0);
  EXPECT_EQ(end_button->bounds.w, 10);
  EXPECT_EQ(end_button->bounds.h, 10);
}

TEST_F(SessionChildrenTest, scrollbar_button_children_bubble_click_behavior_to_scrollbar)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 10}
                   :state {:axis :x :content-size 100 :value 0 :step 5}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);

  input().mouse_down({45, 5});
  update_cycle();

  auto value =
    Lisple::Dict::get_property(scrollbar->state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 5);
}

TEST_F(SessionChildrenTest, scroll_pane_offsets_content_inside_clipped_viewport)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defmode root-mode
      {:children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 40}
                    :content-size {:w 100 :h 80}
                    :offset {:x 10 :y 20}
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 2u);

  auto row = pane->children[0];
  auto footer = pane->children[1];
  ASSERT_NE(row, nullptr);
  ASSERT_NE(footer, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);
  ASSERT_EQ(viewport->children.size(), 1u);
  auto content = viewport->children[0];
  ASSERT_NE(content, nullptr);

  EXPECT_EQ(viewport->bounds.x, 0);
  EXPECT_EQ(viewport->bounds.y, 0);
  EXPECT_EQ(viewport->bounds.w, 36);
  EXPECT_EQ(viewport->bounds.h, 26);
  EXPECT_EQ(vertical_scrollbar->bounds.x, 36);
  EXPECT_EQ(vertical_scrollbar->bounds.w, 14);
  EXPECT_EQ(footer->bounds.y, 26);
  EXPECT_EQ(footer->bounds.h, 14);

  ASSERT_TRUE(viewport->effective_style.clip.has_value());
  EXPECT_TRUE(*viewport->effective_style.clip);
  EXPECT_EQ(content->bounds.x, -10);
  EXPECT_EQ(content->bounds.y, -20);
  EXPECT_EQ(content->bounds.w, 100);
  EXPECT_EQ(content->bounds.h, 80);
}

TEST_F(SessionChildrenTest, scroll_pane_without_horizontal_scroll_uses_content_width)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defmode root-mode
      {:children [(pixils.ui.scroll-pane/make
                   {:style {:height 40}
                    :content-size {:w 100 :h 80}
                    :scroll-x? false
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 1u);

  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);

  EXPECT_EQ(pane->bounds.w, 114);
  EXPECT_EQ(viewport->bounds.w, 100);
  EXPECT_EQ(vertical_scrollbar->bounds.x, 100);
  EXPECT_EQ(vertical_scrollbar->bounds.w, 14);
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

TEST_F(SessionStateTreeTest, pop_mode_result_returns_to_explicit_origin_view)
{
  runtime.eval(R"(
    (pixils/defmode popup-mode
      {:update (fn [state ctx]
                 (do (pixils/pop-mode! {:value 42})
                     state))})

    (pixils/defmode child-mode
      {:init (fn [state ctx] {:opened? false})
       :update (fn [state ctx]
                 (if (:opened? state)
                   state
                   (do (pixils/push-mode! 'popup-mode nil {:origin (:view ctx)})
                       (assoc state :opened? true))))
       :on {:pop/result (fn [state ev ctx]
                          (assoc state :result {:source-mode (:source-mode ev)
                                                :payload (:payload ev)}))}})

    (pixils/defmode root-mode
      {:children [{:mode 'child-mode :id "child"}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "popup-mode");

  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  ASSERT_EQ(session.active_mode->children.size(), 1u);

  auto child = session.active_mode->children[0];
  auto result = Lisple::Dict::get_property(child->state, Lisple::RTValue::keyword("result"));
  ASSERT_NE(result, nullptr);

  auto source_mode =
    Lisple::Dict::get_property(result, Lisple::RTValue::keyword("source-mode"));
  auto payload = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("payload"));
  auto value = Lisple::Dict::get_property(payload, Lisple::RTValue::keyword("value"));

  ASSERT_NE(source_mode, nullptr);
  ASSERT_NE(payload, nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(source_mode->str(), "popup-mode");
  EXPECT_EQ(value->num().get_int(), 42);
}

TEST_F(SessionStateTreeTest, pop_mode_result_uses_custom_origin_event_and_bubbles)
{
  runtime.eval(R"(
    (pixils/defmode popup-mode
      {:update (fn [state ctx]
                 (do (pixils/pop-mode! {:value :expert})
                     state))})

    (pixils/defmode child-mode
      {:init (fn [state ctx] {:opened? false})
       :update (fn [state ctx]
                 (if (:opened? state)
                   state
                   (do (pixils/push-mode! 'popup-mode nil
                                          {:origin {:view (:view ctx)
                                                    :event :menu/select}})
                       (assoc state :opened? true))))})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:result nil})
       :on {:menu/select (fn [state ev ctx]
                           (assoc state :result {:source-mode (:source-mode ev)
                                                 :payload (:payload ev)}))}
       :children [{:mode 'child-mode :id "child"}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "popup-mode");

  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "root-mode");

  auto result = Lisple::Dict::get_property(session.active_mode->state,
                                           Lisple::RTValue::keyword("result"));
  ASSERT_NE(result, nullptr);

  auto source_mode =
    Lisple::Dict::get_property(result, Lisple::RTValue::keyword("source-mode"));
  auto payload = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("payload"));
  auto value = Lisple::Dict::get_property(payload, Lisple::RTValue::keyword("value"));

  ASSERT_NE(source_mode, nullptr);
  ASSERT_NE(payload, nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(source_mode->str(), "popup-mode");
  EXPECT_EQ(value->str(), "expert");
}

TEST_F(SessionStateTreeTest, pop_mode_result_defaults_to_exposed_root_view_without_origin)
{
  runtime.eval(R"(
    (pixils/defmode popup-mode
      {:update (fn [state ctx]
                 (do (pixils/pop-mode! {:dismissed? true})
                     state))})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:opened? false :result nil})
       :update (fn [state ctx]
                 (if (:opened? state)
                   state
                   (do (pixils/push-mode! 'popup-mode)
                       (assoc state :opened? true))))
       :on {:pop/result (fn [state ev ctx]
                          (assoc state :result {:source-mode (:source-mode ev)
                                                :payload (:payload ev)}))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "popup-mode");

  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "root-mode");

  auto result = Lisple::Dict::get_property(session.active_mode->state,
                                           Lisple::RTValue::keyword("result"));
  ASSERT_NE(result, nullptr);

  auto source_mode =
    Lisple::Dict::get_property(result, Lisple::RTValue::keyword("source-mode"));
  auto payload = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("payload"));
  auto dismissed =
    Lisple::Dict::get_property(payload, Lisple::RTValue::keyword("dismissed?"));

  ASSERT_NE(source_mode, nullptr);
  ASSERT_NE(payload, nullptr);
  ASSERT_NE(dismissed, nullptr);
  EXPECT_EQ(source_mode->str(), "popup-mode");
  EXPECT_TRUE(Lisple::is_truthy(*dismissed));
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

TEST_F(SessionStateTreeTest, style_bang_mutates_only_the_target_view_instance)
{
  runtime.eval(R"(
    (pixils/defmode panel-mode
      {:style {:width 10 :height 10}
       :init (fn [state ctx] state)
       :update (fn [state ctx]
                 (if (:absolute? state)
                   (do (pixils.ui/style! ctx {:position :absolute
                                              :left 21
                                              :top 7
                                              :width 30
                                              :height 5})
                       state)
                   state))})

    (pixils/defmode root-mode
      {:style {:padding 3}
       :children [{:mode 'panel-mode :state {:absolute? true}}
                  {:mode 'panel-mode :state {:absolute? false}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto absolute_child = session.active_mode->children[0];
  auto flow_child = session.active_mode->children[1];
  ASSERT_NE(absolute_child, nullptr);
  ASSERT_NE(flow_child, nullptr);

  ASSERT_NE(absolute_child->owned_mode, nullptr);
  ASSERT_TRUE(absolute_child->mode->style.has_value());
  ASSERT_TRUE(absolute_child->mode->style->position.has_value());
  EXPECT_EQ(*absolute_child->mode->style->position, Pixils::UI::PositionMode::ABSOLUTE);
  EXPECT_EQ(absolute_child->bounds.x, 24);
  EXPECT_EQ(absolute_child->bounds.y, 10);
  EXPECT_EQ(absolute_child->bounds.w, 30);
  EXPECT_EQ(absolute_child->bounds.h, 5);

  ASSERT_TRUE(flow_child->mode->style.has_value());
  EXPECT_FALSE(flow_child->mode->style->position.has_value());
  EXPECT_EQ(flow_child->bounds.x, 3);
  EXPECT_EQ(flow_child->bounds.y, 3);
  EXPECT_EQ(flow_child->bounds.w, 10);
  EXPECT_EQ(flow_child->bounds.h, 10);
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

TEST_F(SessionStateTreeTest,
       replace_child_bang_rebuilds_direct_child_against_post_hook_state)
{
  runtime.eval(R"(
    (pixils/defmode old-child {})
    (pixils/defmode new-child {})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:game {:value 1} :swapped? false})
       :update (fn [state ctx]
                 (if (:swapped? state)
                   state
                   (do (pixils.ui/replace-child! (:view ctx)
                                                 "game"
                                                 {:mode 'new-child
                                                  :state (pixils.ui/bind-state :game)})
                       (-> state
                           (assoc :game {:value 42})
                           (assoc :swapped? true)))))
       :children [{:mode 'old-child
                   :id "game"
                   :state (pixils.ui/bind-state :game)}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children[0]->mode->name, "old-child");
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:value 1}");

  update_cycle();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  EXPECT_EQ(session.active_mode->children[0]->id, "game");
  EXPECT_EQ(session.active_mode->children[0]->mode->name, "new-child");
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:value 42}");
  EXPECT_EQ(session.active_mode->state->to_string(), "{:game {:value 42} :swapped? true}");
}
