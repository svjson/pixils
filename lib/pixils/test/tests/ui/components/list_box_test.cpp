#include "../../render_fixture.h"

#include <SDL2/SDL_keycode.h>
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
