#include "../../render_fixture.h"

#include <pixils/program.h>

#include <gtest/gtest.h>
#include <SDL2/SDL_keycode.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <string>
#include <utility>

class ComboBoxTest : public RenderFixture
{
 protected:
  std::shared_ptr<Pixils::Runtime::View> render_combo_for_theme(
    const std::string& theme)
  {
    runtime.eval(std::string(R"(
      (pixils/defprogram combo-box-test-program
        {:theme ')") + theme + R"(
         :initial-mode 'root-mode})

      (pixils/defmode root-mode
        {:children [(pixils.ui.combo-box/make
                     {:options [{:value :a :label ""}
                                {:value :b :label ""}]
                      :style {:width 100}})]})
    )");

    Pixils::load_program(runtime, session);
    session.update_mode();
    session.render_mode();

    if (!session.active_mode || session.active_mode->children.empty()) return nullptr;
    return session.active_mode->children[0];
  }
};

namespace
{
  Roo::sptr_val get_state_key(const std::shared_ptr<Pixils::Runtime::View>& view,
                                 const std::string& key)
  {
    return Roo::Dict::get_property(view->state, Roo::keyword(key));
  }

  bool layout_hidden(const std::shared_ptr<Pixils::Runtime::View>& view)
  {
    return view && view->effective_style.visibility &&
           *view->effective_style.visibility == Pixils::UI::Style::Visibility::NONE;
  }
} // namespace

TEST_F(ComboBoxTest, combo_box_trigger_uses_styleable_scrollbar_button)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
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

TEST_F(ComboBoxTest, natural_height_and_popup_rows_use_default_ttf_font_metrics)
{
  runtime.eval(R"(
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24
       :line-height 30})
    (pixils/deftheme large-text-theme
      {:defaults {:text {:font :font/large-font}}})
    (pixils/defmode root-mode
      {:theme ['pixils/windows-3 'large-text-theme]
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 120}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  EXPECT_GT(combo->bounds.h, 22);

  input().mouse_down({combo->bounds.x + 2, combo->bounds.y + 2});
  update_cycle();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto popup_list_box = popup_panel->children[0];
  ASSERT_EQ(popup_list_box->children.size(), 1u);
  auto popup_scroll_pane = popup_list_box->children[0];
  ASSERT_EQ(popup_scroll_pane->children.size(), 1u);
  auto popup_row = popup_scroll_pane->children[0];
  ASSERT_GE(popup_row->children.size(), 1u);
  auto popup_viewport = popup_row->children[0];
  ASSERT_EQ(popup_viewport->children.size(), 1u);
  auto popup_content = popup_viewport->children[0];
  ASSERT_GE(popup_content->children.size(), 1u);
  EXPECT_GT(popup_content->children[0]->bounds.h, 20);
}

TEST_F(ComboBoxTest, windows_3_combo_box_button_is_flush_to_field_edge)
{
  auto combo = render_combo_for_theme("pixils/windows-3");
  ASSERT_NE(combo, nullptr);
  ASSERT_EQ(combo->children.size(), 1u);
  auto trigger = combo->children[0];
  ASSERT_NE(trigger, nullptr);
  ASSERT_EQ(trigger->children.size(), 2u);
  auto label = trigger->children[0];
  auto button = trigger->children[1];
  ASSERT_NE(label, nullptr);
  ASSERT_NE(button, nullptr);

  EXPECT_EQ(label->bounds.x, trigger->bounds.x + 3);
  EXPECT_EQ(button->bounds.w, 15);
  EXPECT_EQ(button->bounds.x + button->bounds.w, trigger->bounds.x + trigger->bounds.w);
  EXPECT_EQ(button->bounds.y, trigger->bounds.y);
  EXPECT_EQ(button->bounds.h, trigger->bounds.h);
}

TEST_F(ComboBoxTest, windows_95_combo_box_button_is_flush_to_field_edge)
{
  auto combo = render_combo_for_theme("pixils/windows-95");
  ASSERT_NE(combo, nullptr);
  ASSERT_EQ(combo->children.size(), 1u);
  auto trigger = combo->children[0];
  ASSERT_NE(trigger, nullptr);
  ASSERT_EQ(trigger->children.size(), 2u);
  auto label = trigger->children[0];
  auto button = trigger->children[1];
  ASSERT_NE(label, nullptr);
  ASSERT_NE(button, nullptr);

  EXPECT_EQ(label->bounds.x, trigger->bounds.x + 3);
  EXPECT_EQ(button->bounds.w, 16);
  EXPECT_EQ(button->bounds.x + button->bounds.w, trigger->bounds.x + trigger->bounds.w);
  EXPECT_EQ(button->bounds.y, trigger->bounds.y);
  EXPECT_EQ(button->bounds.h, trigger->bounds.h);
}

TEST_F(ComboBoxTest, disabled_combo_box_disables_trigger_button_and_does_not_press)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options []
                    :disabled? true
                    :style {:width 100}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
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

  auto disabled = Roo::Dict::get_property(button->state, Roo::keyword("disabled?"));
  ASSERT_NE(disabled, nullptr);
  EXPECT_EQ(disabled->to_string(), "true");

  input().mouse_down({95, 5});
  update_cycle();

  auto pressed = Roo::Dict::get_property(button->state, Roo::keyword("pressed"));
  ASSERT_NE(pressed, nullptr);
  EXPECT_EQ(pressed->to_string(), "false");
  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
}

TEST_F(ComboBoxTest, focused_combo_box_arrow_keys_change_selection_without_opening)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected-index 0 :focused? false})
       :update (fn [state ctx]
                 (do
                   (when (not (:focused? state))
                     (pixils.ui/focus! (head (pixils.ui/children ctx))))
                   (assoc state :focused? true)))
       :on {:combo-box/change (fn [state event ctx]
                                (assoc (assoc state
                                              :selected-index
                                              (:selected-index (:payload event)))
                                       :value
                                       (:value (:payload event))))}
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta" :disabled? true}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :selected-index (pixils.ui/bind-state :selected-index)})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  EXPECT_TRUE(combo->interaction.focused);

  input().key_down(SDLK_DOWN);
  update_cycle();
  input().key_up(SDLK_DOWN);
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
  auto selected_index = get_state_key(session.active_mode, "selected-index");
  auto value = get_state_key(session.active_mode, "value");
  ASSERT_NE(selected_index, nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(selected_index->num().get_int(), 2);
  EXPECT_EQ(value->to_string(), ":c");

  input().key_down(SDLK_UP);
  update_cycle();
  input().key_up(SDLK_UP);
  update_cycle();

  selected_index = get_state_key(session.active_mode, "selected-index");
  value = get_state_key(session.active_mode, "value");
  ASSERT_NE(selected_index, nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(selected_index->num().get_int(), 0);
  EXPECT_EQ(value->to_string(), ":a");
}

TEST_F(ComboBoxTest, combo_box_opens_scrollable_popup_and_reports_selection)
{
  runtime.eval("(def probe-combo-value (pixils.ui.combo-box/selected-value "
               "[{:value :a :label \"Alpha\"} {:value :b :label \"Beta\"}] 1))");
  auto probe = runtime.lookup("test/probe-combo-value");
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
  auto first_selected =
    Roo::Dict::get_property(first_popup_item->state, Roo::keyword("selected"));
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

  auto selected_index = Roo::Dict::get_property(session.active_mode->state,
                                                   Roo::keyword("selected-index"));
  auto value =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("value"));
  auto payload =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("payload"));
  ASSERT_NE(selected_index, nullptr);
  ASSERT_NE(value, nullptr);
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(selected_index->num().get_int(), 1);
  EXPECT_EQ(payload->to_string(), "{:selected-index 1 :value :b}");
  EXPECT_EQ(value->to_string(), ":b");
  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  EXPECT_TRUE(combo->interaction.focused);
}

TEST_F(ComboBoxTest, combo_box_popup_scroll_range_uses_measured_tall_item_height)
{
  runtime.eval(R"(
    (pixils/deftheme tall-list-item-theme
      {:styles {'ui/list-box-item {:box-sizing :content-box
                                   :padding [6 0]}}})

    (pixils/defmode root-mode
      {:theme ['pixils/base-theme 'tall-list-item-theme]
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}
                              {:value :d :label "Delta"}
                              {:value :e :label "Epsilon"}]
                    :style {:width 100}
                    :max-height 40})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  input().mouse_down({combo->bounds.x + 2, combo->bounds.y + 2});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto popup_list_box = popup_panel->children[0];
  ASSERT_EQ(popup_list_box->children.size(), 1u);
  auto popup_scroll_pane = popup_list_box->children[0];
  ASSERT_EQ(popup_scroll_pane->children.size(), 1u);
  auto popup_row = popup_scroll_pane->children[0];
  ASSERT_EQ(popup_row->children.size(), 2u);
  auto popup_viewport = popup_row->children[0];
  ASSERT_EQ(popup_viewport->children.size(), 1u);
  auto popup_content = popup_viewport->children[0];
  ASSERT_EQ(popup_content->children.size(), 5u);
  EXPECT_EQ(popup_content->children[0]->bounds.h, 32);

  auto content_size = get_state_key(popup_scroll_pane, "content-size");
  ASSERT_NE(content_size, nullptr);
  auto measured_height = Roo::Dict::get_property(content_size, Roo::keyword("h"));
  ASSERT_NE(measured_height, nullptr);
  EXPECT_EQ(measured_height->num().get_int(), 160);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
  ASSERT_EQ(row->children.size(), 2u);
  EXPECT_TRUE(layout_hidden(row->children[1]));
  auto viewport = row->children[0];
  ASSERT_NE(viewport, nullptr);
  EXPECT_EQ(viewport->bounds.w, 98);
}

TEST_F(ComboBoxTest, combo_box_popup_panel_theme_max_height_caps_scroll_viewport)
{
  runtime.eval(R"(
    (pixils/deftheme combo-popup-height-theme
      {:styles {'ui/combo-box-popup-panel {:max-height 40}}})

    (pixils/defmode root-mode
      {:theme ['pixils/base-theme 'combo-popup-height-theme]
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}
                              {:value :d :label "Delta"}
                              {:value :e :label "Epsilon"}]
                    :style {:width 100}
                    :row-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);
  EXPECT_EQ(popup_panel->bounds.h, 40);
  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto list_box = popup_panel->children[0];
  ASSERT_EQ(list_box->children.size(), 1u);
  auto scroll_pane = list_box->children[0];
  ASSERT_EQ(scroll_pane->children.size(), 1u);
  auto row = scroll_pane->children[0];
  ASSERT_EQ(row->children.size(), 2u);

  auto content_size = get_state_key(scroll_pane, "content-size");
  ASSERT_NE(content_size, nullptr);
  auto measured_height = Roo::Dict::get_property(content_size, Roo::keyword("h"));
  ASSERT_NE(measured_height, nullptr);
  EXPECT_EQ(measured_height->num().get_int(), 100);
}

TEST_F(ComboBoxTest, combo_box_popup_expands_to_wide_option_label)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont wide-test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"S" {:x 0 :y 0 :w 4 :h 8}
                "h" {:x 4 :y 0 :w 4 :h 8}
                "o" {:x 8 :y 0 :w 4 :h 8}
                "r" {:x 12 :y 0 :w 4 :h 8}
                "t" {:x 0 :y 0 :w 4 :h 8}
                "W" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Short"}
                              {:value :b :label "WWWWWWWWWWWWWWWWWWWWWWWW"}]
                    :style {:width 70
                            :max-width 70
                            :text {:font :font/wide-test-font}}
                    :row-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  input().mouse_down({combo->bounds.x + 2, combo->bounds.y + 2});
  update_cycle();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);
  EXPECT_GT(popup_panel->bounds.w, combo->bounds.w);

  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto list_box = popup_panel->children[0];
  auto content_width = get_state_key(list_box, "content-width");
  ASSERT_NE(content_width, nullptr);
  EXPECT_GT(content_width->num().get_int(), combo->bounds.w - 2);
  EXPECT_LT(content_width->num().get_int(), 150);
  EXPECT_GT(list_box->bounds.w, combo->bounds.w - 2);
}

TEST_F(ComboBoxTest, combo_box_natural_width_uses_widest_option_label)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont width-test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"S" {:x 0 :y 0 :w 4 :h 8}
                "h" {:x 4 :y 0 :w 4 :h 8}
                "o" {:x 8 :y 0 :w 4 :h 8}
                "r" {:x 12 :y 0 :w 4 :h 8}
                "t" {:x 0 :y 0 :w 4 :h 8}
                "W" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected-index 0})
       :on {:combo-box/change (fn [state event ctx]
                                (assoc state
                                       :selected-index
                                       (:selected-index (:payload event))))}
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Short"}
                              {:value :b :label "WWWWWWWWWWWWWWWWWW"}]
                    :selected-index (pixils.ui/bind-state :selected-index)
                    :style {:width :shrink
                            :text {:font :font/width-test-font}}
                    :row-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  int initial_width = combo->bounds.w;
  EXPECT_GT(initial_width, 70);

  input().mouse_down({combo->bounds.x + 2, combo->bounds.y + 2});
  update_cycle();
  session.render_mode();
  ASSERT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  input().mouse_down({combo->bounds.x + 2, combo->bounds.y + 25});
  update_cycle();
  input().mouse_up({combo->bounds.x + 2, combo->bounds.y + 25});
  update_cycle();
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);
  EXPECT_EQ(combo->bounds.w, initial_width);

  ASSERT_EQ(combo->children.size(), 1u);
  auto trigger = combo->children[0];
  ASSERT_EQ(trigger->children.size(), 2u);
  auto label = trigger->children[0];
  ASSERT_TRUE(label->effective_style.text.has_value());
  ASSERT_TRUE(label->effective_style.text->wrap.has_value());
  EXPECT_EQ(*label->effective_style.text->wrap,
            Pixils::UI::Style::Text::Wrap::NONE);
}

TEST_F(ComboBoxTest, open_combo_box_closes_without_reopening_when_trigger_clicked)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100}
                    :row-height 10
                    :max-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");

  input().mouse_down({5, 5});
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
}

TEST_F(ComboBoxTest, combo_box_popup_flips_above_when_below_would_leave_screen)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}
                              {:value :d :label "Delta"}]
                    :style {:position :absolute
                            :left 20
                            :top 170
                            :width 100}
                    :row-height 10
                    :max-height 40})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);

  input().mouse_down({25, 175});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);

  EXPECT_LT(popup_panel->visual_bounds.y, combo->visual_bounds.y);
  EXPECT_LE(popup_panel->visual_bounds.y + popup_panel->visual_bounds.h, 200);
  EXPECT_EQ(popup_panel->bounds.x, 20);
}

TEST_F(ComboBoxTest, combo_box_popup_clamps_right_edge_to_screen)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:position :absolute
                            :left 260
                            :top 20
                            :width 100}
                    :row-height 10
                    :max-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto combo = session.active_mode->children[0];
  ASSERT_NE(combo, nullptr);

  input().mouse_down({265, 25});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);

  EXPECT_LT(popup_panel->visual_bounds.x, combo->visual_bounds.x);
  EXPECT_LE(popup_panel->visual_bounds.x + popup_panel->visual_bounds.w, 320);
  EXPECT_EQ(popup_panel->bounds.x, 220);
}

TEST_F(ComboBoxTest, combo_box_popup_clamps_left_edge_to_screen)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:position :absolute
                            :left -20
                            :top 20
                            :width 100}
                    :row-height 10
                    :max-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 25});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);

  EXPECT_EQ(popup_panel->visual_bounds.x, 0);
  EXPECT_EQ(popup_panel->bounds.x, 0);
}

TEST_F(ComboBoxTest, scaled_combo_box_popup_uses_anchor_visual_geometry)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100 :scale 2}
                    :row-height 10
                    :max-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({10, 10});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_TRUE(session.active_mode->effective_style.scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.scale, 2);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);

  EXPECT_EQ(popup_panel->bounds.x, 0);
  EXPECT_EQ(popup_panel->bounds.y, 22);
  EXPECT_EQ(popup_panel->bounds.w, 100);
  EXPECT_EQ(popup_panel->visual_bounds.x, 0);
  EXPECT_EQ(popup_panel->visual_bounds.y, 44);
  EXPECT_EQ(popup_panel->visual_bounds.w, 200);
  EXPECT_EQ(popup_panel->visual_scale, 2);
}

TEST_F(ComboBoxTest, scaled_parent_combo_box_popup_aligns_and_reports_selection)
{
  runtime.eval(R"(
    (pixils/defmode scaled-panel
      {:style {:position :absolute
               :left 20
               :top 30
               :width 140
               :height 70
               :scale 2}
       :children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:position :absolute
                            :left 10
                            :top 5
                            :width 100}
                    :row-height 10
                    :max-height 40})]})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected-index nil})
       :on {:combo-box/change (fn [state event ctx]
                                (assoc (assoc state
                                              :selected-index
                                              (:selected-index (:payload event)))
                                       :value
                                       (:value (:payload event))))}
       :children [{:mode 'scaled-panel :id "panel"}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({50, 44});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  ASSERT_TRUE(session.active_mode->effective_style.scale.has_value());
  EXPECT_EQ(*session.active_mode->effective_style.scale, 2);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);
  EXPECT_EQ(popup_panel->bounds.x, 20);
  EXPECT_EQ(popup_panel->bounds.y, 42);
  EXPECT_EQ(popup_panel->bounds.w, 100);
  EXPECT_EQ(popup_panel->visual_bounds.x, 40);
  EXPECT_EQ(popup_panel->visual_bounds.y, 84);
  EXPECT_EQ(popup_panel->visual_bounds.w, 200);

  ASSERT_EQ(popup_panel->children.size(), 1u);
  auto popup_list_box = popup_panel->children[0];
  ASSERT_EQ(popup_list_box->children.size(), 1u);
  auto popup_scroll_pane = popup_list_box->children[0];
  ASSERT_EQ(popup_scroll_pane->children.size(), 1u);
  auto popup_row = popup_scroll_pane->children[0];
  ASSERT_GE(popup_row->children.size(), 1u);
  auto popup_viewport = popup_row->children[0];
  ASSERT_EQ(popup_viewport->children.size(), 1u);
  auto popup_content = popup_viewport->children[0];
  ASSERT_GE(popup_content->children.size(), 2u);
  auto second_popup_item = popup_content->children[1];
  ASSERT_NE(second_popup_item, nullptr);
  EXPECT_EQ(second_popup_item->visual_bounds.x, 42);
  EXPECT_EQ(second_popup_item->visual_bounds.y, 106);
  EXPECT_EQ(second_popup_item->visual_bounds.w, 196);
  EXPECT_EQ(second_popup_item->visual_bounds.h, 20);

  auto item_center = std::make_pair(second_popup_item->visual_bounds.x +
                                      (second_popup_item->visual_bounds.w / 2),
                                    second_popup_item->visual_bounds.y +
                                      (second_popup_item->visual_bounds.h / 2));
  input().mouse_move(item_center);
  update_cycle();
  input().mouse_down(item_center);
  update_cycle();
  input().mouse_up(item_center);
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
  update_cycle();
  auto selected_index =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected-index"));
  auto value = Roo::Dict::get_property(session.active_mode->state, Roo::keyword("value"));
  ASSERT_NE(selected_index, nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(selected_index->num().get_int(), 1);
  EXPECT_EQ(value->to_string(), ":b");
}

TEST_F(ComboBoxTest, scaled_combo_box_popup_dismisses_on_outside_click)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.combo-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100 :scale 2}
                    :row-height 10
                    :max-height 20})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({10, 10});
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");

  input().mouse_down({250, 150});
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
}
