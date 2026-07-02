#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

#include <vector>

using ProgressBarTest = RenderFixture;

namespace
{
  Roo::sptr_val get_state_key(const std::shared_ptr<Pixils::Runtime::View>& view,
                              const std::string& key)
  {
    return Roo::Dict::get_property(view->state, Roo::keyword(key));
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

TEST_F(ProgressBarTest, progress_bar_make_creates_bound_value_state)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:load-progress 40})
       :children [(pixils.ui.progress-bar/make
                   {:style {:width 100 :height 12}
                    :value (pixils.ui/bind-state :load-progress)
                    :max 80})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto progress = session.active_mode->children[0];
  ASSERT_NE(progress, nullptr);
  EXPECT_EQ(progress->mode->name, "ui/progress-bar");

  auto value = get_state_key(progress, "value");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 40);
}

TEST_F(ProgressBarTest, progress_bar_renders_horizontal_fill_from_value_ratio)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.progress-bar/make
                   {:style {:width 100 :height 12
                            :border {:thickness 0}}
                    :value 25
                    :max 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, {0, 0, 100, 12}));
  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, {0, 0, 25, 12}));
}

TEST_F(ProgressBarTest, progress_bar_uses_theme_dimensions_when_instance_style_omits_them)
{
  runtime.eval(R"(
    (pixils/deftheme tall-progress-theme
      {:styles {'ui/progress-bar {:height 28}}})

    (pixils/defmode root-mode
      {:theme 'tall-progress-theme
       :children [(pixils.ui.progress-bar/make
                   {:style {:width 100}
                    :value 25
                    :max 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto progress = session.active_mode->children[0];
  ASSERT_NE(progress, nullptr);
  EXPECT_EQ(progress->bounds.w, 100);
  EXPECT_EQ(progress->bounds.h, 28);
}

TEST_F(ProgressBarTest, progress_bar_does_not_inset_fill_for_bordered_styles)
{
  auto rect = runtime.eval(R"(
    (pixils.ui.progress-bar/progress-bar-fill-rect
      {:value 25 :min 0 :max 100}
      {:view {:bounds {:x 0 :y 0 :w 100 :h 12}
              :effective-style {:border {:thickness 1}}}})
  )");

  ASSERT_NE(rect, nullptr);
  EXPECT_EQ(Roo::Dict::get_property(rect, Roo::keyword("x"))->num().get_int(), 0);
  EXPECT_EQ(Roo::Dict::get_property(rect, Roo::keyword("y"))->num().get_int(), 0);
  EXPECT_EQ(Roo::Dict::get_property(rect, Roo::keyword("w"))->num().get_int(), 25);
  EXPECT_EQ(Roo::Dict::get_property(rect, Roo::keyword("h"))->num().get_int(), 12);
}

TEST_F(ProgressBarTest, progress_bar_clamps_value_before_rendering)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.progress-bar/make
                   {:style {:width 100 :height 12
                            :border {:thickness 0}}
                    :value 150
                    :max 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto progress = session.active_mode->children[0];
  ASSERT_NE(progress, nullptr);
  auto value = get_state_key(progress, "value");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 100);
  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, {0, 0, 100, 12}));
}

TEST_F(ProgressBarTest, progress_bar_renders_vertical_fill_from_bottom)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.progress-bar/make
                   {:style {:width 12 :height 100
                            :border {:thickness 0}}
                    :axis :y
                    :value 25
                    :max 100})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, {0, 0, 12, 100}));
  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, {0, 75, 12, 25}));
}
