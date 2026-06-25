#include "../../render_fixture.h"
#include <pixils/program.h>
#include <pixils/ui/style.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>
#include <string>

class SliderTest : public RenderFixture
{
 protected:
  std::shared_ptr<Pixils::Runtime::View> render_slider_for_theme(
    const std::string& theme)
  {
    runtime.eval(std::string(R"(
      (pixils/defprogram slider-test-program
        {:theme ')") + theme + R"(
         :initial-mode 'root-mode})

      (pixils/defmode root-mode
        {:children [(pixils.ui.slider/make
                     {:style {:width 100 :height 18}
                      :value 5
                      :min 0
                      :max 10
                      :step 1})]})
    )");

    Pixils::load_program(runtime, session);
    session.update_mode();
    session.render_mode();

    if (!session.active_mode || session.active_mode->children.empty()) return nullptr;
    return session.active_mode->children[0];
  }
};

TEST_F(SliderTest, slider_drag_updates_bound_value)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:zoom 1})
       :children [(pixils.ui.slider/make
                   {:style {:width 100 :height 10}
                    :value (pixils.ui/bind-state :zoom)
                    :min 1
                    :max 4
                    :step 1})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({95, 5});
  update_cycle();

  auto zoom =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("zoom"));
  ASSERT_NE(zoom, nullptr);
  EXPECT_EQ(zoom->num().get_int(), 4);
}

TEST_F(SliderTest, slider_track_rect_is_centered_on_handle_travel)
{
  auto result = runtime.eval(R"(
    (let [horizontal-ctx {:view {:bounds {:x 0 :y 0 :w 100 :h 21}}}
          vertical-ctx {:view {:bounds {:x 0 :y 0 :w 21 :h 100}}}
          horizontal (pixils.ui.slider/slider-track-rect
                       {:axis :x :value 5 :min 0 :max 10 :thumb-shape :windows}
                       horizontal-ctx)
          vertical (pixils.ui.slider/slider-track-rect
                     {:axis :y :value 5 :min 0 :max 10 :thumb-shape :windows}
                     vertical-ctx)]
      [(:x horizontal) (:y horizontal) (:w horizontal) (:h horizontal)
       (:x vertical) (:y vertical) (:w vertical) (:h vertical)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 8u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 10);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 8);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 79);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 4);
  EXPECT_EQ(Roo::get_child(*result, 4)->num().get_int(), 8);
  EXPECT_EQ(Roo::get_child(*result, 5)->num().get_int(), 10);
  EXPECT_EQ(Roo::get_child(*result, 6)->num().get_int(), 4);
  EXPECT_EQ(Roo::get_child(*result, 7)->num().get_int(), 79);
}

TEST_F(SliderTest, slider_track_rect_uses_content_box_inside_outer_border)
{
  auto result = runtime.eval(R"(
    (let [horizontal-ctx {:view {:bounds {:x 0 :y 0 :w 100 :h 21}
                                 :effective-style {:border {:thickness 2}}}}
          vertical-ctx {:view {:bounds {:x 0 :y 0 :w 21 :h 100}
                               :effective-style {:border {:thickness 2}}}}
          horizontal (pixils.ui.slider/slider-track-rect
                       {:axis :x :value 5 :min 0 :max 10 :thumb-shape :windows}
                       horizontal-ctx)
          vertical (pixils.ui.slider/slider-track-rect
                     {:axis :y :value 5 :min 0 :max 10 :thumb-shape :windows}
                     vertical-ctx)]
      [(:x horizontal) (:y horizontal) (:w horizontal) (:h horizontal)
       (:x vertical) (:y vertical) (:w vertical) (:h vertical)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 8u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 8);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 6);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 79);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 4);
  EXPECT_EQ(Roo::get_child(*result, 4)->num().get_int(), 6);
  EXPECT_EQ(Roo::get_child(*result, 5)->num().get_int(), 8);
  EXPECT_EQ(Roo::get_child(*result, 6)->num().get_int(), 4);
  EXPECT_EQ(Roo::get_child(*result, 7)->num().get_int(), 79);
}

TEST_F(SliderTest, slider_standard_track_rect_uses_full_axis)
{
  auto result = runtime.eval(R"(
    (let [horizontal-ctx {:view {:bounds {:x 0 :y 0 :w 100 :h 21}}}
          vertical-ctx {:view {:bounds {:x 0 :y 0 :w 21 :h 100}}}
          horizontal (pixils.ui.slider/slider-track-rect
                       {:axis :x :value 5 :min 0 :max 10}
                       horizontal-ctx)
          vertical (pixils.ui.slider/slider-track-rect
                     {:axis :y :value 5 :min 0 :max 10}
                     vertical-ctx)]
      [(:x horizontal) (:y horizontal) (:w horizontal) (:h horizontal)
       (:x vertical) (:y vertical) (:w vertical) (:h vertical)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 8u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 0);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 9);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 100);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 3);
  EXPECT_EQ(Roo::get_child(*result, 4)->num().get_int(), 9);
  EXPECT_EQ(Roo::get_child(*result, 5)->num().get_int(), 0);
  EXPECT_EQ(Roo::get_child(*result, 6)->num().get_int(), 3);
  EXPECT_EQ(Roo::get_child(*result, 7)->num().get_int(), 100);
}

TEST_F(SliderTest, slider_windows_style_uses_bevel_or_state_override)
{
  auto result = runtime.eval(R"(
    (let [bevel-ctx {:view {:effective-style {:border {:line-style :bevel}}}}
          solid-ctx {:view {:effective-style {:border {:line-style :solid
                                                       :top {:color {:r 1 :g 1 :b 1}}}}}}]
      [(if (pixils.ui.slider/slider-windows-style? {} bevel-ctx) 1 0)
       (if (pixils.ui.slider/slider-windows-style? {} solid-ctx) 1 0)
       (if (pixils.ui.slider/slider-windows-style? {:thumb-shape :windows} solid-ctx) 1 0)
       (if (pixils.ui.slider/slider-windows-style? {:thumb-shape :rect} bevel-ctx) 1 0)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 4u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 1);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 0);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 1);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 0);
}

TEST_F(SliderTest, slider_render_helpers_read_effective_style)
{
  auto result = runtime.eval(R"(
    (let [ctx {:view {:style {:background {:r 255 :g 255 :b 255}
                              :border {:line-style :solid
                                       :color {:r 0 :g 0 :b 0}}}
                       :effective-style {:background {:r 61 :g 74 :b 94}
                                         :border {:line-style :bevel
                                                  :color {:r 103 :g 120 :b 143}
                                                  :right {:color {:r 45 :g 54 :b 70}}
                                                  :bottom {:color {:r 45 :g 54 :b 70}}}}}}
          handle (pixils.ui.slider/slider-handle-color {} ctx)
          top (pixils.ui.slider/slider-border-top-color {} ctx)
          bottom (pixils.ui.slider/slider-border-bottom-color {} ctx)]
      [(:r handle)
       (:g handle)
       (:b handle)
       (:r top)
       (:r bottom)
       (if (pixils.ui.slider/slider-bevel? ctx) 1 0)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 6u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 61);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 74);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 94);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 103);
  EXPECT_EQ(Roo::get_child(*result, 4)->num().get_int(), 45);
  EXPECT_EQ(Roo::get_child(*result, 5)->num().get_int(), 1);
}

TEST_F(SliderTest, classic_blue_theme_styles_slider_handle)
{
  auto slider = render_slider_for_theme("pixils/classic-blue");
  ASSERT_NE(slider, nullptr);
  ASSERT_TRUE(slider->effective_style.background.has_value());
  ASSERT_TRUE(slider->effective_style.background->color.has_value());
  ASSERT_TRUE(slider->effective_style.border.has_value());

  EXPECT_EQ(*slider->effective_style.background->color,
            (Pixils::Color{0x3d, 0x4a, 0x5e, 255}));
  EXPECT_EQ(slider->effective_style.border->top_color(),
            (Pixils::Color{0x67, 0x78, 0x8f, 255}));
  EXPECT_EQ(slider->effective_style.border->bottom_color(),
            (Pixils::Color{0x2d, 0x36, 0x46, 255}));
}

TEST_F(SliderTest, windows_95_theme_styles_slider_handle)
{
  auto slider = render_slider_for_theme("pixils/windows-95");
  ASSERT_NE(slider, nullptr);
  ASSERT_TRUE(slider->effective_style.background.has_value());
  ASSERT_TRUE(slider->effective_style.background->color.has_value());
  ASSERT_TRUE(slider->effective_style.border.has_value());

  EXPECT_EQ(*slider->effective_style.background->color,
            (Pixils::Color{0xb8, 0xb8, 0xb8, 255}));
  EXPECT_EQ(slider->effective_style.border->line_style,
            Pixils::UI::Style::LineStyle::BEVEL);
  EXPECT_EQ(slider->effective_style.border->top_color(),
            (Pixils::Color{0xdf, 0xdf, 0xdf, 255}));
  EXPECT_EQ(slider->effective_style.border->bottom_color(),
            (Pixils::Color{0x7f, 0x7f, 0x7f, 255}));
}

TEST_F(SliderTest, windows_3_theme_styles_slider_handle)
{
  auto slider = render_slider_for_theme("pixils/windows-3");
  ASSERT_NE(slider, nullptr);
  ASSERT_TRUE(slider->effective_style.background.has_value());
  ASSERT_TRUE(slider->effective_style.background->color.has_value());
  ASSERT_TRUE(slider->effective_style.border.has_value());

  EXPECT_EQ(*slider->effective_style.background->color,
            (Pixils::Color{0xc0, 0xc7, 0xc8, 255}));
  EXPECT_EQ(slider->effective_style.border->line_style,
            Pixils::UI::Style::LineStyle::BEVEL);
  EXPECT_EQ(slider->effective_style.border->top_color(),
            (Pixils::Color{255, 255, 255, 255}));
  EXPECT_EQ(slider->effective_style.border->bottom_color(),
            (Pixils::Color{0x87, 0x88, 0x8f, 255}));
}
