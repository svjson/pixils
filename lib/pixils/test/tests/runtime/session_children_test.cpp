
#include "../render_fixture.h"
#include "session_fixture.h"
#include <pixils/program.h>

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using SessionChildrenTest = RenderFixture;
using SessionStateTreeTest = SessionFixture;

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

TEST_F(SessionChildrenTest, hook_can_read_active_theme_vars)
{
  runtime.eval(R"(
    (pixils/deftheme token-theme
      {:default-variant :light
       :vars {:light {:row-height 11
                      :alias-height (pixils/var :row-height)}
              :dark {:row-height 17}}
       :styles {}})

    (pixils/defmode child-mode
      {:theme 'token-theme
       :theme-variant :dark
       :content-size (fn [state ctx]
                       {:w (pixils.ui/theme-var (:view ctx) :missing-width 5)
                        :h (pixils.ui/theme-var ctx :alias-height)})
       :render (fn [state ctx] nil)})

    (pixils/defmode parent-mode {:children [{:mode 'child-mode}]})
  )");

  session.push_mode("parent-mode", Lisple::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->bounds.w, 5);
  EXPECT_EQ(child->bounds.h, 17);
}

TEST_F(SessionChildrenTest, inline_style_can_use_active_theme_vars)
{
  runtime.eval(R"(
    (pixils/deftheme token-theme
      {:default-variant :light
       :vars {:light {:panel-bg {:r 1 :g 2 :b 3}
                      :panel-text {:r 4 :g 5 :b 6}
                      :panel-width 40}
              :dark {:panel-bg {:r 10 :g 11 :b 12}
                     :panel-width 64}}
       :styles {}})

    (pixils/defmode root-mode
      {:theme 'token-theme
       :theme-variant :dark
       :style {:width (pixils/var :panel-width)
               :height 20
               :background (pixils/var :panel-bg)
               :text {:color (pixils/var :panel-text)}}
       :children [{:mode 'child-mode
                   :style {:width (pixils/var :panel-width)
                           :height 10
                           :background (pixils/var :panel-bg)}}]
       :render (fn [state ctx] nil)})

    (pixils/defmode child-mode
      {:render (fn [state ctx] nil)})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.width.has_value());
  EXPECT_EQ(style.width->fixed_value_or(0), 64);
  ASSERT_TRUE(style.background.has_value());
  ASSERT_TRUE(style.background->color.has_value());
  EXPECT_EQ(style.background->color->r, 10);
  ASSERT_TRUE(style.text.has_value());
  ASSERT_TRUE(style.text->color.has_value());
  EXPECT_EQ(style.text->color->r, 4);

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_TRUE(child->effective_style.width.has_value());
  EXPECT_EQ(child->effective_style.width->fixed_value_or(0), 64);
  ASSERT_TRUE(child->effective_style.background.has_value());
  ASSERT_TRUE(child->effective_style.background->color.has_value());
  EXPECT_EQ(child->effective_style.background->color->r, 10);
}

TEST_F(SessionChildrenTest, inline_theme_var_style_keeps_update_time_style_mutations)
{
  runtime.eval(R"(
    (pixils/deftheme token-theme
      {:default-variant :light
       :vars {:light {:panel-width 40}}
       :styles {}})

    (pixils/defmode child-mode
      {:style {:width (pixils/var :panel-width)
               :height 10}
       :update (fn [state ctx]
                 (do
                   (assoc-in! ctx [:view :style :position] :absolute)
                   (assoc-in! ctx [:view :style :left] 21)
                   (assoc-in! ctx [:view :style :top] 7)
                   state))
       :render (fn [state ctx] nil)})

    (pixils/defmode root-mode
      {:theme 'token-theme
       :style {:padding 3}
       :children [{:mode 'child-mode}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);

  ASSERT_TRUE(child->effective_style.width.has_value());
  EXPECT_EQ(child->effective_style.width->fixed_value_or(0), 40);
  ASSERT_TRUE(child->effective_style.position.has_value());
  EXPECT_EQ(*child->effective_style.position, Pixils::UI::PositionMode::ABSOLUTE);
  EXPECT_EQ(child->bounds.x, 24);
  EXPECT_EQ(child->bounds.y, 10);
  EXPECT_EQ(child->bounds.w, 40);
  EXPECT_EQ(child->bounds.h, 10);
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

TEST_F(SessionChildrenTest, program_theme_variant_selects_theme_vars)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme visual-theme
      {:default-variant :light
       :vars {:light {:panel-bg {:r 1 :g 2 :b 3}}
              :dark {:panel-bg {:r 10 :g 11 :b 12}}}
       :styles {:ui/panel {:background (pixils/var :panel-bg)}}})

    (pixils/defmode root-mode
      {:class :ui/panel
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'visual-theme
       :theme-variant :dark})
  )");
  Pixils::load_program(runtime, session);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.background.has_value());
  ASSERT_TRUE(style.background->color.has_value());
  EXPECT_EQ(*style.background->color, (Pixils::Color{10, 11, 12, 255}));
  EXPECT_EQ(session.active_mode->effective_theme.selected_variant,
            std::make_optional<std::string>("dark"));
}

TEST_F(SessionChildrenTest, program_theme_vector_uses_each_theme_default_variant_fallback)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme visual-theme
      {:default-variant :light
       :vars {:light {:panel-bg {:r 1 :g 2 :b 3}
                      :panel-border {:r 4 :g 5 :b 6}}
              :dark {:panel-bg {:r 10 :g 11 :b 12}
                     :panel-border {:r 13 :g 14 :b 15}}}
       :styles {:ui/panel {:background (pixils/var :panel-bg)
                           :border {:color (pixils/var :panel-border)}
                           :width (pixils/var :panel-width)}}})

    (pixils/deftheme compact-layout-theme
      {:default-variant :compact
       :vars {:compact {:panel-bg {:r 20 :g 21 :b 22}
                        :panel-width 42}}})

    (pixils/defmode root-mode
      {:class :ui/panel
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme ['visual-theme 'compact-layout-theme]
       :theme-variant :dark})
  )");
  Pixils::load_program(runtime, session);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.background.has_value());
  ASSERT_TRUE(style.background->color.has_value());
  EXPECT_EQ(*style.background->color, (Pixils::Color{20, 21, 22, 255}));

  ASSERT_TRUE(style.border.has_value());
  ASSERT_TRUE(style.border->color.has_value());
  EXPECT_EQ(*style.border->color, (Pixils::Color{13, 14, 15, 255}));

  ASSERT_TRUE(style.width.has_value());
  EXPECT_EQ(style.width->fixed_value_or(0), 42);
  EXPECT_EQ(session.active_mode->effective_theme.selected_variant,
            std::make_optional<std::string>("dark"));
}

TEST_F(SessionChildrenTest, set_theme_bang_switches_application_theme_at_runtime)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme red-theme
      {:styles {:ui/panel {:background {:r 1 :g 2 :b 3}}}})

    (pixils/deftheme blue-theme
      {:styles {:ui/panel {:background {:r 10 :g 20 :b 30}}}})

    (pixils/defmode root-mode
      {:class :ui/panel
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'red-theme})
  )");
  Pixils::load_program(runtime, session);
  session.render_mode();

  ASSERT_TRUE(session.active_mode->effective_style.background.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.background->color.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.background->color,
            (Pixils::Color{1, 2, 3, 255}));

  // When
  runtime.eval("(pixils/set-theme! 'blue-theme)");
  session.process_messages();
  session.render_mode();

  // Then
  ASSERT_TRUE(session.active_mode->effective_style.background.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.background->color.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.background->color,
            (Pixils::Color{10, 20, 30, 255}));
  ASSERT_TRUE(session.application_theme.has_value());
  ASSERT_EQ(session.application_theme->size(), 1u);
  EXPECT_EQ((*session.application_theme)[0], "blue-theme");
}

TEST_F(SessionChildrenTest, set_theme_bang_updates_children_inheriting_application_theme)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme red-theme
      {:styles {:ui/panel {:background {:r 1 :g 2 :b 3}}}})

    (pixils/deftheme blue-theme
      {:styles {:ui/panel {:background {:r 10 :g 20 :b 30}}}})

    (pixils/defmode child-mode
      {:class :ui/panel
       :render (fn [state ctx] nil)})

    (pixils/defmode root-mode
      {:children [{:mode 'child-mode}]})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'red-theme})
  )");
  Pixils::load_program(runtime, session);
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_TRUE(child->effective_style.background.has_value());
  ASSERT_TRUE(child->effective_style.background->color.has_value());
  EXPECT_EQ(*child->effective_style.background->color, (Pixils::Color{1, 2, 3, 255}));

  // When
  runtime.eval("(pixils/set-theme! 'blue-theme)");
  session.process_messages();
  session.render_mode();

  // Then
  ASSERT_TRUE(child->effective_style.background.has_value());
  ASSERT_TRUE(child->effective_style.background->color.has_value());
  EXPECT_EQ(*child->effective_style.background->color, (Pixils::Color{10, 20, 30, 255}));
}

TEST_F(SessionChildrenTest, set_theme_bang_switches_application_theme_variant)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme visual-theme
      {:default-variant :light
       :vars {:light {:panel-bg {:r 1 :g 2 :b 3}}
              :dark {:panel-bg {:r 10 :g 11 :b 12}}}
       :styles {:ui/panel {:background (pixils/var :panel-bg)}}})

    (pixils/defmode root-mode
      {:class :ui/panel
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'visual-theme})
  )");
  Pixils::load_program(runtime, session);
  session.render_mode();

  ASSERT_TRUE(session.active_mode->effective_style.background.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.background->color.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.background->color,
            (Pixils::Color{1, 2, 3, 255}));

  // When
  runtime.eval("(pixils/set-theme! 'visual-theme :dark)");
  session.process_messages();
  session.render_mode();

  // Then
  ASSERT_TRUE(session.active_mode->effective_style.background.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.background->color.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.background->color,
            (Pixils::Color{10, 11, 12, 255}));
  EXPECT_EQ(session.application_theme_variant, std::make_optional<std::string>("dark"));
}

TEST_F(SessionChildrenTest, theme_vars_override_inherited_base_theme_rules)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme border-theme
      {:default-variant :custom
       :vars {:custom {:border {:r 20 :g 40 :b 60}}}})

    (pixils/defmode root-mode
      {:class :ui/canvas
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'border-theme})
  )");
  Pixils::load_program(runtime, session);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.border.has_value());
  ASSERT_TRUE(style.border->color.has_value());
  EXPECT_EQ(*style.border->color, (Pixils::Color{20, 40, 60, 255}));
}

TEST_F(SessionChildrenTest, theme_vars_override_inherited_base_inset_border_edges)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme border-theme
      {:default-variant :custom
       :vars {:custom {:border-inset-top-left {:r 20 :g 40 :b 60}
                       :border-inset-bottom-right {:r 70 :g 80 :b 90}}}})

    (pixils/defmode root-mode
      {:class :ui/canvas
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'border-theme})
  )");
  Pixils::load_program(runtime, session);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.border.has_value());
  ASSERT_TRUE(style.border->top_color().has_value());
  ASSERT_TRUE(style.border->left_color().has_value());
  ASSERT_TRUE(style.border->right_color().has_value());
  ASSERT_TRUE(style.border->bottom_color().has_value());
  EXPECT_EQ(*style.border->top_color(), (Pixils::Color{20, 40, 60, 255}));
  EXPECT_EQ(*style.border->left_color(), (Pixils::Color{20, 40, 60, 255}));
  EXPECT_EQ(*style.border->right_color(), (Pixils::Color{70, 80, 90, 255}));
  EXPECT_EQ(*style.border->bottom_color(), (Pixils::Color{70, 80, 90, 255}));
}

TEST_F(SessionChildrenTest, theme_vars_override_inherited_base_inset_border_recipe)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme border-theme
      {:default-variant :custom
       :vars {:custom {:inset-border-style {:color {:r 1 :g 2 :b 3}
                                            :thickness 3
                                            :top {:color {:r 20 :g 40 :b 60}}
                                            :left {:color {:r 20 :g 40 :b 60}}
                                            :right {:color {:r 70 :g 80 :b 90}}
                                            :bottom {:color {:r 70 :g 80 :b 90}}}}}})

    (pixils/defmode root-mode
      {:class :ui/canvas
       :render (fn [state ctx] nil)})

    (pixils/defprogram app
      {:initial-mode 'root-mode
       :theme 'border-theme})
  )");
  Pixils::load_program(runtime, session);

  // When
  session.render_mode();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  const auto& style = session.active_mode->effective_style;
  ASSERT_TRUE(style.border.has_value());
  ASSERT_TRUE(style.border->color.has_value());
  EXPECT_EQ(*style.border->color, (Pixils::Color{1, 2, 3, 255}));
  EXPECT_EQ(style.border->top_thickness(), 3);
  EXPECT_EQ(style.border->right_thickness(), 3);
  EXPECT_EQ(style.border->bottom_thickness(), 3);
  EXPECT_EQ(style.border->left_thickness(), 3);
  EXPECT_EQ(*style.border->top_color(), (Pixils::Color{20, 40, 60, 255}));
  EXPECT_EQ(*style.border->left_color(), (Pixils::Color{20, 40, 60, 255}));
  EXPECT_EQ(*style.border->right_color(), (Pixils::Color{70, 80, 90, 255}));
  EXPECT_EQ(*style.border->bottom_color(), (Pixils::Color{70, 80, 90, 255}));
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

TEST_F(SessionChildrenTest, theme_defaults_apply_to_views_without_matching_styles)
{
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:defaults {:text {:font :font/default
                         :scale 3}}})

    (pixils/defmode root-mode
      {:theme 'root-theme
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_TRUE(session.active_mode->effective_style.text.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->font.has_value());
  ASSERT_TRUE(session.active_mode->effective_style.text->scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/default");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 3);
}

TEST_F(SessionChildrenTest, inherited_text_style_overrides_theme_defaults)
{
  runtime.eval(R"(
    (pixils/deftheme root-theme
      {:defaults {:text {:font :font/default
                         :scale 3}}})

    (pixils/defmode child-mode
      {:render (fn [state ctx] nil)})

    (pixils/defmode parent-mode
      {:style {:text {:font :font/panel}}
       :children [{:mode 'child-mode}]})

    (pixils/defmode root-mode
      {:theme 'root-theme
       :children [{:mode 'parent-mode}]})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto parent = session.active_mode->children[0];
  ASSERT_NE(parent, nullptr);
  ASSERT_EQ(parent->children.size(), 1u);
  auto child = parent->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_TRUE(child->effective_style.text.has_value());
  ASSERT_TRUE(child->effective_style.text->font.has_value());
  ASSERT_TRUE(child->effective_style.text->scale.has_value());
  EXPECT_EQ(*child->effective_style.text->font, "font/panel");
  EXPECT_EQ(*child->effective_style.text->scale, 3);
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
  auto overrides = Lisple::map({Lisple::keyword("theme"), Lisple::symbol("root-theme")});
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

TEST_F(SessionChildrenTest, pushed_root_mode_from_init_inherits_parent_effective_theme)
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
       :init (fn [state ctx]
               (do
                 (pixils/push-mode! 'popup-mode)
                 state))
       :render (fn [state ctx] nil)})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // When
  session.process_messages();
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

TEST_F(SessionChildrenTest, pushed_root_mode_uses_parent_theme_defaults)
{
  // Given
  runtime.eval(R"(
    (pixils/deftheme app-theme
      {:defaults {:text {:font :font/default
                         :scale 5}}})

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
  EXPECT_EQ(*session.active_mode->effective_style.text->font, "font/default");
  EXPECT_EQ(*session.active_mode->effective_style.text->scale, 5);
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
  auto value = Lisple::Dict::get_property(child->state, Lisple::keyword("value"));
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
  auto count = Lisple::Dict::get_property(child->state, Lisple::keyword("count"));
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
  auto value = Lisple::Dict::get_property(child->state, Lisple::keyword("value"));
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
  auto result = Lisple::Dict::get_property(child->state, Lisple::keyword("result"));
  ASSERT_NE(result, nullptr);

  auto source_mode = Lisple::Dict::get_property(result, Lisple::keyword("source-mode"));
  auto payload = Lisple::Dict::get_property(result, Lisple::keyword("payload"));
  auto value = Lisple::Dict::get_property(payload, Lisple::keyword("value"));

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

  auto result =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("result"));
  ASSERT_NE(result, nullptr);

  auto source_mode = Lisple::Dict::get_property(result, Lisple::keyword("source-mode"));
  auto payload = Lisple::Dict::get_property(result, Lisple::keyword("payload"));
  auto value = Lisple::Dict::get_property(payload, Lisple::keyword("value"));

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

  auto result =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("result"));
  ASSERT_NE(result, nullptr);

  auto source_mode = Lisple::Dict::get_property(result, Lisple::keyword("source-mode"));
  auto payload = Lisple::Dict::get_property(result, Lisple::keyword("payload"));
  auto dismissed = Lisple::Dict::get_property(payload, Lisple::keyword("dismissed?"));

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
