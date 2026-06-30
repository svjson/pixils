#include "../../render_fixture.h"

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <set>

using ListBoxTest = RenderFixture;

namespace
{
  std::shared_ptr<Pixils::Runtime::View> find_descendant_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;
    for (const auto& child : view->children)
    {
      auto match = find_descendant_mode(child, mode_name);
      if (match) return match;
    }
    return nullptr;
  }

  std::shared_ptr<Pixils::Runtime::View> list_box_content(
    const std::shared_ptr<Pixils::Runtime::View>& list_box)
  {
    if (!list_box || list_box->children.empty()) return nullptr;
    auto scroll_pane = list_box->children[0];
    if (!scroll_pane || scroll_pane->children.empty()) return nullptr;
    auto row = scroll_pane->children[0];
    if (!row || row->children.empty()) return nullptr;
    auto viewport = row->children[0];
    if (!viewport || viewport->children.empty()) return nullptr;
    return viewport->children[0];
  }

  std::shared_ptr<Pixils::Runtime::View> list_box_row(
    const std::shared_ptr<Pixils::Runtime::View>& list_box)
  {
    if (!list_box || list_box->children.empty()) return nullptr;
    auto scroll_pane = list_box->children[0];
    if (!scroll_pane || scroll_pane->children.empty()) return nullptr;
    return scroll_pane->children[0];
  }

  std::shared_ptr<Pixils::Runtime::View> list_box_viewport(
    const std::shared_ptr<Pixils::Runtime::View>& list_box)
  {
    auto row = list_box_row(list_box);
    if (!row || row->children.empty()) return nullptr;
    return row->children[0];
  }

  bool layout_hidden(const std::shared_ptr<Pixils::Runtime::View>& view)
  {
    return view && view->effective_style.visibility &&
           *view->effective_style.visibility == Pixils::UI::Style::Visibility::NONE;
  }

  Roo::sptr_val get_state_key(const std::shared_ptr<Pixils::Runtime::View>& view,
                              const std::string& key)
  {
    return Roo::Dict::get_property(view->state, Roo::keyword(key));
  }
} // namespace

TEST_F(ListBoxTest, shrink_height_list_box_rebuilds_with_scrollbar_when_clamped)
{
  runtime.eval(R"(
    (pixils/defmode header-mode
      {:style {:width :fill
               :height 10}})

    (pixils/defmode root-mode
      {:style {:width 100
               :height 25
               :max-height 25
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

  session.push_mode("root-mode", Roo::Constant::NIL);

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

TEST_F(ListBoxTest, windows_3_natural_height_list_box_includes_border_without_scrollbar)
{
  runtime.eval(R"(
    (pixils/defcomponent natural-row
      {:style {:width :fill
               :height 10}})

    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :row-height 10
                    :visible-rows 2
                    :content-width 80
                    :item-child (fn [index option]
                                  {:mode 'natural-row})})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);

  EXPECT_EQ(list_box->bounds.h, 22);

  auto row = list_box_row(list_box);
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  EXPECT_TRUE(layout_hidden(row->children[1]));

  auto viewport = list_box_viewport(list_box);
  ASSERT_NE(viewport, nullptr);
  EXPECT_EQ(viewport->bounds.h, 20);
}

TEST_F(ListBoxTest, natural_row_height_uses_default_ttf_font_metrics_without_extra_scrollbar)
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
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :visible-rows 2
                    :content-width 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  auto row = list_box_row(list_box);
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  EXPECT_TRUE(layout_hidden(row->children[1]));
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);

  EXPECT_GT(content->children[0]->bounds.h, 20);
  EXPECT_GT(list_box->bounds.h, 42);
}

TEST_F(ListBoxTest, tab_switch_natural_list_box_uses_measured_rows_on_first_frame)
{
  runtime.eval(R"(
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24
       :line-height 30})
    (pixils/deftheme large-text-theme
      {:defaults {:text {:font :font/large-font}}})

    (pixils/defcomponent empty-tab
      {:style {:width 40
               :height 20}})

    (pixils/defcomponent identity-tab
      {:style {:layout {:direction :row}
               :gap 6}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :visible-rows 2
                    :content-width 100})
                  (pixils.ui.list-box/make
                   {:options [{:value :x :label "Xenon"}
                              {:value :y :label "Yttrium"}]
                    :visible-rows 2
                    :content-width 100})]})

    (pixils/defmode root-mode
      {:theme ['pixils/windows-3 'large-text-theme]
       :children [(pixils.ui.tab-panel/make
                   {:selected-tab :blank
                    :tabs [{:id :blank
                            :label "Blank"
                            :child {:mode 'empty-tab}}
                           {:id :identity
                            :label "Identity"
                            :child {:mode 'identity-tab}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_EQ(tab_strip->children.size(), 2u);
  auto identity_tab = tab_strip->children[1];
  ASSERT_NE(identity_tab, nullptr);

  const int click_x = identity_tab->bounds.x + identity_tab->bounds.w / 2;
  const int click_y = identity_tab->bounds.y + identity_tab->bounds.h / 2;
  input().mouse_down({click_x, click_y});
  update_cycle();
  input().mouse_up({click_x, click_y});
  update_cycle();
  render_cycle();

  auto list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(list_box, nullptr);
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);

  const int first_frame_list_box_height = list_box->bounds.h;
  const int first_frame_row_height = content->children[0]->bounds.h;
  EXPECT_GT(content->children[0]->bounds.h, 20);
  EXPECT_GT(list_box->bounds.h, 42);

  frame_cycle();

  auto stable_list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(stable_list_box, nullptr);
  auto stable_content = list_box_content(stable_list_box);
  ASSERT_NE(stable_content, nullptr);
  ASSERT_EQ(stable_content->children.size(), 2u);

  EXPECT_EQ(stable_list_box->bounds.h, first_frame_list_box_height);
  EXPECT_EQ(stable_content->children[0]->bounds.h, first_frame_row_height);
}

TEST_F(ListBoxTest, list_box_item_text_does_not_wrap_when_width_is_constrained)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/deftheme test-font-theme
      {:defaults {:text {:font :font/test-font}}})

    (pixils/defmode root-mode
      {:theme ['pixils/base-theme 'test-font-theme]
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "AA AA AA"}]
                    :visible-rows 1
                    :content-width 12})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  std::set<int> text_y_positions;
  for (const auto& op : render_target()->render_ops)
  {
    if (op.type == RenderOpType::RENDER_COPY)
    {
      text_y_positions.insert(op.rendered_rect.y);
    }
  }

  EXPECT_EQ(text_y_positions.size(), 1u);
}

TEST_F(ListBoxTest, selected_default_item_is_marked_on_first_render)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :selected-indices [1]
                    :row-height 10
                    :visible-rows 2
                    :content-width 80})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);

  auto first_selected = get_state_key(content->children[0], "selected");
  auto second_selected = get_state_key(content->children[1], "selected");
  ASSERT_NE(first_selected, nullptr);
  ASSERT_NE(second_selected, nullptr);
  EXPECT_EQ(first_selected->to_string(), "false");
  EXPECT_EQ(second_selected->to_string(), "true");
}

TEST_F(ListBoxTest, selected_custom_item_is_marked_on_first_render)
{
  runtime.eval(R"(
    (pixils/defcomponent custom-item
      {:style {:width :fill
               :height 10}})

    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :selected-indices [1]
                    :row-height 10
                    :visible-rows 2
                    :content-width 80
                    :item-child (fn [index option]
                                  {:mode 'custom-item})})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);

  auto first_selected = get_state_key(content->children[0], "selected");
  auto second_selected = get_state_key(content->children[1], "selected");
  ASSERT_NE(first_selected, nullptr);
  ASSERT_NE(second_selected, nullptr);
  EXPECT_EQ(first_selected->to_string(), "false");
  EXPECT_EQ(second_selected->to_string(), "true");
}

TEST_F(ListBoxTest, selected_auto_width_item_uses_measured_width_on_first_render)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})

    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "A"}
                              {:value :b :label "AAAAAAAAAA"}]
                    :selected-indices [1]
                    :style {:text {:font :font/test-font}}
                    :row-height 10
                    :visible-rows 2})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);

  auto selected_item = content->children[1];
  ASSERT_NE(selected_item, nullptr);
  EXPECT_GT(selected_item->bounds.w, 1);
  EXPECT_EQ(selected_item->bounds.w, content->bounds.w);
}

TEST_F(ListBoxTest, tab_switch_selected_auto_width_item_is_stable_on_first_frame)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})

    (pixils/defcomponent empty-tab
      {:style {:width 40
               :height 20}})

    (pixils/defcomponent list-tab
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "A"}
                              {:value :b :label "AAAAAAAAAA"}]
                    :selected-indices [1]
                    :style {:text {:font :font/test-font}}
                    :row-height 10
                    :visible-rows 2})]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.tab-panel/make
                   {:selected-tab :blank
                    :tabs [{:id :blank
                            :label "Blank"
                            :child {:mode 'empty-tab}}
                           {:id :list
                            :label "List"
                            :child {:mode 'list-tab}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_EQ(tab_strip->children.size(), 2u);
  auto list_tab = tab_strip->children[1];
  ASSERT_NE(list_tab, nullptr);

  const int click_x = list_tab->bounds.x + list_tab->bounds.w / 2;
  const int click_y = list_tab->bounds.y + list_tab->bounds.h / 2;
  input().mouse_down({click_x, click_y});
  update_cycle();
  input().mouse_up({click_x, click_y});
  update_cycle();
  render_cycle();

  auto list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(list_box, nullptr);
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);
  auto selected_item = content->children[1];
  ASSERT_NE(selected_item, nullptr);
  ASSERT_EQ(selected_item->children.size(), 1u);
  auto selected_text = selected_item->children[0];
  ASSERT_NE(selected_text, nullptr);

  const int first_frame_list_width = list_box->bounds.w;
  const int first_frame_content_width = content->bounds.w;
  const int first_frame_item_width = selected_item->bounds.w;
  const int first_frame_text_width = selected_text->bounds.w;

  EXPECT_GT(first_frame_item_width, 1);
  EXPECT_EQ(first_frame_item_width, first_frame_content_width);
  EXPECT_GT(first_frame_text_width, 1);

  frame_cycle();

  auto stable_list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(stable_list_box, nullptr);
  auto stable_content = list_box_content(stable_list_box);
  ASSERT_NE(stable_content, nullptr);
  ASSERT_EQ(stable_content->children.size(), 2u);
  auto stable_selected_item = stable_content->children[1];
  ASSERT_NE(stable_selected_item, nullptr);
  ASSERT_EQ(stable_selected_item->children.size(), 1u);
  auto stable_selected_text = stable_selected_item->children[0];
  ASSERT_NE(stable_selected_text, nullptr);

  EXPECT_EQ(stable_list_box->bounds.w, first_frame_list_width);
  EXPECT_EQ(stable_content->bounds.w, first_frame_content_width);
  EXPECT_EQ(stable_selected_item->bounds.w, first_frame_item_width);
  EXPECT_EQ(stable_selected_text->bounds.w, first_frame_text_width);
}

TEST_F(ListBoxTest, tab_switch_bound_selected_auto_width_item_is_stable_on_first_frame)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})

    (pixils/defcomponent empty-tab
      {:style {:width 40
               :height 20}})

    (pixils/defcomponent list-tab
      {:state {:list-selected (pixils.ui/bind-state :list-selected)}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "A"}
                              {:value :b :label "AAAAAAAAAA"}]
                    :selected-indices (pixils.ui/bind-state :list-selected)
                    :style {:text {:font :font/test-font}}
                    :row-height 10
                    :visible-rows 2})]})

    (pixils/defmode root-mode
      {:state {:selected-tab :blank
               :list-selected [1]}
       :children [(pixils.ui.tab-panel/make
                   {:selected-tab (pixils.ui/bind-state :selected-tab)
                    :state {:selected-tab (pixils.ui/bind-state :selected-tab)
                            :list-selected (pixils.ui/bind-state :list-selected)}
                    :body-state {:list-selected (pixils.ui/bind-state :list-selected)}
                    :tabs [{:id :blank
                            :label "Blank"
                            :child {:mode 'empty-tab}}
                           {:id :list
                            :label "List"
                            :body {:mode 'list-tab}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_EQ(tab_strip->children.size(), 2u);
  auto list_tab = tab_strip->children[1];
  ASSERT_NE(list_tab, nullptr);

  const int click_x = list_tab->bounds.x + list_tab->bounds.w / 2;
  const int click_y = list_tab->bounds.y + list_tab->bounds.h / 2;
  input().mouse_down({click_x, click_y});
  update_cycle();
  input().mouse_up({click_x, click_y});
  update_cycle();
  render_cycle();

  auto list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(list_box, nullptr);
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);
  auto selected_item = content->children[1];
  ASSERT_NE(selected_item, nullptr);
  ASSERT_EQ(selected_item->children.size(), 1u);
  auto selected_text = selected_item->children[0];
  ASSERT_NE(selected_text, nullptr);

  const int first_frame_list_width = list_box->bounds.w;
  const int first_frame_content_width = content->bounds.w;
  const int first_frame_item_width = selected_item->bounds.w;
  const int first_frame_text_width = selected_text->bounds.w;

  EXPECT_GT(first_frame_item_width, 1);
  EXPECT_EQ(first_frame_item_width, first_frame_content_width);
  EXPECT_GT(first_frame_text_width, 1);

  frame_cycle();

  auto stable_list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(stable_list_box, nullptr);
  auto stable_content = list_box_content(stable_list_box);
  ASSERT_NE(stable_content, nullptr);
  ASSERT_EQ(stable_content->children.size(), 2u);
  auto stable_selected_item = stable_content->children[1];
  ASSERT_NE(stable_selected_item, nullptr);
  ASSERT_EQ(stable_selected_item->children.size(), 1u);
  auto stable_selected_text = stable_selected_item->children[0];
  ASSERT_NE(stable_selected_text, nullptr);

  EXPECT_EQ(stable_list_box->bounds.w, first_frame_list_width);
  EXPECT_EQ(stable_content->bounds.w, first_frame_content_width);
  EXPECT_EQ(stable_selected_item->bounds.w, first_frame_item_width);
  EXPECT_EQ(stable_selected_text->bounds.w, first_frame_text_width);
}

TEST_F(ListBoxTest, tab_switch_explicit_content_width_item_fills_on_first_frame)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})

    (pixils/defcomponent empty-tab
      {:style {:width 40
               :height 20}})

    (pixils/defcomponent list-tab
      {:style {:width :fill
               :height :shrink
               :layout {:direction :row
                        :gap 10}}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Short"}
                              {:value :b :label "A much longer list item"}
                              {:value :c :label "Medium"}]
                    :selected-indices [0]
                    :style {:text {:font :font/test-font}}
                    :visible-rows 3
                    :content-width 180})]})

    (pixils/defmode root-mode
      {:style {:width 260
               :height 120}
       :children [(pixils.ui.tab-panel/make
                   {:selected-tab :blank
                    :style {:width :fill
                            :height :shrink}
                    :tabs [{:id :blank
                            :label "Blank"
                            :child {:mode 'empty-tab}}
                           {:id :list
                            :label "List"
                            :child {:mode 'list-tab}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto tab_panel = session.active_mode->children[0];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_EQ(tab_strip->children.size(), 2u);
  auto list_tab = tab_strip->children[1];
  ASSERT_NE(list_tab, nullptr);

  const int click_x = list_tab->bounds.x + list_tab->bounds.w / 2;
  const int click_y = list_tab->bounds.y + list_tab->bounds.h / 2;
  input().mouse_down({click_x, click_y});
  update_cycle();
  input().mouse_up({click_x, click_y});
  update_cycle();
  render_cycle();

  auto list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(list_box, nullptr);
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 3u);
  auto selected_item = content->children[0];
  ASSERT_NE(selected_item, nullptr);

  const int first_frame_content_width = content->bounds.w;
  const int first_frame_item_width = selected_item->bounds.w;
  EXPECT_EQ(first_frame_content_width, 180);
  EXPECT_EQ(first_frame_item_width, first_frame_content_width);

  frame_cycle();

  auto stable_list_box = find_descendant_mode(session.active_mode, "ui/list-box");
  ASSERT_NE(stable_list_box, nullptr);
  auto stable_content = list_box_content(stable_list_box);
  ASSERT_NE(stable_content, nullptr);
  ASSERT_EQ(stable_content->children.size(), 3u);
  auto stable_selected_item = stable_content->children[0];
  ASSERT_NE(stable_selected_item, nullptr);

  EXPECT_EQ(stable_content->bounds.w, first_frame_content_width);
  EXPECT_EQ(stable_selected_item->bounds.w, first_frame_item_width);
}

TEST_F(ListBoxTest, windows_3_natural_height_list_box_with_max_height_keeps_scrollbar_when_clamped)
{
  runtime.eval(R"(
    (pixils/defcomponent natural-row
      {:style {:width :fill
               :height 10}})

    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :row-height 10
                    :max-height 20
                    :content-width 80
                    :item-child (fn [index option]
                                  {:mode 'natural-row})})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);

  EXPECT_EQ(list_box->bounds.h, 22);

  auto row = list_box_row(list_box);
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  EXPECT_EQ(row->children[1]->mode->name, "ui/scrollbar");

  auto viewport = list_box_viewport(list_box);
  ASSERT_NE(viewport, nullptr);
  EXPECT_EQ(viewport->bounds.h, 20);
}

TEST_F(ListBoxTest, list_box_uses_scroll_pane_and_forces_initial_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100
                            :max-width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100
                    :force-selection? true})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  ASSERT_NE(list_box->state, nullptr);
  EXPECT_EQ(list_box->bounds.w, 100);
  EXPECT_EQ(list_box->bounds.h, 22);

  auto selected =
    Roo::Dict::get_property(list_box->state, Roo::keyword("selected-indices"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[0]");

  ASSERT_EQ(list_box->children.size(), 1u);
  auto scroll_pane = list_box->children[0];
  ASSERT_NE(scroll_pane, nullptr);
  EXPECT_EQ(scroll_pane->mode->name, "ui/scroll-pane");
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  EXPECT_EQ(viewport->bounds.h, 20);
  auto content = viewport->children[0];
  ASSERT_GE(content->children.size(), 1u);
  auto first_item = content->children[0];
  auto first_value = Roo::Dict::get_property(first_item->state, Roo::keyword("value"));
  ASSERT_NE(first_value, nullptr);
  EXPECT_EQ(first_value->to_string(), ":a");
}

TEST_F(ListBoxTest, list_box_renders_rows_from_bound_options)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:items [{:value :a :label "Alpha"}
                        {:value :b :label "Beta"}
                        {:value :c :label "Gamma"}]})
       :children [(pixils.ui.list-box/make
                   {:options (pixils.ui/bind-state :items)
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 2
                    :content-width 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  EXPECT_EQ(list_box->bounds.h, 22);

  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 3u);
  auto first_value =
    Roo::Dict::get_property(content->children[0]->state, Roo::keyword("value"));
  ASSERT_NE(first_value, nullptr);
  EXPECT_EQ(first_value->to_string(), ":a");
}

TEST_F(ListBoxTest, list_box_rebuilds_rows_when_bound_options_change)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:expanded? false
                :items [{:value :a :label "Alpha"}
                        {:value :b :label "Beta"}]})
       :update (fn [state ctx]
                 (if (:expanded? state)
                   state
                   (assoc state
                          :expanded? true
                          :items [{:value :a :label "Alpha"}
                                  {:value :b :label "Beta"}
                                  {:value :c :label "Gamma"}
                                  {:value :d :label "Delta"}])))
       :children [(pixils.ui.list-box/make
                   {:options (pixils.ui/bind-state :items)
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 3
                    :content-width 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  EXPECT_EQ(list_box->bounds.h, 32);

  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 4u);
  auto last_value =
    Roo::Dict::get_property(content->children[3]->state, Roo::keyword("value"));
  ASSERT_NE(last_value, nullptr);
  EXPECT_EQ(last_value->to_string(), ":d");
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  auto selected =
    Roo::Dict::get_property(list_box->state, Roo::keyword("selected-indices"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[1]");

  auto scroll_pane = list_box->children[0];
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  auto content = viewport->children[0];
  auto first_item = content->children[0];
  auto disabled =
    Roo::Dict::get_property(first_item->state, Roo::keyword("disabled?"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto list_box = session.active_mode->children[0];
  ASSERT_NE(list_box, nullptr);
  EXPECT_EQ(list_box->bounds.w, 82);
  EXPECT_EQ(list_box->bounds.h, 22);

  auto scroll_pane = list_box->children[0];
  auto row = scroll_pane->children[0];
  auto viewport = row->children[0];
  EXPECT_EQ(viewport->bounds.w, 80);
  EXPECT_EQ(viewport->bounds.h, 20);
}

TEST_F(ListBoxTest, list_box_without_content_width_shrinks_to_option_labels)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/defmode root-mode
      {:style {:width 300
               :height 40
               :layout {:direction :row
                        :gap 4}}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "A"}]
                    :style {:text {:font :font/test-font}}
                    :row-height 10
                    :visible-rows 1})
                  (pixils.ui.list-box/make
                   {:options [{:value :long :label "AAAAAAAAAA"}]
                    :style {:text {:font :font/test-font}}
                    :row-height 10
                    :visible-rows 1})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto short_list = session.active_mode->children[0];
  auto long_list = session.active_mode->children[1];
  ASSERT_NE(short_list, nullptr);
  ASSERT_NE(long_list, nullptr);

  EXPECT_LT(short_list->bounds.w, 82);
  EXPECT_LT(long_list->bounds.w, 162);
  EXPECT_GT(long_list->bounds.w, short_list->bounds.w);
}

TEST_F(ListBoxTest, list_box_content_width_uses_default_ttf_font_metrics)
{
  runtime.eval(R"(
    (pixils/deffont small-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24})
    (pixils/defmode root-mode
      {:style {:width 400
               :height 40
               :layout {:direction :row
                        :gap 4}}
       :children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}]
                    :style {:text {:font :font/small-font}}
                    :row-height 10
                    :visible-rows 1})
                  (pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}]
                    :style {:text {:font :font/large-font}}
                    :row-height 10
                    :visible-rows 1})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto small_list = session.active_mode->children[0];
  auto large_list = session.active_mode->children[1];
  ASSERT_NE(small_list, nullptr);
  ASSERT_NE(large_list, nullptr);

  EXPECT_GT(large_list->bounds.w, small_list->bounds.w);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("reorder"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("reorder"));
  auto selected =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
  ASSERT_NE(reorder, nullptr);
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(reorder->to_string(), "nil");
  EXPECT_EQ(Roo::Dict::get_property(reorder, Roo::keyword("from-index"))
              ->num()
              .get_int(),
            0);
  EXPECT_EQ(Roo::Dict::get_property(reorder, Roo::keyword("to-index"))
              ->num()
              .get_int(),
            2);
  EXPECT_EQ(Roo::Dict::get_property(reorder, Roo::keyword("drop-index"))
              ->num()
              .get_int(),
            3);
  EXPECT_EQ(Roo::Dict::get_property(reorder, Roo::keyword("value"))->to_string(),
            ":a");
  EXPECT_EQ(selected->to_string(), "[]");
}

TEST_F(ListBoxTest, reorderable_list_box_placeholder_strategy_previews_drop)
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
                    :content-width 100
                    :reorderable? true})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 3u);

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_move({5, 28});
  update_cycle();
  update_cycle();
  session.render_mode();

  auto first = content->children[0];
  auto second = content->children[1];
  auto third = content->children[2];
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  ASSERT_NE(third, nullptr);
  EXPECT_EQ(second->bounds.y, content->bounds.y);
  EXPECT_EQ(third->bounds.y, content->bounds.y + 10);
  EXPECT_EQ(first->bounds.y, content->bounds.y + 23);

  auto reorder =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("reorder"));
  ASSERT_NE(reorder, nullptr);
  EXPECT_EQ(reorder->to_string(), "nil");
}

TEST_F(ListBoxTest, reorderable_list_box_none_strategy_keeps_flow_positions_while_dragging)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.list-box/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}]
                    :style {:width 100}
                    :row-height 10
                    :visible-rows 3
                    :content-width 100
                    :reorderable? true
                    :reorder-visual-strategy :none})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto list_box = session.active_mode->children[0];
  auto content = list_box_content(list_box);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 3u);

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_move({5, 28});
  update_cycle();
  session.render_mode();

  EXPECT_EQ(content->children[0]->bounds.y, content->bounds.y);
  EXPECT_EQ(content->children[1]->bounds.y, content->bounds.y + 10);
  EXPECT_EQ(content->children[2]->bounds.y, content->bounds.y + 20);
}

TEST_F(ListBoxTest, reorderable_list_box_with_custom_mouse_row_emits_reorder_drop_event)
{
  runtime.eval(R"(
    (pixils/defmode custom-row
      {:extend 'ui/list-box-item
       :style {:width :fill
               :height 10}
       :on-mouse-down (fn [state event ctx]
                        (do
                          (pixils.ui/stop-propagation! event)
                          state))
       :on-mouse-up (fn [state event ctx]
                      (do
                        (pixils.ui/stop-propagation! event)
                        (pixils.ui/emit! (:view ctx)
                                         :list-box/item-click
                                         {:index (:index state)
                                          :value (:value state)
                                          :shift? false
                                          :ctrl? false})
                        state))
       :children [{:mode 'ui/text
                   :state {:value (pixils.ui/bind-state :label)}}]})

    (defun custom-row-child [index option]
      {:mode 'custom-row
       :state {:index index
               :value (:value option)
               :label (:label option)
               :selected-indices (pixils.ui/bind-state :selected-indices)}})

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
                    :reorderable? true
                    :item-child custom-row-child})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("reorder"));
  ASSERT_NE(reorder, nullptr);
  ASSERT_NE(reorder->to_string(), "nil");
  EXPECT_EQ(Roo::Dict::get_property(reorder, Roo::keyword("from-index"))
              ->num()
              .get_int(),
            0);
  EXPECT_EQ(Roo::Dict::get_property(reorder, Roo::keyword("to-index"))
              ->num()
              .get_int(),
            2);
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

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_up({5, 5});
  update_cycle();

  auto selected =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
  auto changes =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("changes"));
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(changes, nullptr);
  EXPECT_EQ(selected->to_string(), "[0]");
  EXPECT_EQ(changes->num().get_int(), 0);

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();

  selected =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
  changes =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("changes"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "[1 2]");

  input().mouse_down({5, 15});
  update_cycle();
  input().mouse_up({5, 15});
  update_cycle();

  selected =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("selected"));
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

  session.push_mode("root-mode", Roo::Constant::NIL);
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
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("activated"));
  ASSERT_NE(activated, nullptr);
  EXPECT_EQ(Roo::Dict::get_property(activated, Roo::keyword("value"))->to_string(),
            ":b");
  EXPECT_EQ(Roo::Dict::get_property(activated, Roo::keyword("selected-indices"))
              ->to_string(),
            "[1]");
}
