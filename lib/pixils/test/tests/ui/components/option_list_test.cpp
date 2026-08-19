#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>

using OptionListTest = RenderFixture;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

  std::shared_ptr<Pixils::Runtime::View> option_list_content(
    const std::shared_ptr<Pixils::Runtime::View>& option_list)
  {
    if (!option_list || option_list->children.empty()) return nullptr;
    auto scroll_pane = option_list->children[0];
    if (!scroll_pane || scroll_pane->children.empty()) return nullptr;
    auto row = scroll_pane->children[0];
    if (!row || row->children.empty()) return nullptr;
    auto viewport = row->children[0];
    if (!viewport || viewport->children.empty()) return nullptr;
    return viewport->children[0];
  }
} // namespace

TEST_F(OptionListTest, option_list_emits_item_interaction_without_owning_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:on {:option-item/click (fn [state event ctx]
                                 (assoc state :last-click (:payload event)))}
       :children [(pixils.ui.option-list/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :selected-indices [0]
                    :row-height 20
                    :style {:width 100
                            :height 40}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto option_list = session.active_mode->children[0];
  ASSERT_NE(option_list, nullptr);
  ASSERT_NE(option_list->definition, nullptr);
  EXPECT_EQ(option_list->definition->name, "ui/option-list");

  auto content = option_list_content(option_list);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);
  auto first = content->children[0];
  auto second = content->children[1];
  ASSERT_NE(first->definition, nullptr);
  ASSERT_NE(second->definition, nullptr);
  EXPECT_EQ(first->definition->name, "ui/option-item");
  EXPECT_EQ(second->definition->name, "ui/option-item");
  EXPECT_EQ(get_key(first->ui_state, "index")->to_string(), "0");
  EXPECT_EQ(get_key(first->ui_state, "selected-indices")->to_string(), "[0]");
  EXPECT_EQ(get_key(first->ui_state, "selected")->to_string(), "true");
  EXPECT_EQ(get_key(second->ui_state, "selected")->to_string(), "false");

  input().mouse_down(
    {second->bounds.x + (second->bounds.w / 2), second->bounds.y + (second->bounds.h / 2)});
  update_cycle();
  input().mouse_up(
    {second->bounds.x + (second->bounds.w / 2), second->bounds.y + (second->bounds.h / 2)});
  update_cycle();
  update_cycle();

  auto payload = get_key(session.active_mode->state, "last-click");
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(get_key(payload, "index")->num().get_int(), 1);
  EXPECT_EQ(get_key(payload, "value")->to_string(), ":b");
  EXPECT_EQ(get_key(option_list->state, "selected-indices")->to_string(), "[0]");
  EXPECT_EQ(get_key(option_list->ui_state, "selected-indices")->to_string(), "[0]");
}

TEST_F(OptionListTest, option_list_uses_theme_row_height_and_visible_rows)
{
  runtime.eval(R"(
    (pixils/deftheme option-list-size-theme
      {:default-variant :base
       :vars {:base {:list-box-row-height 37
                     :list-box-visible-rows 2}}})

    (pixils/defmode root-mode
      {:theme ['pixils/base-theme 'option-list-size-theme]
       :children [(pixils.ui.option-list/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}
                              {:value :c :label "Gamma"}
                              {:value :d :label "Delta"}]
                    :style {:width 100}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();
  frame_cycle();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto option_list = session.active_mode->children[0];
  ASSERT_NE(option_list, nullptr);
  auto content = option_list_content(option_list);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 4u);

  EXPECT_EQ(content->children[0]->bounds.h, 37);
  EXPECT_EQ(get_key(option_list->ui_state, "computed-row-height")->num().get_int(), 37);
  EXPECT_EQ(get_key(option_list->ui_state, "computed-visible-rows")->num().get_int(), 2);
  EXPECT_EQ(option_list->bounds.h, 76);
}

TEST_F(OptionListTest, option_list_respects_themed_fill_height)
{
  runtime.eval(R"(
    (pixils/deftheme fill-option-list-theme
      {:styles {'ui/option-list {:height :fill}}})

    (pixils/defmode root-mode
      {:theme ['pixils/base-theme 'fill-option-list-theme]
       :style {:width 100
               :height 160
               :layout {:direction :column}}
       :children [(pixils.ui.option-list/make
                   {:options [{:value :a :label "Alpha"}]
                    :row-height 20
                    :fixed-visible-rows? false})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();
  frame_cycle();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto option_list = session.active_mode->children[0];
  ASSERT_NE(option_list, nullptr);
  EXPECT_EQ(option_list->bounds.h, session.active_mode->bounds.h);
}

TEST_F(OptionListTest, option_list_items_fill_themed_container_width)
{
  runtime.eval(R"(
    (pixils/deftheme wide-option-list-theme
      {:styles {'ui/option-list {:width 200}}})

    (pixils/defcomponent fixed-width-option
      {:extend 'ui/option-item
       :style {:width 30
               :height 10}})

    (pixils/defmode root-mode
      {:theme ['pixils/base-theme 'wide-option-list-theme]
       :children [(pixils.ui.option-list/make
                   {:options [{:value :a :label "Alpha"}
                              {:value :b :label "Beta"}]
                    :row-height 10
                    :visible-rows 2
                    :item=> 'fixed-width-option})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();
  frame_cycle();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto option_list = session.active_mode->children[0];
  ASSERT_NE(option_list, nullptr);
  EXPECT_EQ(option_list->bounds.w, 200);

  auto content = option_list_content(option_list);
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 2u);
  EXPECT_GT(content->bounds.w, 30);
  EXPECT_EQ(content->children[0]->bounds.w, content->bounds.w);
  EXPECT_EQ(content->children[1]->bounds.w, content->bounds.w);
}
