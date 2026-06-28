#include "../../render_fixture.h"
#include <pixils/program.h>
#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>

using GroupBoxTest = RenderFixture;

namespace
{
  std::shared_ptr<Pixils::Runtime::View> find_first_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;
    for (const auto& child : view->children)
    {
      if (auto found = find_first_mode(child, mode_name)) return found;
    }
    return nullptr;
  }
} // namespace

TEST_F(GroupBoxTest, windows_group_box_constrains_fill_row_to_frame_content)
{
  runtime.eval(R"(
    (pixils/deftheme group-box-layout-theme
      {:extend 'pixils/windows-3
       :styles {:appearance-frame {:width 120
                                   :height 80}
                :label-column {:width 36}}})

    (pixils/defprogram group-box-test-program
      {:theme 'group-box-layout-theme
       :initial-mode 'root-mode})

    (pixils/defcomponent fixed-label
      {:class :label-column
       :style {:height 12}})

    (pixils/defcomponent fill-control-inner
      {:style {:width :fill
               :height :fill}})

    (pixils/defcomponent fill-control
      {:style {:width :fill
               :height 20
               :min-width 120
               :padding [2 4]
               :border {:line-style :solid
                        :color {:r 0 :g 0 :b 0}
                        :thickness 1}}
       :children [{:mode 'fill-control-inner}]})

    (pixils/defcomponent appearance-row
      {:style {:width :fill
               :layout {:direction :row}}
       :children [{:mode 'fixed-label}
                  {:mode 'fill-control}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.group-box/make
                   {:title "Appearance"
                    :class :appearance-frame
                    :children [{:mode 'appearance-row}]})]})
  )");

  Pixils::load_program(runtime, session);
  session.update_mode();
  Pixils::UI::layout_view_tree(session.active_mode,
                               {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h},
                               runtime,
                               hook_args.render_args[1]);

  auto frame = find_first_mode(session.active_mode, "ui/group-box-frame");
  auto group = find_first_mode(session.active_mode, "ui/group-box");
  auto body = find_first_mode(session.active_mode, "ui/group-box-body");
  auto row = find_first_mode(session.active_mode, "appearance-row");
  auto label = find_first_mode(session.active_mode, "fixed-label");
  auto control = find_first_mode(session.active_mode, "fill-control");
  auto control_inner = find_first_mode(session.active_mode, "fill-control-inner");

  ASSERT_NE(frame, nullptr);
  ASSERT_NE(group, nullptr);
  ASSERT_NE(body, nullptr);
  ASSERT_NE(row, nullptr);
  ASSERT_NE(label, nullptr);
  ASSERT_NE(control, nullptr);
  ASSERT_NE(control_inner, nullptr);

  const auto body_right = body->bounds.x + body->bounds.w;
  const auto row_right = row->bounds.x + row->bounds.w;
  const auto label_right = label->bounds.x + label->bounds.w;
  const auto control_right = control->bounds.x + control->bounds.w;
  const auto control_content = control->effective_style.content_rect(control->bounds);

  EXPECT_GT(group->bounds.w, 120);
  ASSERT_TRUE(frame->effective_style.clip.has_value());
  EXPECT_TRUE(*frame->effective_style.clip);
  EXPECT_GT(body->bounds.x, frame->bounds.x);
  EXPECT_EQ(row->bounds.x, body->bounds.x);
  EXPECT_EQ(row->bounds.w, body->bounds.w);
  EXPECT_EQ(label->bounds.x, row->bounds.x);
  EXPECT_EQ(control->bounds.x, label_right);
  EXPECT_EQ(row_right, body_right);
  EXPECT_EQ(control_right, body_right);
  EXPECT_EQ(control_inner->bounds.x, control_content.x);
  EXPECT_EQ(control_inner->bounds.w, control_content.w);
  EXPECT_EQ(control_inner->bounds.x + control_inner->bounds.w,
            control_content.x + control_content.w);
}

TEST_F(GroupBoxTest, max_width_caps_preferred_group_box_and_frame_clips_overflow)
{
  runtime.eval(R"(
    (pixils/deftheme group-box-layout-theme
      {:extend 'pixils/windows-3
       :styles {:appearance-frame {:width 120
                                   :max-width 120
                                   :height 80}}})

    (pixils/defprogram group-box-test-program
      {:theme 'group-box-layout-theme
       :initial-mode 'root-mode})

    (pixils/defcomponent oversized-content
      {:style {:width 180
               :height 12
               :background {:r 255 :g 0 :b 0}}})

    (pixils/defmode root-mode
      {:children [(pixils.ui.group-box/make
                   {:title "Appearance"
                    :class :appearance-frame
                    :children [{:mode 'oversized-content}]})]})
  )");

  Pixils::load_program(runtime, session);
  session.update_mode();
  Pixils::UI::layout_view_tree(session.active_mode,
                               {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h},
                               runtime,
                               hook_args.render_args[1]);

  auto group = find_first_mode(session.active_mode, "ui/group-box");
  auto frame = find_first_mode(session.active_mode, "ui/group-box-frame");
  auto body = find_first_mode(session.active_mode, "ui/group-box-body");
  auto content = find_first_mode(session.active_mode, "oversized-content");

  ASSERT_NE(group, nullptr);
  ASSERT_NE(frame, nullptr);
  ASSERT_NE(body, nullptr);
  ASSERT_NE(content, nullptr);

  const auto body_right = body->bounds.x + body->bounds.w;
  const auto content_right = content->bounds.x + content->bounds.w;

  EXPECT_EQ(group->bounds.w, 120);
  EXPECT_GT(content_right, body_right);
  ASSERT_TRUE(frame->effective_style.clip.has_value());
  EXPECT_TRUE(*frame->effective_style.clip);
}
