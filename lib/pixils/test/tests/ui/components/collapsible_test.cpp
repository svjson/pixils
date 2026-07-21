#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <pixils/ui/style.h>
#include <roo/runtime/dict.h>

using CollapsibleTest = RenderFixture;

namespace
{
  Roo::sptr_val state_property(const Roo::sptr_val& state, const std::string& key)
  {
    return Roo::Dict::get_property(state, Roo::keyword(key));
  }

  bool has_render_copy_fitting(const std::vector<RenderOperation>& ops, const Pixils::Rect& rect)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::RENDER_COPY && op.rendered_rect.w > 0 &&
          op.rendered_rect.h > 0 && op.rendered_rect.w <= rect.w && op.rendered_rect.h <= rect.h)
      {
        return true;
      }

      if (has_render_copy_fitting(op.sub_ops, rect)) return true;
    }

    return false;
  }
} // namespace

TEST_F(CollapsibleTest, collapsible_toggles_bound_state_and_hides_content)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:advanced? false})
       :on {:collapsible/toggle
            (fn [state event ctx]
              (assoc state :last-toggle (:payload event)))}
       :children [(pixils.ui.collapsible/make
                   {:title "Advanced"
                    :expanded? (pixils.ui/bind-state :advanced?)
                    :value :advanced
                    :style {:width 120 :height :shrink}
                    :header-style {:height 20}
                    :marker-style {:width 12}
                    :content-style {:height 30}
                    :children [{:mode 'ui/text
                                :state {:value "Advanced content"}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto collapsible = session.active_mode->children[0];
  ASSERT_NE(collapsible, nullptr);
  ASSERT_EQ(collapsible->children.size(), 2u);
  auto header = collapsible->children[0];
  auto content = collapsible->children[1];
  ASSERT_NE(header, nullptr);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(header->children.size(), 2u);

  ASSERT_TRUE(content->effective_style.visibility.has_value());
  EXPECT_EQ(*content->effective_style.visibility, Pixils::UI::Style::Visibility::NONE);
  EXPECT_EQ(content->bounds.h, 0);
  auto marker = header->children[0];
  EXPECT_EQ(marker->mode->name, "ui/collapsible-marker");
  EXPECT_EQ(marker->children.size(), 0u);
  EXPECT_GT(marker->bounds.w, 0);
  EXPECT_GT(marker->bounds.h, 0);
  EXPECT_TRUE(has_render_copy_fitting(render_target()->render_ops, marker->bounds));

  input().mouse_down({header->bounds.x + 5, header->bounds.y + 5});
  update_cycle();
  input().mouse_up({header->bounds.x + 5, header->bounds.y + 5});
  update_cycle();

  auto expanded = state_property(session.active_mode->state, "advanced?");
  auto last_toggle = state_property(session.active_mode->state, "last-toggle");
  ASSERT_NE(expanded, nullptr);
  ASSERT_NE(last_toggle, nullptr);
  EXPECT_EQ(expanded->to_string(), "true");
  EXPECT_EQ(last_toggle->to_string(), "{:expanded? true :value :advanced}");

  session.render_mode();
  ASSERT_TRUE(content->effective_style.visibility.has_value());
  EXPECT_EQ(*content->effective_style.visibility, Pixils::UI::Style::Visibility::VISIBLE);
  EXPECT_EQ(content->bounds.h, 30);
}

TEST_F(CollapsibleTest, disabled_collapsible_header_does_not_toggle)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:advanced? false})
       :children [(pixils.ui.collapsible/make
                   {:title "Advanced"
                    :expanded? (pixils.ui/bind-state :advanced?)
                    :disabled? true
                    :header-style {:height 20}
                    :content-style {:height 30}
                    :children [{:mode 'ui/text
                                :state {:value "Advanced content"}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto collapsible = session.active_mode->children[0];
  ASSERT_NE(collapsible, nullptr);
  auto header = collapsible->children[0];
  ASSERT_NE(header, nullptr);

  input().mouse_down({header->bounds.x + 5, header->bounds.y + 5});
  update_cycle();
  input().mouse_up({header->bounds.x + 5, header->bounds.y + 5});
  update_cycle();

  auto expanded = state_property(session.active_mode->state, "advanced?");
  ASSERT_NE(expanded, nullptr);
  EXPECT_EQ(expanded->to_string(), "false");
}

TEST_F(CollapsibleTest, collapsible_can_omit_marker)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.collapsible/make
                   {:title "Plain"
                    :marker? false
                    :expanded? true
                    :header-style {:height 20}
                    :children [{:mode 'ui/text
                                :state {:value "Content"}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto collapsible = session.active_mode->children[0];
  ASSERT_NE(collapsible, nullptr);
  auto header = collapsible->children[0];
  ASSERT_NE(header, nullptr);
  ASSERT_EQ(header->children.size(), 1u);
  EXPECT_EQ(header->children[0]->mode->name, "ui/text");
}

TEST_F(CollapsibleTest, collapsible_accepts_text_marker_overrides)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:expanded? false})
       :children [(pixils.ui.collapsible/make
                   {:title "Advanced"
                    :expanded? (pixils.ui/bind-state :expanded?)
                    :collapsed-marker "+"
                    :expanded-marker "-"
                    :header-style {:height 20}
                    :children [{:mode 'ui/text
                                :state {:value "Content"}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto collapsible = session.active_mode->children[0];
  ASSERT_NE(collapsible, nullptr);
  auto header = collapsible->children[0];
  ASSERT_NE(header, nullptr);
  ASSERT_EQ(header->children.size(), 2u);
  auto marker = header->children[0];
  EXPECT_EQ(marker->mode->name, "ui/collapsible-marker");
  EXPECT_EQ(marker->children.size(), 0u);
  auto collapsed_marker = state_property(marker->state, "collapsed-marker");
  auto expanded_marker = state_property(marker->state, "expanded-marker");
  ASSERT_NE(collapsed_marker, nullptr);
  ASSERT_NE(expanded_marker, nullptr);
  EXPECT_EQ(collapsed_marker->to_string(), "\"+\"");
  EXPECT_EQ(expanded_marker->to_string(), "\"-\"");

  input().mouse_down({header->bounds.x + 5, header->bounds.y + 5});
  update_cycle();
  input().mouse_up({header->bounds.x + 5, header->bounds.y + 5});
  update_cycle();
  session.render_mode();

  auto expanded = state_property(session.active_mode->state, "expanded?");
  ASSERT_NE(expanded, nullptr);
  EXPECT_EQ(expanded->to_string(), "true");
}
