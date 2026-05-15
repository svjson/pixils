#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>

using HeaderPanelTest = RenderFixture;

TEST_F(HeaderPanelTest, make_composes_header_and_body)
{
  runtime.eval(R"(
    (pixils/defcomponent content-mode
      {:style {:width :fill
               :height :fill}})

    (pixils/defmode root-mode
      {:children [(pixils.ui.header-panel/make
                   {:title "Inspector"
                    :style {:width 100 :height 60}
                    :children [{:mode 'content-mode}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);

  auto panel = session.active_mode->children[0];
  ASSERT_NE(panel, nullptr);
  EXPECT_EQ(panel->mode->name, "ui/header-panel");
  EXPECT_EQ(panel->bounds.w, 100);
  EXPECT_EQ(panel->bounds.h, 60);
  ASSERT_EQ(panel->children.size(), 2u);

  auto header = panel->children[0];
  auto body = panel->children[1];
  ASSERT_NE(header, nullptr);
  ASSERT_NE(body, nullptr);
  EXPECT_EQ(header->mode->name, "ui/header-panel-header");
  EXPECT_EQ(body->mode->name, "ui/header-panel-body");
  EXPECT_EQ(header->bounds.w, 100);
  EXPECT_GT(header->bounds.h, 0);
  EXPECT_EQ(body->bounds.y, header->bounds.h);
  EXPECT_EQ(body->bounds.h, panel->bounds.h - header->bounds.h);

  ASSERT_EQ(header->children.size(), 1u);
  auto title = header->children[0];
  ASSERT_NE(title, nullptr);
  EXPECT_EQ(title->mode->name, "ui/text");
  auto title_value = Lisple::Dict::get_property(title->state, Lisple::keyword("value"));
  ASSERT_NE(title_value, nullptr);
  EXPECT_EQ(title_value->to_string(), "\"Inspector\"");

  ASSERT_EQ(body->children.size(), 1u);
  EXPECT_EQ(body->children[0]->mode->name, "content-mode");
}

TEST_F(HeaderPanelTest, body_state_can_bind_to_panel_state)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:label "Selected: Grass"})
       :children [(pixils.ui.header-panel/make
                   {:title "Inspector"
                    :state {:label (pixils.ui/bind-state :label)}
                    :body-state {:label (pixils.ui/bind-state :label)}
                    :children [{:mode 'ui/text
                                :state {:value (pixils.ui/bind-state :label)}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  auto panel = session.active_mode->children[0];
  auto body = panel->children[1];
  auto text = body->children[0];
  auto text_value = Lisple::Dict::get_property(text->state, Lisple::keyword("value"));
  ASSERT_NE(text_value, nullptr);
  EXPECT_EQ(text_value->to_string(), "\"Selected: Grass\"");
}
