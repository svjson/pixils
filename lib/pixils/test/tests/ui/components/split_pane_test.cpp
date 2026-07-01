#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <string>

using SplitPaneTest = RenderFixture;

#define SETTLE_LAYOUT() \
  do                    \
  {                     \
    update_cycle();     \
    render_cycle();     \
    update_cycle();     \
    render_cycle();     \
  } while (false)

namespace
{
  void eval_split_pane_fixture(Roo::Runtime& runtime, const char* options)
  {
    runtime.eval(std::string(R"(
      (pixils/defmode first-pane {})
      (pixils/defmode second-pane {})
      (pixils/defmode root-mode
        {:children [(pixils.ui.split-pane/make )") +
                 options + R"(
                    )]})
    )");
  }

}

TEST_F(SplitPaneTest, split_pane_lays_out_horizontal_panes_from_ratio)
{
  eval_split_pane_fixture(runtime, R"(
    {:style {:width 200 :height 80}
     :axis :x
     :split 0.25
     :resizer-size 6
     :min-first 20
     :min-second 20
     :children [{:mode 'first-pane} {:mode 'second-pane}]})");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 3u);

  auto primary = pane->children[0];
  auto resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.x, 0);
  EXPECT_EQ(primary->bounds.y, 0);
  EXPECT_EQ(primary->bounds.w, 48);
  EXPECT_EQ(primary->bounds.h, 80);
  EXPECT_EQ(resizer->bounds.x, 48);
  EXPECT_EQ(resizer->bounds.w, 6);
  EXPECT_EQ(resizer->bounds.h, 80);
  EXPECT_EQ(secondary->bounds.x, 54);
  EXPECT_EQ(secondary->bounds.w, 146);
  EXPECT_EQ(secondary->bounds.h, 80);
}

TEST_F(SplitPaneTest, split_pane_lays_out_horizontal_panes_on_first_render)
{
  eval_split_pane_fixture(runtime, R"(
    {:style {:width :fill :height 210}
     :split 0.44
     :children [{:mode 'first-pane
                 :style {:width :fill :height :fill}}
                {:mode 'second-pane
                 :style {:width :fill :height :fill}}]})");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 3u);

  auto primary = pane->children[0];
  auto resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(pane->bounds.w, 320);
  EXPECT_EQ(primary->bounds.x, 0);
  EXPECT_EQ(primary->bounds.y, 0);
  EXPECT_EQ(primary->bounds.w, 138);
  EXPECT_EQ(primary->bounds.h, 210);
  EXPECT_EQ(resizer->bounds.x, 138);
  EXPECT_EQ(resizer->bounds.w, 6);
  EXPECT_EQ(resizer->bounds.h, 210);
  EXPECT_EQ(secondary->bounds.x, 144);
  EXPECT_EQ(secondary->bounds.w, 176);
  EXPECT_EQ(secondary->bounds.h, 210);
}

TEST_F(SplitPaneTest, dragging_horizontal_split_pane_resizer_updates_proportions)
{
  eval_split_pane_fixture(runtime, R"(
    {:style {:width 200 :height 80}
     :axis :x
     :split 0.25
     :resizer-size 6
     :min-first 20
     :min-second 20
     :children [{:mode 'first-pane} {:mode 'second-pane}]})");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  auto pane = session.active_mode->children[0];
  auto resizer = pane->children[1];
  ASSERT_NE(resizer, nullptr);

  input().mouse_down({resizer->bounds.x + 2, resizer->bounds.y + 10});
  update_cycle();

  input().mouse_move({resizer->bounds.x + 52, resizer->bounds.y + 10});
  SETTLE_LAYOUT();

  pane = session.active_mode->children[0];
  ASSERT_EQ(pane->children.size(), 3u);
  auto primary = pane->children[0];
  resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.w, 98);
  EXPECT_EQ(resizer->bounds.x, 98);
  EXPECT_EQ(secondary->bounds.x, 104);
  EXPECT_EQ(secondary->bounds.w, 96);
}

TEST_F(SplitPaneTest, split_pane_supports_vertical_axis)
{
  eval_split_pane_fixture(runtime, R"(
    {:style {:width 120 :height 160}
     :axis :y
     :split 0.5
     :resizer-size 8
     :min-first 20
     :min-second 20
     :children [{:mode 'first-pane} {:mode 'second-pane}]})");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  auto pane = session.active_mode->children[0];
  ASSERT_EQ(pane->children.size(), 3u);
  auto primary = pane->children[0];
  auto resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.w, 120);
  EXPECT_EQ(primary->bounds.h, 76);
  EXPECT_EQ(resizer->bounds.y, 76);
  EXPECT_EQ(resizer->bounds.w, 120);
  EXPECT_EQ(resizer->bounds.h, 8);
  EXPECT_EQ(secondary->bounds.y, 84);
  EXPECT_EQ(secondary->bounds.h, 76);

  input().mouse_down({resizer->bounds.x + 10, resizer->bounds.y + 2});
  update_cycle();

  input().mouse_move({resizer->bounds.x + 10, resizer->bounds.y + 32});
  SETTLE_LAYOUT();

  pane = session.active_mode->children[0];
  primary = pane->children[0];
  resizer = pane->children[1];
  secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.h, 106);
  EXPECT_EQ(resizer->bounds.y, 106);
  EXPECT_EQ(secondary->bounds.y, 114);
  EXPECT_EQ(secondary->bounds.h, 46);
}

TEST_F(SplitPaneTest, split_pane_drag_is_clamped_by_minimum_pane_sizes)
{
  eval_split_pane_fixture(runtime, R"(
    {:style {:width 200 :height 80}
     :axis :x
     :split 0.5
     :resizer-size 6
     :min-first 40
     :min-second 50
     :children [{:mode 'first-pane} {:mode 'second-pane}]})");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  auto pane = session.active_mode->children[0];
  auto resizer = pane->children[1];
  ASSERT_NE(resizer, nullptr);

  input().mouse_down({resizer->bounds.x + 2, resizer->bounds.y + 10});
  update_cycle();

  input().mouse_move({-200, resizer->bounds.y + 10});
  SETTLE_LAYOUT();

  pane = session.active_mode->children[0];
  auto primary = pane->children[0];
  resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.w, 40);
  EXPECT_EQ(resizer->bounds.x, 40);
  EXPECT_EQ(secondary->bounds.x, 46);
  EXPECT_EQ(secondary->bounds.w, 154);
}

TEST_F(SplitPaneTest, split_pane_uses_theme_resizer_size_when_not_overridden)
{
  runtime.eval(R"(
    (pixils/deftheme split-pane-theme
      {:default-variant :light
       :vars {:light {:split-pane-resizer-size 10}}
       :styles {'(:ui/split-pane-resizer {:axis :x})
                {:width (pixils/var :split-pane-resizer-size)}

                '(:ui/split-pane-resizer {:axis :y})
                {:height (pixils/var :split-pane-resizer-size)}}})

    (pixils/defmode first-pane {})
    (pixils/defmode second-pane {})
    (pixils/defmode root-mode
      {:theme 'split-pane-theme
       :children [(pixils.ui.split-pane/make
                   {:style {:width 200 :height 80}
                    :axis :x
                    :split 0.5
                    :min-first 20
                    :min-second 20
                    :children [{:mode 'first-pane} {:mode 'second-pane}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 3u);

  auto primary = pane->children[0];
  auto resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.w, 95);
  EXPECT_EQ(resizer->bounds.x, 95);
  EXPECT_EQ(resizer->bounds.w, 10);
  EXPECT_EQ(secondary->bounds.x, 105);
  EXPECT_EQ(secondary->bounds.w, 95);
}

TEST_F(SplitPaneTest, vertical_split_pane_uses_theme_resizer_size_when_not_overridden)
{
  runtime.eval(R"(
    (pixils/deftheme split-pane-theme
      {:default-variant :light
       :vars {:light {:split-pane-resizer-size 10}}
       :styles {'(:ui/split-pane-resizer {:axis :x})
                {:width (pixils/var :split-pane-resizer-size)}

                '(:ui/split-pane-resizer {:axis :y})
                {:height (pixils/var :split-pane-resizer-size)}}})

    (pixils/defmode first-pane {})
    (pixils/defmode second-pane {})
    (pixils/defmode root-mode
      {:theme 'split-pane-theme
       :children [(pixils.ui.split-pane/make
                   {:style {:width 120 :height 160}
                    :axis :y
                    :split 0.5
                    :min-first 20
                    :min-second 20
                    :children [{:mode 'first-pane} {:mode 'second-pane}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 3u);

  auto primary = pane->children[0];
  auto resizer = pane->children[1];
  auto secondary = pane->children[2];
  ASSERT_NE(primary, nullptr);
  ASSERT_NE(resizer, nullptr);
  ASSERT_NE(secondary, nullptr);

  EXPECT_EQ(primary->bounds.h, 75);
  EXPECT_EQ(resizer->bounds.y, 75);
  EXPECT_EQ(resizer->bounds.w, 120);
  EXPECT_EQ(resizer->bounds.h, 10);
  EXPECT_EQ(secondary->bounds.y, 85);
  EXPECT_EQ(secondary->bounds.h, 75);
}

TEST_F(SplitPaneTest, split_pane_slots_pass_parent_state_to_children)
{
  runtime.eval(R"(
    (pixils/defmode state-reader
      {:update (fn [state ctx]
                 (assoc state :seen (:shared state)))})
    (pixils/defmode second-pane {})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:shared 7})
       :children [(pixils.ui.split-pane/make
                   {:style {:width 200 :height 80}
                    :axis :x
                    :split 0.5
                    :state {:shared (pixils.ui/bind-state :shared)}
                    :children [{:mode 'state-reader
                                :state {:shared (pixils.ui/bind-state :shared)}}
                               {:mode 'second-pane}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 3u);

  auto primary = pane->children[0];
  ASSERT_NE(primary, nullptr);
  ASSERT_EQ(primary->children.size(), 1u);

  auto reader = primary->children[0];
  ASSERT_NE(reader, nullptr);
  EXPECT_EQ(reader->state->to_string(), "{:shared 7 :seen 7}");
}
