#include "../../render_fixture.h"

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using ListBoxTest = RenderFixture;

TEST_F(ListBoxTest, shrink_height_list_box_rebuilds_with_scrollbar_when_clamped)
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

TEST_F(ListBoxTest, list_box_uses_scroll_pane_and_forces_initial_selection)
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

  auto selected =
    Lisple::Dict::get_property(list_box->state, Lisple::keyword("selected-indices"));
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
  auto first_value = Lisple::Dict::get_property(first_item->state, Lisple::keyword("value"));
  ASSERT_NE(first_value, nullptr);
  EXPECT_EQ(first_value->to_string(), ":a");
}

TEST_F(ListBoxTest, forced_selection_skips_disabled_items)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha" :disabled? true}
                              {:value :b :label "Beta"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100
                    :force-selection? true})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  auto selected =
    Lisple::Dict::get_property(list_box->state, Lisple::keyword("selected-indices"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[1]");

  auto scroll_pane = list_box->children[0];
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  auto content = viewport->children[0];
  auto first_item = content->children[0];
  auto disabled =
    Lisple::Dict::get_property(first_item->state, Lisple::keyword("disabled?"));
  ASSERT_NE(disabled, nullptr);
  EXPECT_EQ(disabled->to_string(), "true");
}

TEST_F(ListBoxTest, classic_blue_disabled_list_box_styles_visible_viewport)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/classic-blue
       :children [(pixils.ui.list-box/make
                   {:options []
                    :disabled? true
                    :style {:width 100
                            :height 30}
                    :row-height 10
                    :visible-rows 3
                    :content-width 100})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  ASSERT_TRUE(list_box->effective_style.background.has_value());
  ASSERT_TRUE(list_box->effective_style.background->color.has_value());
  EXPECT_EQ(*list_box->effective_style.background->color,
            (Pixils::Color{0x25, 0x2d, 0x38, 255}));

  auto scroll_pane = list_box->children[0];
  ASSERT_NE(scroll_pane, nullptr);
  ASSERT_TRUE(scroll_pane->effective_style.background.has_value());
  ASSERT_TRUE(scroll_pane->effective_style.background->color.has_value());
  EXPECT_EQ(*scroll_pane->effective_style.background->color,
            (Pixils::Color{0x25, 0x2d, 0x38, 255}));

  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  ASSERT_NE(viewport, nullptr);
  ASSERT_TRUE(viewport->effective_style.background.has_value());
  ASSERT_TRUE(viewport->effective_style.background->color.has_value());
  EXPECT_EQ(*viewport->effective_style.background->color,
            (Pixils::Color{0x25, 0x2d, 0x38, 255}));
}

TEST_F(ListBoxTest, list_box_defaults_to_shrink_width)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:style {:width 300
               :height 40
               :layout {:direction :row}}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :row-height 10
                    :visible-rows 2
                    :content-width 80})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  EXPECT_EQ(list_box->bounds.w, 82);
  EXPECT_EQ(list_box->bounds.h, 20);

  auto scroll_pane = list_box->children[0];
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  EXPECT_EQ(viewport->bounds.w, 80);
}

TEST_F(ListBoxTest, list_box_with_explicit_width_stretches_items)
{
  runtime.eval(R"(
    (pixils/defcomponent custom-fixed-row
      {:extend 'ui/list-box-item
       :style {:width 30
               :height 10}
       :children [{:mode 'ui/text
                   :state {:value "Fixed"}}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 200}
                    :row-height 10
                    :visible-rows 2
                    :content-width 80
                    :item-child (fn [index option]
                                  {:mode 'custom-fixed-row
                                   :state {:index index
                                           :value (:value option)
                                           :selected-indices (pixils.ui/bind-state
                                                              :selected-indices)}})})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  EXPECT_EQ(list_box->bounds.w, 200);

  auto scroll_pane = list_box->children[0];
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  auto content = viewport->children[0];
  ASSERT_EQ(content->children.size(), 2u);
  auto first_item = content->children[0];
  EXPECT_GT(viewport->bounds.w, 80);
  EXPECT_EQ(first_item->bounds.w, viewport->bounds.w);
}

TEST_F(ListBoxTest, list_box_accepts_custom_item_children)
{
  runtime.eval(R"(
    (pixils/defcomponent custom-list-row
      {:extend 'ui/list-box-item
       :on-mouse-down (fn [state event ctx]
                        (do
                          (pixils.ui/stop-propagation! event)
                          state))
       :on-mouse-up (fn [state event ctx]
                      (do
                        (pixils.ui/emit! (:view ctx)
                                         :list-box/item-click
                                         {:index (:index state)
                                          :value (:value state)
                                          :shift? false
                                          :ctrl? false})
                        state))
       :children [{:mode 'ui/text
                   :style {:width :fill}
                   :state {:value (pixils.ui/bind-state :custom-label)}}]})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected [0]})
       :on {:list-box/change (fn [state event ctx]
                               (assoc state
                                      :selected (:selected-indices (:payload event))))}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100
                    :selected-indices (pixils.ui/bind-state :selected)
                    :force-selection? true
                    :item-child (fn [index option]
                                  {:mode 'custom-list-row
                                   :style {:height 10}
                                   :state {:index index
                                           :value (:value option)
                                           :selected-indices (pixils.ui/bind-state
                                                              :selected-indices)
                                           :custom-label (str "Custom " (:label option))}})})]})
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
  ASSERT_EQ(content->children.size(), 2u);
  EXPECT_EQ(content->children[0]->mode->name, "custom-list-row");

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();

  auto selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[1]");
}

TEST_F(ListBoxTest, list_box_item_hover_highlight_is_opt_in)
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

TEST_F(ListBoxTest, list_box_reorder_drag_is_off_by_default)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:reorder nil})
       :on {:list-box/reorder (fn [state event ctx]
                                (assoc state :reorder (:payload event)))}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 3
                    :content-width 100})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_move({5, 12});
  update_cycle();
  input().mouse_move({5, 28});
  update_cycle();
  input().mouse_up({5, 28});
  update_cycle();

  auto reorder =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("reorder"));
  ASSERT_NE(reorder, nullptr);
  EXPECT_EQ(reorder->to_string(), "nil");
}

TEST_F(ListBoxTest, reorderable_list_box_emits_reorder_drop_event)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:reorder nil
                              :selected []})
       :on {:list-box/reorder (fn [state event ctx]
                                (assoc state :reorder (:payload event)))
            :list-box/change (fn [state event ctx]
                               (assoc state
                                      :selected
                                      (:selected-indices (:payload event))))}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 3
                    :content-width 100
                    :selected-indices (pixils.ui/bind-state :selected)
                    :reorderable? true})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_move({5, 28});
  update_cycle();
  input().mouse_up({5, 28});
  update_cycle();

  auto reorder =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("reorder"));
  auto selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  ASSERT_NE(reorder, nullptr);
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(reorder->to_string(), "nil");
  EXPECT_EQ(Lisple::Dict::get_property(reorder, Lisple::keyword("from-index"))
              ->num()
              .get_int(),
            0);
  EXPECT_EQ(Lisple::Dict::get_property(reorder, Lisple::keyword("to-index"))
              ->num()
              .get_int(),
            2);
  EXPECT_EQ(Lisple::Dict::get_property(reorder, Lisple::keyword("drop-index"))
              ->num()
              .get_int(),
            3);
  EXPECT_EQ(Lisple::Dict::get_property(reorder, Lisple::keyword("value"))->to_string(),
            ":a");
  EXPECT_EQ(selected->to_string(), "[]");
}

TEST_F(ListBoxTest, clicking_selected_single_select_item_does_not_emit_change)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:selected [0]
                              :changes 0})
       :on {:list-box/change (fn [state event ctx]
                               (assoc state
                                      :selected (:selected-indices (:payload event))
                                      :changes (inc (:changes state))))}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100
                    :selected-indices (pixils.ui/bind-state :selected)})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_up({5, 5});
  update_cycle();

  auto selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  auto changes =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("changes"));
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(changes, nullptr);
  EXPECT_EQ(selected->to_string(), "[0]");
  EXPECT_EQ(changes->num().get_int(), 0);

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();

  selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  changes =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("changes"));
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(changes, nullptr);
  EXPECT_EQ(selected->to_string(), "[1]");
  EXPECT_EQ(changes->num().get_int(), 1);
}

TEST_F(ListBoxTest, list_box_ctrl_and_shift_click_update_selection)
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

  auto selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[0 1]");

  input().key_down(SDLK_LSHIFT);
  input().mouse_down({5, 25});
  update_cycle();
  input().mouse_up({5, 25});
  update_cycle();
  input().key_up(SDLK_LSHIFT);
  update_cycle();

  selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[1 2]");

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();

  selected =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[2]");
}

TEST_F(ListBoxTest, list_box_emits_activate_on_double_click)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:activated nil})
       :on {:list-box/activate (fn [state event ctx]
                                  (assoc state :activated (:payload event)))}
       :children [(pixils.ui.list-box/make
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

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();
  input().mouse_down({5, 15}, SDL_BUTTON_LEFT, 2);
  update_cycle();
  input().mouse_up({5, 15}, SDL_BUTTON_LEFT, 2);
  update_cycle();

  auto activated =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("activated"));
  ASSERT_NE(activated, nullptr);
  EXPECT_EQ(Lisple::Dict::get_property(activated, Lisple::keyword("value"))->to_string(),
            ":b");
  EXPECT_EQ(Lisple::Dict::get_property(activated, Lisple::keyword("selected-indices"))
              ->to_string(),
            "[1]");
}
