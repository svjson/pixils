#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>

using OptionBoxTest = RenderFixture;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

  bool has_fill_rect(const std::vector<RenderOperation>& ops, const SDL_Rect& rect)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::FILL_RECT && op.rendered_rect.x == rect.x &&
          op.rendered_rect.y == rect.y && op.rendered_rect.w == rect.w &&
          op.rendered_rect.h == rect.h)
      {
        return true;
      }
      if (has_fill_rect(op.sub_ops, rect)) return true;
    }
    return false;
  }
} // namespace

TEST_F(OptionBoxTest, option_box_selects_and_keeps_selected_by_default)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:on {:option-box/change (fn [state event ctx]
                                  (assoc state :last-change (:payload event)))}
       :children [(pixils.ui.option-box/make
                   {:label "Terrain"
                    :value :terrain
                    :selected? false})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto option = session.active_mode->children[0];
  ASSERT_NE(option, nullptr);
  EXPECT_EQ(option->mode->name, "ui/option-box");
  ASSERT_EQ(option->children.size(), 2u);
  EXPECT_EQ(option->children[0]->mode->name, "ui/option-box-indicator");
  EXPECT_EQ(option->children[1]->mode->name, "ui/option-box-label");

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_up({5, 5});
  update_cycle();
  update_cycle();

  auto selected = get_key(option->state, "selected?");
  auto last_change = get_key(session.active_mode->state, "last-change");
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(selected->to_string(), "true");
  EXPECT_EQ(last_change->to_string(), "{:selected? true :value :terrain :index nil}");

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_up({5, 5});
  update_cycle();
  update_cycle();

  selected = get_key(option->state, "selected?");
  last_change = get_key(session.active_mode->state, "last-change");
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(selected->to_string(), "true");
  EXPECT_EQ(last_change->to_string(), "{:selected? true :value :terrain :index nil}");
}

TEST_F(OptionBoxTest, option_box_group_selects_one_option_and_keeps_selection_by_default)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:on {:option-box-group/change (fn [state event ctx]
                                        (assoc state :last-change (:payload event)))}
       :children [(pixils.ui.option-box/make-group
                   {:options [{:label "Tiles" :value :tiles}
                              {:label "Terrain" :value :terrain}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto group = session.active_mode->children[0];
  ASSERT_NE(group, nullptr);
  EXPECT_EQ(group->mode->name, "ui/option-box-group");
  ASSERT_EQ(group->children.size(), 1u);
  auto column = group->children[0];
  ASSERT_NE(column, nullptr);
  ASSERT_EQ(column->children.size(), 2u);

  auto tiles = column->children[0];
  auto terrain = column->children[1];
  ASSERT_NE(tiles, nullptr);
  ASSERT_NE(terrain, nullptr);
  EXPECT_EQ(get_key(tiles->state, "selected?")->to_string(), "true");
  EXPECT_EQ(get_key(terrain->state, "selected?")->to_string(), "false");

  input().mouse_down({terrain->bounds.x + (terrain->bounds.w / 2),
                      terrain->bounds.y + (terrain->bounds.h / 2)});
  update_cycle();
  input().mouse_up({terrain->bounds.x + (terrain->bounds.w / 2),
                    terrain->bounds.y + (terrain->bounds.h / 2)});
  update_cycle();
  update_cycle();

  auto selected = get_key(group->state, "selected");
  auto last_change = get_key(session.active_mode->state, "last-change");
  ASSERT_NE(selected, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(selected->to_string(), ":terrain");
  EXPECT_EQ(last_change->to_string(),
            "{:selected :terrain :value :terrain :index 1 :option {:label \"Terrain\" "
            ":value :terrain}}");

  column = group->children[0];
  tiles = column->children[0];
  terrain = column->children[1];
  EXPECT_EQ(get_key(tiles->state, "selected?")->to_string(), "false");
  EXPECT_EQ(get_key(terrain->state, "selected?")->to_string(), "true");

  input().mouse_down({terrain->bounds.x + (terrain->bounds.w / 2),
                      terrain->bounds.y + (terrain->bounds.h / 2)});
  update_cycle();
  input().mouse_up({terrain->bounds.x + (terrain->bounds.w / 2),
                    terrain->bounds.y + (terrain->bounds.h / 2)});
  update_cycle();
  update_cycle();

  selected = get_key(group->state, "selected");
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), ":terrain");
  EXPECT_EQ(get_key(terrain->state, "selected?")->to_string(), "true");
}

TEST_F(OptionBoxTest, windows_option_box_indicator_draws_unselected_outer_circle)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :children [{:mode 'ui/option-box-indicator
                   :style {:width 12 :height 12}
                   :state {:selected false}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{6, 1, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{6, 11, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{1, 6, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{11, 6, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{4, 6, 5, 1}));
}

TEST_F(OptionBoxTest, windows_option_box_indicator_draws_selected_inner_circle)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :children [{:mode 'ui/option-box-indicator
                   :style {:width 12 :height 12}
                   :state {:selected true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{6, 1, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{4, 6, 5, 1}));
}

TEST_F(OptionBoxTest, windows_option_box_indicator_draws_pressed_thicker_outer_circle)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :children [{:mode 'ui/option-box-indicator
                   :style {:width 12 :height 12}
                   :state {:selected false
                           :pressed true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{6, 1, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{6, 2, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 6, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 6, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{4, 6, 5, 1}));
}
