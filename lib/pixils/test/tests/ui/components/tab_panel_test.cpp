#include "../../render_fixture.h"
#include <pixils/program.h>
#include <pixils/ui/style.h>
#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <string>
#include <vector>

class TabPanelTest : public RenderFixture
{
 protected:
  struct RenderedTabs
  {
    std::shared_ptr<Pixils::Runtime::View> tab_panel;
    std::shared_ptr<Pixils::Runtime::View> tab_strip;
    std::shared_ptr<Pixils::Runtime::View> selected_tab;
    std::shared_ptr<Pixils::Runtime::View> inactive_tab;
    std::shared_ptr<Pixils::Runtime::View> body;
  };

  RenderedTabs render_tabs_for_theme(const std::string& theme, bool empty_labels = false)
  {
    const std::string map_label = empty_labels ? "" : "Map";
    const std::string tilesets_label = empty_labels ? "" : "Tilesets";
    const std::string source = std::string(R"roo(
      (pixils/defcomponent map-body {})
      (pixils/defcomponent tilesets-body {})

      (pixils/defprogram tab-test-program
        {:theme ')roo" + theme + R"roo(
         :initial-mode 'root-mode})

      (pixils/defmode root-mode
        {:children [(pixils.ui.tab-panel/make
                     {:selected-tab :map
                      :tabs [{:id :map
                              :label ")roo" + map_label + R"roo("
                              :child {:mode 'map-body}}
                             {:id :tilesets
                              :label ")roo" + tilesets_label + R"roo("
                              :child {:mode 'tilesets-body}}]})]})
    )roo");
    runtime.eval(source);

    Pixils::load_program(runtime, session);
    session.update_mode();
    Pixils::UI::layout_view_tree(session.active_mode,
                                 {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h},
                                 runtime,
                                 hook_args.render_args[1]);

    RenderedTabs tabs;
    if (!session.active_mode || session.active_mode->children.empty()) return tabs;
    tabs.tab_panel = session.active_mode->children[0];
    if (!tabs.tab_panel || tabs.tab_panel->children.size() < 2) return tabs;
    tabs.tab_strip = tabs.tab_panel->children[0];
    tabs.body = tabs.tab_panel->children[1];
    if (!tabs.tab_strip || tabs.tab_strip->children.size() < 2) return tabs;
    tabs.selected_tab = tabs.tab_strip->children[0];
    tabs.inactive_tab = tabs.tab_strip->children[1];
    return tabs;
  }
};

namespace
{
  bool rect_eq(const SDL_Rect& a, const SDL_Rect& b)
  {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
  }

  bool has_fill_rect(const std::vector<RenderOperation>& ops, const SDL_Rect& rect)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::FILL_RECT && rect_eq(op.rendered_rect, rect))
      {
        return true;
      }
    }

    return false;
  }

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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);

  auto selected =
    Roo::Dict::get_property(tab_panel->state, Roo::keyword("selected-tab"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(tab_panel->state, Roo::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":tilesets");

  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->mode->name, "tilesets-body");

  auto changed =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("changed-tab"));
  ASSERT_NE(changed, nullptr);
  EXPECT_EQ(changed->to_string(), ":tilesets");

  tab_panel = session.active_mode->children[0];
  tab_strip = tab_panel->children[0];
  auto first_tab = tab_strip->children[0];
  input().mouse_down({first_tab->bounds.x + first_tab->bounds.w / 2,
                      first_tab->bounds.y + first_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({first_tab->bounds.x + first_tab->bounds.w / 2,
                    first_tab->bounds.y + first_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  selected = Roo::Dict::get_property(tab_panel->state, Roo::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":map");
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);

  auto selected =
    Roo::Dict::get_property(tab_panel->state, Roo::keyword("selected-tab"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);

  auto value = Roo::Dict::get_property(active->state, Roo::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->to_string(), ":from-root");
}

TEST_F(TabPanelTest, tab_panel_body_state_passes_state_to_active_child)
{
  runtime.eval(R"(
    (pixils/defcomponent value-body {})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:shared-value :from-root})
       :children [(pixils.ui.tab-panel/make
                   {:body-state (pixils.ui/bind-state)
                    :tabs [{:id :value
                            :label "Value"
                            :child {:mode 'value-body
                                    :state {:value (pixils.ui/bind-state :shared-value)}}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);

  auto value = Roo::Dict::get_property(active->state, Roo::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->to_string(), ":from-root");
}

TEST_F(TabPanelTest, tab_panel_body_refreshes_when_bound_body_state_changes)
{
  runtime.eval(R"(
    (pixils/defcomponent value-body
      {:style {:width 80
               :height 24}
       :on-mouse-down (fn [state event ctx]
                        (do
                          (pixils.ui/emit! (:view ctx) :value/change)
                          state))})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:shared-value :initial})
       :on {:value/change (fn [state event ctx]
                            (assoc state :shared-value :changed))}
       :children [(pixils.ui.tab-panel/make
                   {:state {:shared-value (pixils.ui/bind-state :shared-value)}
                    :tabs [{:id :value
                            :label "Value"
                            :child {:mode 'value-body
                                    :state {:value (pixils.ui/bind-state :shared-value)}}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto tab_panel = session.active_mode->children[0];
  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);

  auto value = Roo::Dict::get_property(active->state, Roo::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->to_string(), ":initial");

  input().mouse_down({active->bounds.x + active->bounds.w / 2,
                      active->bounds.y + active->bounds.h / 2});
  update_cycle();
  input().mouse_up({active->bounds.x + active->bounds.w / 2,
                    active->bounds.y + active->bounds.h / 2});
  update_cycle();
  session.render_mode();

  tab_panel = session.active_mode->children[0];
  active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);

  value = Roo::Dict::get_property(active->state, Roo::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->to_string(), ":changed");
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(tab_panel->state, Roo::keyword("selected-tab"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":map");

  auto active = active_tab_child(tab_panel);
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->mode->name, "map-body");
}

TEST_F(TabPanelTest, classic_blue_theme_makes_selected_tab_raised_and_brighter)
{
  runtime.eval(R"(
    (pixils/defcomponent map-body {})
    (pixils/defcomponent tilesets-body {})

    (pixils/defprogram tab-test-program
      {:theme 'pixils/classic-blue
       :initial-mode 'root-mode})

    (pixils/defmode root-mode
      {:children [(pixils.ui.tab-panel/make
                   {:selected-tab :map
                    :tabs [{:id :map
                            :label "Map"
                            :child {:mode 'map-body}}
                           {:id :tilesets
                            :label "Tilesets"
                            :child {:mode 'tilesets-body}}]})]})
  )");

  Pixils::load_program(runtime, session);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_EQ(tab_strip->children.size(), 2u);

  auto selected_tab = tab_strip->children[0];
  auto inactive_tab = tab_strip->children[1];
  ASSERT_NE(selected_tab, nullptr);
  ASSERT_NE(inactive_tab, nullptr);

  ASSERT_TRUE(selected_tab->effective_style.background.has_value());
  ASSERT_TRUE(selected_tab->effective_style.background->color.has_value());
  ASSERT_TRUE(inactive_tab->effective_style.background.has_value());
  ASSERT_TRUE(inactive_tab->effective_style.background->color.has_value());
  EXPECT_EQ(*selected_tab->effective_style.background->color,
            (Pixils::Color{0x3d, 0x4a, 0x5e, 255}));
  EXPECT_EQ(*inactive_tab->effective_style.background->color,
            (Pixils::Color{0x2d, 0x39, 0x48, 255}));

  ASSERT_TRUE(selected_tab->effective_style.margin.has_value());
  ASSERT_TRUE(inactive_tab->effective_style.margin.has_value());
  EXPECT_EQ(selected_tab->effective_style.margin->t, 0);
  EXPECT_EQ(inactive_tab->effective_style.margin->t, 2);
  EXPECT_LT(selected_tab->bounds.y, inactive_tab->bounds.y);
  EXPECT_EQ(selected_tab->bounds.y + selected_tab->bounds.h,
            tab_strip->bounds.y + tab_strip->bounds.h);
}

TEST_F(TabPanelTest, windows_95_theme_uses_property_sheet_tabs)
{
  auto tabs = render_tabs_for_theme("pixils/windows-95");
  ASSERT_NE(tabs.tab_strip, nullptr);
  ASSERT_NE(tabs.selected_tab, nullptr);
  ASSERT_NE(tabs.inactive_tab, nullptr);
  ASSERT_NE(tabs.body, nullptr);

  const Pixils::Color panel{0xb8, 0xb8, 0xb8, 255};
  const Pixils::Color transparent{0, 0, 0, 0};
  const Pixils::Color highlight{0xdf, 0xdf, 0xdf, 255};
  const Pixils::Color shadow{0x7f, 0x7f, 0x7f, 255};

  ASSERT_TRUE(tabs.tab_panel->effective_style.background.has_value());
  ASSERT_TRUE(tabs.tab_panel->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.tab_strip->effective_style.background.has_value());
  ASSERT_TRUE(tabs.tab_strip->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.selected_tab->effective_style.background.has_value());
  ASSERT_TRUE(tabs.selected_tab->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.background.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.body->effective_style.background.has_value());
  ASSERT_TRUE(tabs.body->effective_style.background->color.has_value());
  EXPECT_EQ(*tabs.tab_panel->effective_style.background->color, transparent);
  EXPECT_EQ(*tabs.tab_strip->effective_style.background->color, transparent);
  EXPECT_EQ(*tabs.selected_tab->effective_style.background->color, panel);
  EXPECT_EQ(*tabs.inactive_tab->effective_style.background->color, panel);
  EXPECT_EQ(*tabs.body->effective_style.background->color, panel);

  ASSERT_TRUE(tabs.selected_tab->effective_style.margin.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.margin.has_value());
  EXPECT_EQ(tabs.selected_tab->effective_style.margin->t, 0);
  EXPECT_EQ(tabs.inactive_tab->effective_style.margin->t, 2);
  EXPECT_LT(tabs.selected_tab->bounds.y, tabs.inactive_tab->bounds.y);
  EXPECT_EQ(tabs.selected_tab->bounds.y + tabs.selected_tab->bounds.h,
            tabs.tab_strip->bounds.y + tabs.tab_strip->bounds.h);

  ASSERT_TRUE(tabs.selected_tab->effective_style.border.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.border.has_value());
  EXPECT_EQ(tabs.selected_tab->effective_style.border->top_thickness(), 2);
  EXPECT_EQ(tabs.selected_tab->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(tabs.selected_tab->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(tabs.selected_tab->effective_style.border->left_thickness(), 2);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->top_thickness(), 2);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->left_thickness(), 2);

  auto selected_top = tabs.selected_tab->effective_style.border->top_color();
  auto selected_right = tabs.selected_tab->effective_style.border->right_color();
  auto selected_bottom = tabs.selected_tab->effective_style.border->bottom_color();
  ASSERT_TRUE(selected_top.has_value());
  ASSERT_TRUE(selected_right.has_value());
  ASSERT_TRUE(selected_bottom.has_value());
  EXPECT_EQ(*selected_top, highlight);
  EXPECT_EQ(*selected_right, shadow);
  EXPECT_EQ(*selected_bottom, panel);

  ASSERT_TRUE(tabs.selected_tab->effective_style.padding.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.padding.has_value());
  EXPECT_EQ(tabs.selected_tab->effective_style.padding->t, 4);
  EXPECT_EQ(tabs.selected_tab->effective_style.padding->b, 6);
  EXPECT_EQ(tabs.inactive_tab->effective_style.padding->t, 3);
  EXPECT_EQ(tabs.inactive_tab->effective_style.padding->b, 3);

  ASSERT_TRUE(tabs.body->effective_style.border.has_value());
  EXPECT_EQ(tabs.body->effective_style.border->line_style,
            Pixils::UI::Style::LineStyle::BEVEL);
  EXPECT_EQ(tabs.body->effective_style.border->top_thickness(), 0);
  EXPECT_EQ(tabs.body->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(tabs.body->effective_style.border->bottom_thickness(), 2);
  EXPECT_EQ(tabs.body->effective_style.border->left_thickness(), 2);
  auto body_right = tabs.body->effective_style.border->right_color();
  ASSERT_TRUE(body_right.has_value());
  EXPECT_EQ(*body_right, shadow);
}

TEST_F(TabPanelTest, windows_95_theme_renders_panel_border_from_tab_strip)
{
  auto tabs = render_tabs_for_theme("pixils/windows-95", true);
  ASSERT_NE(tabs.tab_strip, nullptr);
  ASSERT_TRUE(tabs.tab_strip->effective_style.border.has_value());

  render_target()->render_ops.clear();
  session.render_mode();

  const int thickness = tabs.tab_strip->effective_style.border->bottom_thickness();

  EXPECT_TRUE(has_fill_rect(render_target()->render_ops,
                            SDL_Rect{tabs.tab_strip->bounds.x,
                                     tabs.tab_strip->bounds.y + tabs.tab_strip->bounds.h -
                                       thickness,
                                     tabs.tab_strip->bounds.w,
                                     thickness}));
}

TEST_F(TabPanelTest, windows_3_theme_uses_classic_property_sheet_tabs)
{
  auto tabs = render_tabs_for_theme("pixils/windows-3");
  ASSERT_NE(tabs.tab_strip, nullptr);
  ASSERT_NE(tabs.selected_tab, nullptr);
  ASSERT_NE(tabs.inactive_tab, nullptr);
  ASSERT_NE(tabs.body, nullptr);

  const Pixils::Color panel{0xc0, 0xc7, 0xc8, 255};
  const Pixils::Color transparent{0, 0, 0, 0};
  ASSERT_TRUE(tabs.tab_panel->effective_style.background.has_value());
  ASSERT_TRUE(tabs.tab_panel->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.tab_strip->effective_style.background.has_value());
  ASSERT_TRUE(tabs.tab_strip->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.selected_tab->effective_style.background.has_value());
  ASSERT_TRUE(tabs.selected_tab->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.background.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.background->color.has_value());
  ASSERT_TRUE(tabs.body->effective_style.background.has_value());
  ASSERT_TRUE(tabs.body->effective_style.background->color.has_value());
  EXPECT_EQ(*tabs.tab_panel->effective_style.background->color, transparent);
  EXPECT_EQ(*tabs.tab_strip->effective_style.background->color, transparent);
  EXPECT_EQ(*tabs.selected_tab->effective_style.background->color, panel);
  EXPECT_EQ(*tabs.inactive_tab->effective_style.background->color, panel);
  EXPECT_EQ(*tabs.body->effective_style.background->color, panel);

  ASSERT_TRUE(tabs.selected_tab->effective_style.margin.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.margin.has_value());
  EXPECT_EQ(tabs.selected_tab->effective_style.margin->t, 0);
  EXPECT_EQ(tabs.inactive_tab->effective_style.margin->t, 2);
  EXPECT_LT(tabs.selected_tab->bounds.y, tabs.inactive_tab->bounds.y);
  EXPECT_EQ(tabs.selected_tab->bounds.y + tabs.selected_tab->bounds.h,
            tabs.tab_strip->bounds.y + tabs.tab_strip->bounds.h);

  ASSERT_TRUE(tabs.selected_tab->effective_style.border.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.border.has_value());
  EXPECT_EQ(tabs.selected_tab->effective_style.border->top_thickness(), 2);
  EXPECT_EQ(tabs.selected_tab->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(tabs.selected_tab->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(tabs.selected_tab->effective_style.border->left_thickness(), 2);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->top_thickness(), 2);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(tabs.inactive_tab->effective_style.border->left_thickness(), 2);

  auto selected_top = tabs.selected_tab->effective_style.border->top_color();
  auto selected_right = tabs.selected_tab->effective_style.border->right_color();
  auto selected_bottom = tabs.selected_tab->effective_style.border->bottom_color();
  ASSERT_TRUE(selected_top.has_value());
  ASSERT_TRUE(selected_right.has_value());
  ASSERT_TRUE(selected_bottom.has_value());
  EXPECT_EQ(*selected_top, (Pixils::Color{255, 255, 255, 255}));
  EXPECT_EQ(*selected_right, (Pixils::Color{0x87, 0x88, 0x8f, 255}));
  EXPECT_EQ(*selected_bottom, panel);

  ASSERT_TRUE(tabs.selected_tab->effective_style.padding.has_value());
  ASSERT_TRUE(tabs.inactive_tab->effective_style.padding.has_value());
  EXPECT_EQ(tabs.selected_tab->effective_style.padding->t, 4);
  EXPECT_EQ(tabs.selected_tab->effective_style.padding->b, 6);
  EXPECT_EQ(tabs.inactive_tab->effective_style.padding->t, 3);
  EXPECT_EQ(tabs.inactive_tab->effective_style.padding->b, 3);

  ASSERT_TRUE(tabs.body->effective_style.border.has_value());
  EXPECT_EQ(tabs.body->effective_style.border->line_style,
            Pixils::UI::Style::LineStyle::BEVEL);
  EXPECT_EQ(tabs.body->effective_style.border->top_thickness(), 0);
  EXPECT_EQ(tabs.body->effective_style.border->right_thickness(), 2);
  EXPECT_EQ(tabs.body->effective_style.border->bottom_thickness(), 2);
  EXPECT_EQ(tabs.body->effective_style.border->left_thickness(), 2);
}
