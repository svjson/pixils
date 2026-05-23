#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using ScrollPaneTest = RenderFixture;

TEST_F(ScrollPaneTest, scroll_pane_offsets_content_inside_clipped_viewport)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defmode root-mode
      {:children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 40}
                    :content-size {:w 100 :h 80}
                    :offset {:x 10 :y 20}
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 2u);

  auto row = pane->children[0];
  auto footer = pane->children[1];
  ASSERT_NE(row, nullptr);
  ASSERT_NE(footer, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);
  ASSERT_EQ(viewport->children.size(), 1u);
  auto content = viewport->children[0];
  ASSERT_NE(content, nullptr);

  EXPECT_EQ(viewport->bounds.x, 0);
  EXPECT_EQ(viewport->bounds.y, 0);
  EXPECT_EQ(viewport->bounds.w, 36);
  EXPECT_EQ(viewport->bounds.h, 26);
  EXPECT_EQ(vertical_scrollbar->bounds.x, 36);
  EXPECT_EQ(vertical_scrollbar->bounds.w, 14);
  EXPECT_EQ(footer->bounds.y, 26);
  EXPECT_EQ(footer->bounds.h, 14);

  ASSERT_TRUE(viewport->effective_style.clip.has_value());
  EXPECT_TRUE(*viewport->effective_style.clip);
  EXPECT_EQ(content->bounds.x, -10);
  EXPECT_EQ(content->bounds.y, -20);
  EXPECT_EQ(content->bounds.w, 100);
  EXPECT_EQ(content->bounds.h, 80);
}

TEST_F(ScrollPaneTest, scroll_pane_without_horizontal_scroll_uses_content_width)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defmode root-mode
      {:children [(pixils.ui.scroll-pane/make
                   {:style {:height 40}
                    :content-size {:w 100 :h 80}
                    :scroll-x? false
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 1u);

  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);

  EXPECT_EQ(pane->bounds.w, 114);
  EXPECT_EQ(viewport->bounds.w, 100);
  EXPECT_EQ(vertical_scrollbar->bounds.x, 100);
  EXPECT_EQ(vertical_scrollbar->bounds.w, 14);
}

TEST_F(ScrollPaneTest, scroll_pane_fill_width_contributes_content_width_to_auto_parent)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defcomponent auto-panel
      {:style {:height 40
               :layout {:direction :column}}
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width :fill
                            :height 40}
                    :content-size {:w 100 :h 80}
                    :scroll-x? false
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
    (pixils/defmode root-mode
      {:children [{:mode 'auto-panel}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto panel = session.active_mode->children[0];
  ASSERT_NE(panel, nullptr);
  EXPECT_EQ(panel->bounds.w, 114);

  ASSERT_EQ(panel->children.size(), 1u);
  auto pane = panel->children[0];
  ASSERT_NE(pane, nullptr);
  EXPECT_EQ(pane->bounds.w, 114);
}

TEST_F(ScrollPaneTest, scroll_pane_without_content_size_keeps_fill_width_viewport)
{
  runtime.eval(R"(
    (pixils/defcomponent message-area
      {:class :ui/panel
       :style {:width :fill
               :height :fill}
       :children [{:mode 'ui/text
                   :state {:value "one\ntwo"}}]})

    (pixils/defcomponent info-box
      {:class :ui/panel
       :style {:width 200
               :height :fill}
       :children [{:mode 'ui/text
                   :state {:value "info"}}]})

    (pixils/defcomponent status-area
      {:style {:layout {:direction :row}
               :height 150
               :width :fill}
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width :fill
                            :height :fill}
                    :scroll-x? false
                    :children [{:mode 'message-area}]})
                  {:mode 'info-box}]})

    (pixils/defmode root-mode
      {:children [{:mode 'status-area}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  auto status = session.active_mode->children[0];
  ASSERT_NE(status, nullptr);
  ASSERT_EQ(status->children.size(), 2u);

  auto pane = status->children[0];
  auto info = status->children[1];
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(info, nullptr);
  EXPECT_EQ(pane->bounds.w, 120);
  EXPECT_EQ(info->bounds.x, 120);
  EXPECT_EQ(info->bounds.w, 200);

  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_EQ(row->children.size(), 2u);
  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);
  EXPECT_GT(viewport->bounds.w, 0);
  EXPECT_EQ(vertical_scrollbar->bounds.x, viewport->bounds.w);

  ASSERT_EQ(viewport->children.size(), 1u);
  auto content = viewport->children[0];
  ASSERT_EQ(content->children.size(), 1u);
  auto message = content->children[0];
  ASSERT_NE(message, nullptr);
  EXPECT_GT(message->bounds.w, 0);
}

TEST_F(ScrollPaneTest, scroll_pane_measures_runtime_content_growth)
{
  runtime.eval(R"(
    (pixils/defcomponent growing-content
      {:content-size (fn [state ctx] {:w 100 :h (:height state)})})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:content-state {:height 20} :grown? false})
       :update (fn [state ctx]
                 (if (:grown? state)
                   state
                   (assoc state :content-state {:height 200} :grown? true)))
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 80}
                    :scroll-x? false
                    :scroll-y? :auto
                    :state {:content-state (pixils.ui/bind-state :content-state)}
                    :children [{:mode 'growing-content
                                :state {:height (pixils.ui/bind-state :height)}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  session.update_mode();
  session.render_mode();

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);

  auto content_size =
    Lisple::Dict::get_property(pane->state, Lisple::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height = Lisple::Dict::get_property(content_size, Lisple::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 200);

  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);
  auto scrollbar = row->children[1];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);
  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);
  EXPECT_LT(handle->bounds.h, track->bounds.h);
}

TEST_F(ScrollPaneTest, scroll_pane_keeps_explicit_content_size_when_child_grows)
{
  runtime.eval(R"(
    (pixils/defcomponent growing-content
      {:content-size (fn [state ctx] {:w 100 :h (:height state)})})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:content-state {:height 20} :grown? false})
       :update (fn [state ctx]
                 (if (:grown? state)
                   state
                   (assoc state :content-state {:height 200} :grown? true)))
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 80}
                    :content-size {:w 100 :h 80}
                    :scroll-x? false
                    :state {:content-state (pixils.ui/bind-state :content-state)}
                    :children [{:mode 'growing-content
                                :state {:height (pixils.ui/bind-state :height)}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  session.update_mode();
  session.render_mode();

  session.update_mode();
  session.render_mode();

  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  auto content_size =
    Lisple::Dict::get_property(pane->state, Lisple::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height = Lisple::Dict::get_property(content_size, Lisple::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 80);
}

TEST_F(ScrollPaneTest, windows_3_scroll_pane_uses_theme_scrollbar_size)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width 50 :height 40}
                    :content-size {:w 100 :h 80}
                    :children [{:mode 'content-mode
                                :style {:width 100 :height 80}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 2u);

  auto row = pane->children[0];
  auto footer = pane->children[1];
  ASSERT_NE(row, nullptr);
  ASSERT_NE(footer, nullptr);
  ASSERT_EQ(row->children.size(), 2u);

  auto viewport = row->children[0];
  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);
  ASSERT_EQ(vertical_scrollbar->children.size(), 3u);

  EXPECT_EQ(viewport->bounds.w, 33);
  EXPECT_EQ(vertical_scrollbar->bounds.x, 33);
  EXPECT_EQ(vertical_scrollbar->bounds.w, 17);
  EXPECT_EQ(footer->bounds.y, 23);
  EXPECT_EQ(footer->bounds.h, 17);

  auto vertical_start_button = vertical_scrollbar->children[0];
  ASSERT_NE(vertical_start_button, nullptr);
  EXPECT_EQ(vertical_start_button->bounds.w, 15);
  EXPECT_EQ(vertical_start_button->bounds.h, 15);
  EXPECT_EQ(vertical_start_button->effective_style.content_rect(vertical_start_button->bounds).w,
            12);
  EXPECT_EQ(vertical_start_button->effective_style.content_rect(vertical_start_button->bounds).h,
            12);
}

TEST_F(ScrollPaneTest, windows_3_scroll_pane_uses_fixed_vertical_scrollbar_handle)
{
  runtime.eval(R"(
    (pixils/defmode content-mode {})
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [(pixils.ui.scroll-pane/make
                   {:style {:width 80 :height 120}
                    :content-size {:w 80 :h 300}
                    :scroll-x? false
                    :children [{:mode 'content-mode
                                :style {:width 80 :height 300}}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_EQ(pane->children.size(), 1u);

  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_EQ(row->children.size(), 2u);

  auto vertical_scrollbar = row->children[1];
  ASSERT_NE(vertical_scrollbar, nullptr);
  ASSERT_EQ(vertical_scrollbar->children.size(), 3u);

  auto track = vertical_scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);

  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);
  EXPECT_GT(track->bounds.h, 15);
  EXPECT_EQ(handle->bounds.h, 15);
  EXPECT_EQ(handle->bounds.w, track->bounds.w);
  EXPECT_LT(handle->bounds.h, track->bounds.h);
}
