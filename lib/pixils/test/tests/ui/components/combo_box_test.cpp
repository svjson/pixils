#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using ComboBoxTest = RenderFixture;

TEST_F(ComboBoxTest, combo_box_trigger_uses_styleable_scrollbar_button)
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

TEST_F(ComboBoxTest, combo_box_opens_scrollable_popup_and_reports_selection)
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

TEST_F(ComboBoxTest, combo_box_popup_omits_scrollbar_when_options_fit)
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
