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
  EXPECT_EQ(option->children[0]->mode->name, "ui/menu-option-indicator");
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
