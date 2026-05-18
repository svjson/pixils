#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using TabPanelTest = RenderFixture;

namespace
{
  std::shared_ptr<Pixils::Runtime::View> tab_panel_body(
    const std::shared_ptr<Pixils::Runtime::View>& tab_panel)
  {
    if (!tab_panel || tab_panel->children.size() < 2) return nullptr;
    return tab_panel->children[1];
  }

  std::shared_ptr<Pixils::Runtime::View> active_tab_child(
    const std::shared_ptr<Pixils::Runtime::View>& tab_panel)
  {
    auto body = tab_panel_body(tab_panel);
    if (!body || body->children.empty()) return nullptr;
    return body->children[0];
  }
} // namespace

TEST_F(TabPanelTest, tab_panel_selects_first_enabled_tab_by_default)
{
  runtime.eval(R"(
    (pixils/defcomponent first-body {})
    (pixils/defcomponent second-body {})

    (pixils/defmode root-mode
      {:children [(pixils.ui.tab-panel/make
                   {:tabs [{:id :first
                            :label "First"
                            :disabled? true
                            :child {:mode 'first-body}}
                           {:id :second
                            :label "Second"
                            :child {:mode 'second-body}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);

  auto selected =
    Lisple::Dict::get_property(tab_panel->state, Lisple::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":second");

  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->mode->name, "second-body");
}

TEST_F(TabPanelTest, clicking_tab_selects_body_and_emits_change)
{
  runtime.eval(R"(
    (pixils/defcomponent map-body {})
    (pixils/defcomponent tilesets-body {})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected-tab :map})
       :on {:tab-panel/change (fn [state event ctx]
                                (assoc state
                                       :changed-tab
                                       (:tab (:payload event))))}
       :children [(pixils.ui.tab-panel/make
                   {:selected-tab (pixils.ui/bind-state :selected-tab)
                    :tabs [{:id :map
                            :label "Map"
                            :child {:mode 'map-body}}
                           {:id :tilesets
                            :label "Tilesets"
                            :child {:mode 'tilesets-body}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_EQ(tab_strip->children.size(), 2u);
  auto second_tab = tab_strip->children[1];
  ASSERT_NE(second_tab, nullptr);

  input().mouse_down({second_tab->bounds.x + second_tab->bounds.w / 2,
                      second_tab->bounds.y + second_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({second_tab->bounds.x + second_tab->bounds.w / 2,
                    second_tab->bounds.y + second_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  auto selected =
    Lisple::Dict::get_property(tab_panel->state, Lisple::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":tilesets");

  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->mode->name, "tilesets-body");

  auto changed =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("changed-tab"));
  ASSERT_NE(changed, nullptr);
  EXPECT_EQ(changed->to_string(), ":tilesets");
}

TEST_F(TabPanelTest, tab_panel_uses_bound_selected_tab)
{
  runtime.eval(R"(
    (pixils/defcomponent map-body {})
    (pixils/defcomponent tilesets-body {})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected-tab :tilesets})
       :children [(pixils.ui.tab-panel/make
                   {:selected-tab (pixils.ui/bind-state :selected-tab)
                    :tabs [{:id :map
                            :label "Map"
                            :child {:mode 'map-body}}
                           {:id :tilesets
                            :label "Tilesets"
                            :child {:mode 'tilesets-body}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);

  auto selected =
    Lisple::Dict::get_property(tab_panel->state, Lisple::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":tilesets");

  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->mode->name, "tilesets-body");
}

TEST_F(TabPanelTest, tab_panel_body_passes_state_to_active_child)
{
  runtime.eval(R"(
    (pixils/defcomponent value-body {})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:shared-value :from-root})
       :children [(pixils.ui.tab-panel/make
                   {:state {:shared-value (pixils.ui/bind-state :shared-value)}
                    :tabs [{:id :value
                            :label "Value"
                            :child {:mode 'value-body
                                    :state {:value (pixils.ui/bind-state :shared-value)}}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);

  auto value = Lisple::Dict::get_property(active->state, Lisple::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->to_string(), ":from-root");
}

TEST_F(TabPanelTest, disabled_tab_does_not_select)
{
  runtime.eval(R"(
    (pixils/defcomponent map-body {})
    (pixils/defcomponent disabled-body {})

    (pixils/defmode root-mode
      {:children [(pixils.ui.tab-panel/make
                   {:selected-tab :map
                    :tabs [{:id :map
                            :label "Map"
                            :child {:mode 'map-body}}
                           {:id :disabled
                            :label "Disabled"
                            :disabled? true
                            :child {:mode 'disabled-body}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  auto tab_strip = tab_panel->children[0];
  ASSERT_EQ(tab_strip->children.size(), 2u);
  auto disabled_tab = tab_strip->children[1];
  ASSERT_NE(disabled_tab, nullptr);

  input().mouse_down({disabled_tab->bounds.x + disabled_tab->bounds.w / 2,
                      disabled_tab->bounds.y + disabled_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({disabled_tab->bounds.x + disabled_tab->bounds.w / 2,
                    disabled_tab->bounds.y + disabled_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  auto selected =
    Lisple::Dict::get_property(tab_panel->state, Lisple::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":map");

  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->mode->name, "map-body");
}
