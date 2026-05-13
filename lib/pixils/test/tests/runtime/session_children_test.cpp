
#include "../render_fixture.h"
#include "session_fixture.h"
#include <pixils/program.h>

#include <SDL2/SDL_keycode.h>
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

TEST_F(SessionChildrenTest, child_without_mode_builds_anonymous_structural_view)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:id "container"
                   :class :ui/panel
                   :style {:width 90 :height 30}
                   :init (fn [state ctx] {:label "nested"})
                   :children [{:mode 'ui/text
                               :state {:value "nested"}}]}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->id, "container");
  ASSERT_NE(child->mode, nullptr);
  EXPECT_EQ(child->mode->name, "container");
  EXPECT_TRUE(child->mode->selector_modes.empty());
  EXPECT_EQ(child->state->to_string(), "{:label \"nested\"}");
  EXPECT_EQ(child->bounds.w, 90);
  EXPECT_EQ(child->bounds.h, 30);

  ASSERT_EQ(child->children.size(), 1u);
  auto grandchild = child->children[0];
  ASSERT_NE(grandchild, nullptr);
  ASSERT_NE(grandchild->mode, nullptr);
  EXPECT_EQ(grandchild->mode->name, "ui/text");
}

TEST_F(SessionChildrenTest, anonymous_children_get_stable_generated_ids)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:style {:width 10 :height 10}}
                  {:style {:width 20 :height 20}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  EXPECT_EQ(session.active_mode->children[0]->id, "anonymous-0");
  EXPECT_EQ(session.active_mode->children[1]->id, "anonymous-1");
  EXPECT_EQ(session.active_mode->children[0]->mode->name, "anonymous-0");
  EXPECT_EQ(session.active_mode->children[1]->mode->name, "anonymous-1");
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

TEST_F(SessionChildrenTest, scroll_pane_fill_width_contributes_content_width_to_auto_parent)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defcomponent auto-panel
      {:style {:height 40
               :layout {:direction :column}}
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width :fill
                            :height 40}
                    :content-size {:w 100 :h 80}
                    :scroll-x? false
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
    (pixils/defmode root-mode
      {:children [{:mode 'auto-panel}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto panel = session.active_mode->children[0];
  ASSERT_NE(panel, nullptr);
  EXPECT_EQ(panel->bounds.w, 114);

  ASSERT_EQ(panel->children.size(), 1u);
  auto pane = panel->children[0];
  ASSERT_NE(pane, nullptr);
  EXPECT_EQ(pane->bounds.w, 114);
}

TEST_F(SessionChildrenTest, scroll_pane_without_content_size_keeps_fill_width_viewport)
{
  runtime.eval(R"(
    (pixils/defcomponent message-area
      {:class :ui/panel
       :style {:width :fill
               :height :fill}
       :children [{:mode 'ui/text
                   :state {:value "one\ntwo"}}]})

    (pixils/defcomponent info-box
      {:class :ui/panel
       :style {:width 200
               :height :fill}
       :children [{:mode 'ui/text
                   :state {:value "info"}}]})

    (pixils/defcomponent status-area
      {:style {:layout {:direction :row}
               :height 150
               :width :fill}
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width :fill
                            :height :fill}
                    :scroll-x? false
                    :children [{:mode 'message-area}]})
                  {:mode 'info-box}]})

    (pixils/defmode root-mode
      {:children [{:mode 'status-area}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  auto status = session.active_mode->children[0];
  ASSERT_NE(status, nullptr);
  ASSERT_EQ(status->children.size(), 2u);

  auto pane = status->children[0];
  auto info = status->children[1];
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(info, nullptr);
  EXPECT_EQ(pane->bounds.w, 120);
  EXPECT_EQ(info->bounds.x, 120);
  EXPECT_EQ(info->bounds.w, 200);

  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_EQ(row->children.size(), 2u);
  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);
  EXPECT_GT(viewport->bounds.w, 0);
  EXPECT_EQ(vertical_scrollbar->bounds.x, viewport->bounds.w);

  ASSERT_EQ(viewport->children.size(), 1u);
  auto content = viewport->children[0];
  ASSERT_EQ(content->children.size(), 1u);
  auto message = content->children[0];
  ASSERT_NE(message, nullptr);
  EXPECT_GT(message->bounds.w, 0);
}

TEST_F(SessionChildrenTest, scroll_pane_measures_runtime_content_growth)
{
  runtime.eval(R"(
    (pixils/defcomponent growing-content
      {:content-size (fn [state ctx] {:w 100 :h (:height state)})})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:content-state {:height 20} :grown? false})
       :update (fn [state ctx]
                 (if (:grown? state)
                   state
                   (assoc state :content-state {:height 200} :grown? true)))
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 80}
                    :scroll-x? false
                    :scroll-y? :auto
                    :state {:content-state (pixils.ui/bind-state :content-state)}
                    :children [{:mode 'growing-content
                                :state {:height (pixils.ui/bind-state :height)}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  session.update_mode();
  session.render_mode();

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);

  auto content_size =
    Lisple::Dict::get_property(pane->state, Lisple::RTValue::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height =
    Lisple::Dict::get_property(content_size, Lisple::RTValue::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 200);

  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  auto scrollbar = row->children[1];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);
  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);
  EXPECT_LT(handle->bounds.h, track->bounds.h);
}

TEST_F(SessionChildrenTest, scroll_pane_keeps_explicit_content_size_when_child_grows)
{
  runtime.eval(R"(
    (pixils/defcomponent growing-content
      {:content-size (fn [state ctx] {:w 100 :h (:height state)})})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:content-state {:height 20} :grown? false})
       :update (fn [state ctx]
                 (if (:grown? state)
                   state
                   (assoc state :content-state {:height 200} :grown? true)))
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 80}
                    :content-size {:w 100 :h 80}
                    :scroll-x? false
                    :state {:content-state (pixils.ui/bind-state :content-state)}
                    :children [{:mode 'growing-content
                                :state {:height (pixils.ui/bind-state :height)}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  session.update_mode();
  session.render_mode();

  session.update_mode();
  session.render_mode();

  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  auto content_size =
    Lisple::Dict::get_property(pane->state, Lisple::RTValue::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height =
    Lisple::Dict::get_property(content_size, Lisple::RTValue::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 80);
}

TEST_F(SessionChildrenTest, shrink_height_list_box_rebuilds_with_scrollbar_when_clamped)
{
  runtime.eval(R"(
    (pixils/defmode header-mode
      {:style {:width :fill
               :height 10}})

    (pixils/defmode root-mode
      {:style {:width 100
               :height 25
               :layout {:direction :column}}
       :children [{:mode 'header-mode}
                  (pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100
                            :height :shrink}
                    :row-height 10
                    :max-height 30
                    :content-width 100
                    :force-selection? true})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto list_box = session.active_mode->children[1];
  ASSERT_NE(list_box, nullptr);
  EXPECT_EQ(list_box->bounds.h, 15);

  session.update_mode();
  session.render_mode();

  ASSERT_EQ(list_box->children.size(), 1u);
  auto pane = list_box->children[0];
  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_EQ(row->children.size(), 2u);
  EXPECT_EQ(row->children[1]->mode->name, "ui/scrollbar");
}

TEST_F(SessionChildrenTest, list_box_uses_scroll_pane_and_forces_initial_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100
                    :force-selection? true})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  ASSERT_NE(list_box->state, nullptr);
  EXPECT_EQ(list_box->bounds.w, 100);
  EXPECT_EQ(list_box->bounds.h, 20);

  auto selected = Lisple::Dict::get_property(list_box->state,
                                             Lisple::RTValue::keyword("selected-indices"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[0]");

  ASSERT_EQ(list_box->children.size(), 1u);
  auto scroll_pane = list_box->children[0];
  ASSERT_NE(scroll_pane, nullptr);
  EXPECT_EQ(scroll_pane->mode->name, "ui/scroll-pane");
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  auto content = viewport->children[0];
  ASSERT_GE(content->children.size(), 1u);
  auto first_item = content->children[0];
  auto first_value =
    Lisple::Dict::get_property(first_item->state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(first_value, nullptr);
  EXPECT_EQ(first_value->to_string(), ":a");
}

TEST_F(SessionChildrenTest, list_box_item_hover_highlight_is_opt_in)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  auto scroll_pane = list_box->children[0];
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  auto content = viewport->children[0];
  auto first_item = content->children[0];

  input().mouse_move({5, 5});
  update_cycle();
  session.render_mode();

  EXPECT_TRUE(first_item->interaction.hovered);
  EXPECT_FALSE(first_item->effective_style.background.has_value());
}

TEST_F(SessionChildrenTest, list_box_ctrl_and_shift_click_update_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected [0]})
       :on {:list-box/change (fn [state event ctx]
                               (assoc state
                                      :selected (:selected-indices (:payload event))))}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 3
                    :content-width 100
                    :selected-indices (pixils.ui/bind-state :selected)
                    :multi-select? true
                    :force-selection? true
                    :toggle-selected? true})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  input().key_down(SDLK_LCTRL);
  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();
  input().key_up(SDLK_LCTRL);
  update_cycle();

  auto selected = Lisple::Dict::get_property(session.active_mode->state,
                                             Lisple::RTValue::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[0 1]");

  input().key_down(SDLK_LSHIFT);
  input().mouse_down({5, 25});
  update_cycle();
  input().mouse_up({5, 25});
  update_cycle();
  input().key_up(SDLK_LSHIFT);
  update_cycle();

  selected = Lisple::Dict::get_property(session.active_mode->state,
                                        Lisple::RTValue::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[1 2]");

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();

  selected = Lisple::Dict::get_property(session.active_mode->state,
                                        Lisple::RTValue::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[2]");
}

TEST_F(SessionChildrenTest, combo_box_trigger_uses_styleable_scrollbar_button)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100}})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  ASSERT_EQ(combo->children.size(), 1u);
  auto trigger = combo->children[0];
  ASSERT_NE(trigger, nullptr);
  ASSERT_EQ(trigger->children.size(), 2u);
  auto button = trigger->children[1];
  ASSERT_NE(button, nullptr);
  EXPECT_EQ(button->mode->name, "ui/combo-box-button");
  ASSERT_GE(button->mode->selector_modes.size(), 2u);
  EXPECT_EQ(button->mode->selector_modes[0], "ui/combo-box-button");
  EXPECT_EQ(button->mode->selector_modes[1], "ui/scrollbar-button");
  ASSERT_TRUE(button->effective_style.border.has_value());
}

TEST_F(SessionChildrenTest, combo_box_opens_scrollable_popup_and_reports_selection)
{
  runtime.eval("(def probe-combo-value (pixils.ui.combo-box/selected-value "
               "[{:value :a :label \"Alpha\"} {:value :b :label \"Beta\"}] 1))");
  auto probe = runtime.lookup_value("test/probe-combo-value");
  ASSERT_NE(probe, nullptr);
  ASSERT_EQ(probe->to_string(), ":b");

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected-index 0})
       :on {:combo-box/change (fn [state event ctx]
                                (assoc (assoc (assoc state
                                                     :selected-index (:selected-index (:payload event)))
                                              :value (:value (:payload event)))
                                       :payload (:payload event)))}
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :selected-index (pixils.ui/bind-state :selected-index)
                    :row-height 10
                    :max-height 20})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");

  session.render_mode();
  session.update_mode();
  session.render_mode();
  EXPECT_FALSE(session.active_mode->effective_style.background.has_value());
  EXPECT_FALSE(session.active_mode->effective_style.border.has_value());
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);
  EXPECT_EQ(popup_panel->mode->name, "ui/combo-box-popup-panel");
  ASSERT_TRUE(popup_panel->effective_style.background.has_value());
  ASSERT_TRUE(popup_panel->effective_style.border.has_value());
  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto popup_list_box = popup_panel->children[0];
  ASSERT_EQ(popup_list_box->children.size(), 1u);
  auto popup_scroll_pane = popup_list_box->children[0];
  ASSERT_EQ(popup_scroll_pane->children.size(), 1u);
  auto popup_row = popup_scroll_pane->children[0];
  ASSERT_EQ(popup_row->children.size(), 2u);
  auto popup_viewport = popup_row->children[0];
  auto popup_scrollbar = popup_row->children[1];
  ASSERT_NE(popup_viewport, nullptr);
  ASSERT_NE(popup_scrollbar, nullptr);
  ASSERT_EQ(popup_viewport->children.size(), 1u);
  auto popup_content = popup_viewport->children[0];
  ASSERT_GE(popup_content->children.size(), 2u);
  auto first_popup_item = popup_content->children[0];
  auto second_popup_item = popup_content->children[1];
  auto first_selected = Lisple::Dict::get_property(first_popup_item->state,
                                                   Lisple::RTValue::keyword("selected"));
  ASSERT_NE(first_selected, nullptr);
  EXPECT_EQ(first_selected->to_string(), "false");
  EXPECT_EQ(popup_viewport->bounds.w, 84);
  EXPECT_EQ(popup_scrollbar->bounds.x, 85);
  EXPECT_EQ(popup_scrollbar->bounds.w, 14);
  EXPECT_LE(popup_scrollbar->bounds.x + popup_scrollbar->bounds.w, popup_panel->bounds.w);

  input().mouse_move({5, 37});
  update_cycle();
  session.render_mode();
  EXPECT_FALSE(first_popup_item->effective_style.background.has_value());
  ASSERT_TRUE(second_popup_item->effective_style.background.has_value());

  input().mouse_move({popup_scrollbar->bounds.x + 2, popup_scrollbar->bounds.y + 5});
  update_cycle();
  session.render_mode();
  EXPECT_FALSE(first_popup_item->interaction.hovered);
  EXPECT_FALSE(second_popup_item->interaction.hovered);

  input().mouse_down({95, 30});
  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");

  input().mouse_down({5, 37});
  update_cycle();
  input().mouse_up({5, 37});
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");

  update_cycle();

  auto selected_index =
    Lisple::Dict::get_property(session.active_mode->state,
                               Lisple::RTValue::keyword("selected-index"));
  auto value = Lisple::Dict::get_property(session.active_mode->state,
                                          Lisple::RTValue::keyword("value"));
  auto payload = Lisple::Dict::get_property(session.active_mode->state,
                                            Lisple::RTValue::keyword("payload"));
  ASSERT_NE(selected_index, nullptr);
  ASSERT_NE(value, nullptr);
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(selected_index->num().get_int(), 1);
  EXPECT_EQ(payload->to_string(), "{:selected-index 1 :value :b}");
  EXPECT_EQ(value->to_string(), ":b");
}

TEST_F(SessionChildrenTest, combo_box_popup_omits_scrollbar_when_options_fit)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :row-height 10
                    :max-height 40})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto list_box = popup_panel->children[0];
  ASSERT_EQ(list_box->children.size(), 1u);
  auto scroll_pane = list_box->children[0];
  ASSERT_EQ(scroll_pane->children.size(), 1u);
  auto row = scroll_pane->children[0];
  ASSERT_EQ(row->children.size(), 1u);
  auto viewport = row->children[0];
  ASSERT_NE(viewport, nullptr);
  EXPECT_EQ(viewport->bounds.w, 98);
}

TEST_F(SessionChildrenTest, slider_drag_updates_bound_value)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:zoom 1})
       :children [(pixils.ui.slider/make
                   {:style {:width 100 :height 10}
                    :value (pixils.ui/bind-state :zoom)
                    :min 1
                    :max 4
                    :step 1})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({95, 5});
  update_cycle();

  auto zoom =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::RTValue::keyword("zoom"));
  ASSERT_NE(zoom, nullptr);
  EXPECT_EQ(zoom->num().get_int(), 4);
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

TEST_F(SessionStateTreeTest, replace_child_bang_accepts_anonymous_child_entry)
{
  runtime.eval(R"(
    (pixils/defmode old-child {})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:game {:value 1} :swapped? false})
       :update (fn [state ctx]
                 (if (:swapped? state)
                   state
                   (do (pixils.ui/replace-child! (:view ctx)
                                                 "game"
                                                 {:state (pixils.ui/bind-state :game)
                                                  :style {:width 40 :height 20}
                                                  :init (fn [child-state child-ctx]
                                                          (assoc child-state :ready true))})
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

  update_cycle();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->id, "game");
  ASSERT_NE(child->mode, nullptr);
  EXPECT_EQ(child->mode->name, "game");
  EXPECT_TRUE(child->mode->selector_modes.empty());
  EXPECT_EQ(child->state->to_string(), "{:value 42 :ready true}");
  ASSERT_TRUE(child->mode->style.has_value());
  ASSERT_TRUE(child->mode->style->width.has_value());
  ASSERT_TRUE(child->mode->style->height.has_value());
  EXPECT_EQ(child->mode->style->width->fixed_value_or(), 40);
  EXPECT_EQ(child->mode->style->height->fixed_value_or(), 20);
}
