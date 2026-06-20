#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>

using CheckboxTest = RenderFixture;

namespace
{
  int child_int(const Roo::sptr_val& value, size_t index)
  {
    return Roo::get_child(*value, index)->num().get_int();
  }
} // namespace

TEST_F(CheckboxTest, checkbox_toggles_bound_state_and_emits_change)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:show-grid? true})
       :on {:checkbox/change (fn [state event ctx]
                               (assoc state :last-change (:payload event)))}
       :children [(pixils.ui.checkbox/make
                   {:label "Show grid"
                    :checked? (pixils.ui/bind-state :show-grid?)})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto checkbox = session.active_mode->children[0];
  ASSERT_NE(checkbox, nullptr);
  EXPECT_EQ(checkbox->mode->name, "ui/checkbox");
  ASSERT_EQ(checkbox->children.size(), 2u);
  EXPECT_EQ(checkbox->children[0]->mode->name, "ui/checkbox-box");
  EXPECT_EQ(checkbox->children[1]->mode->name, "ui/checkbox-label");

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_up({5, 5});
  update_cycle();

  auto show_grid =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("show-grid?"));
  auto last_change =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("last-change"));
  ASSERT_NE(show_grid, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(show_grid->to_string(), "false");
  EXPECT_EQ(last_change->to_string(), "{:checked? false :value nil}");
}

TEST_F(CheckboxTest, checkbox_mark_passes_include_one_pixel_horizontal_offset)
{
  auto result = runtime.eval(R"(
    (let [passes (pixils.ui.checkbox/checkbox-mark-passes {:w 12 :h 12})
          pass1 (nth passes 0)
          pass2 (nth passes 1)
          p1 (nth pass1 0)
          p2 (nth pass1 1)
          p3 (nth pass1 2)
          p1-offset (nth pass2 0)
          p2-offset (nth pass2 1)
          p3-offset (nth pass2 2)]
      [(count passes)
       (- (:x p1-offset) (:x p1))
       (- (:y p1-offset) (:y p1))
       (- (:x p2-offset) (:x p2))
       (- (:y p2-offset) (:y p2))
       (- (:x p3-offset) (:x p3))
       (- (:y p3-offset) (:y p3))])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 7u);
  EXPECT_EQ(child_int(result, 0), 2);
  EXPECT_EQ(child_int(result, 1), 1);
  EXPECT_EQ(child_int(result, 2), 0);
  EXPECT_EQ(child_int(result, 3), 1);
  EXPECT_EQ(child_int(result, 4), 0);
  EXPECT_EQ(child_int(result, 5), 1);
  EXPECT_EQ(child_int(result, 6), 0);
}

TEST_F(CheckboxTest, checkbox_mark_color_prefers_effective_style_text_color)
{
  auto result = runtime.eval(R"(
    (let [color (pixils.ui.checkbox/checkbox-mark-color
                 {}
                 {:view {:style {:text {:color {:r 0 :g 0 :b 0}}}
                         :effective-style {:text {:color {:r 220 :g 224 :b 228}}}}})]
      [(:r color) (:g color) (:b color)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 3u);
  EXPECT_EQ(child_int(result, 0), 220);
  EXPECT_EQ(child_int(result, 1), 224);
  EXPECT_EQ(child_int(result, 2), 228);
}
