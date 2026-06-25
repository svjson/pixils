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
                       {:axis :x :value 5 :min 0 :max 10
                        :track-layout :handle-travel
                        :track-thickness 4}
                       horizontal-ctx)
          vertical (pixils.ui.slider/slider-track-rect
                     {:axis :y :value 5 :min 0 :max 10
                      :track-layout :handle-travel
                      :track-thickness 4}
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

TEST_F(SliderTest, slider_thumb_length_controls_axis_dimension)
{
  auto result = runtime.eval(R"(
    (let [horizontal-ctx {:view {:bounds {:x 0 :y 0 :w 100 :h 21}}}
          vertical-ctx {:view {:bounds {:x 0 :y 0 :w 21 :h 100}}}
          state {:value 5 :min 0 :max 10
                 :track-layout :handle-travel
                 :track-thickness 4
                 :thumb-length 13
                 :thumb-point-size 6}
          horizontal-track (pixils.ui.slider/slider-track-rect
                             (assoc state :axis :x)
                             horizontal-ctx)
          horizontal-thumb (pixils.ui.slider/slider-handle-rect
                             (assoc state :axis :x)
                             horizontal-ctx)
          vertical-track (pixils.ui.slider/slider-track-rect
                           (assoc state :axis :y)
                           vertical-ctx)
          vertical-thumb (pixils.ui.slider/slider-handle-rect
                           (assoc state :axis :y)
                           vertical-ctx)]
      [(:x horizontal-track) (:w horizontal-track)
       (:w horizontal-thumb) (:h horizontal-thumb)
       (:y vertical-track) (:h vertical-track)
       (:w vertical-thumb) (:h vertical-thumb)
       (pixils.ui.slider/slider-thumb-point-size state horizontal-ctx)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 9u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 6);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 87);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 13);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 21);
  EXPECT_EQ(Roo::get_child(*result, 4)->num().get_int(), 6);
  EXPECT_EQ(Roo::get_child(*result, 5)->num().get_int(), 87);
  EXPECT_EQ(Roo::get_child(*result, 6)->num().get_int(), 21);
  EXPECT_EQ(Roo::get_child(*result, 7)->num().get_int(), 13);
  EXPECT_EQ(Roo::get_child(*result, 8)->num().get_int(), 6);
}

TEST_F(SliderTest, slider_track_rect_uses_content_box_inside_outer_border)
{
  auto result = runtime.eval(R"(
    (let [horizontal-ctx {:view {:bounds {:x 0 :y 0 :w 100 :h 21}
                                 :effective-style {:border {:thickness 2}}}}
          vertical-ctx {:view {:bounds {:x 0 :y 0 :w 21 :h 100}
                               :effective-style {:border {:thickness 2}}}}
          horizontal (pixils.ui.slider/slider-track-rect
                       {:axis :x :value 5 :min 0 :max 10
                        :track-layout :handle-travel
                        :track-thickness 4}
                       horizontal-ctx)
          vertical (pixils.ui.slider/slider-track-rect
                     {:axis :y :value 5 :min 0 :max 10
                      :track-layout :handle-travel
                      :track-thickness 4}
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

TEST_F(SliderTest, slider_primitives_use_theme_or_state_override)
{
  auto result = runtime.eval(R"(
    (let [default-ctx {:view {:effective-style {:border {:line-style :bevel}}}}
          theme-ctx {:slider-theme {:thumb-shape :windows
                                    :thumb-color {:r 70 :g 0 :b 0}
                                    :thumb-length 13
                                    :thumb-point-size 6
                                    :track-layout :handle-travel
                                    :track-shape :windows-track
                                    :track-border? true
                                    :track-border-top-color {:r 10 :g 0 :b 0}
                                    :track-border-bottom-color {:r 20 :g 0 :b 0}
                                    :track-bevel {:outer-top-color {:r 11 :g 0 :b 0}
                                                  :outer-bottom-color {:r 22 :g 0 :b 0}
                                                  :inner-top-color {:r 33 :g 0 :b 0}
                                                  :inner-bottom-color {:r 44 :g 0 :b 0}}
                                    :thumb-bevel {:outer-top-color {:r 30 :g 0 :b 0}
                                                  :outer-bottom-color {:r 40 :g 0 :b 0}
                                                  :inner-top-color {:r 50 :g 0 :b 0}
                                                  :inner-bottom-color {:r 60 :g 0 :b 0}}}
                     :view {:bounds {:x 0 :y 0 :w 100 :h 21}}}]
      [(if (pixils.ui.slider/slider-windows-style? {} default-ctx) 1 0)
       (if (pixils.ui.slider/slider-windows-style? {} theme-ctx) 1 0)
       (if (pixils.ui.slider/slider-windows-style? {:thumb-shape :rect} theme-ctx) 1 0)
       (if (= (pixils.ui.slider/slider-track-layout {} theme-ctx) :handle-travel) 1 0)
       (if (pixils.ui.slider/slider-track-border? {} theme-ctx) 1 0)
       (pixils.ui.slider/slider-track-thickness {:track-thickness 5} theme-ctx)
       (if (= (pixils.ui.slider/slider-track-shape {} theme-ctx) :windows-track) 1 0)
       (:r (pixils.ui.slider/slider-track-top-color {} theme-ctx))
       (:r (pixils.ui.slider/slider-track-bottom-color {} theme-ctx))
       (:r (:outer-top-color (pixils.ui.slider/slider-track-bevel {} theme-ctx)))
       (:r (:outer-bottom-color (pixils.ui.slider/slider-track-bevel {} theme-ctx)))
       (:r (:inner-top-color (pixils.ui.slider/slider-track-bevel {} theme-ctx)))
       (:r (:inner-bottom-color (pixils.ui.slider/slider-track-bevel {} theme-ctx)))
       (:r (:outer-top-color (pixils.ui.slider/slider-thumb-bevel {} theme-ctx)))
       (:r (:outer-bottom-color (pixils.ui.slider/slider-thumb-bevel {} theme-ctx)))
       (:r (:inner-top-color (pixils.ui.slider/slider-thumb-bevel {} theme-ctx)))
       (:r (:inner-bottom-color (pixils.ui.slider/slider-thumb-bevel {} theme-ctx)))
       (:r (pixils.ui.slider/slider-handle-color {} theme-ctx))
       (pixils.ui.slider/slider-handle-size {} theme-ctx)
       (pixils.ui.slider/slider-thumb-point-size {} theme-ctx)])
  )");

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(Roo::count(*result), 20u);
  EXPECT_EQ(Roo::get_child(*result, 0)->num().get_int(), 0);
  EXPECT_EQ(Roo::get_child(*result, 1)->num().get_int(), 1);
  EXPECT_EQ(Roo::get_child(*result, 2)->num().get_int(), 0);
  EXPECT_EQ(Roo::get_child(*result, 3)->num().get_int(), 1);
  EXPECT_EQ(Roo::get_child(*result, 4)->num().get_int(), 1);
  EXPECT_EQ(Roo::get_child(*result, 5)->num().get_int(), 5);
  EXPECT_EQ(Roo::get_child(*result, 6)->num().get_int(), 1);
  EXPECT_EQ(Roo::get_child(*result, 7)->num().get_int(), 10);
  EXPECT_EQ(Roo::get_child(*result, 8)->num().get_int(), 20);
  EXPECT_EQ(Roo::get_child(*result, 9)->num().get_int(), 11);
  EXPECT_EQ(Roo::get_child(*result, 10)->num().get_int(), 22);
  EXPECT_EQ(Roo::get_child(*result, 11)->num().get_int(), 33);
  EXPECT_EQ(Roo::get_child(*result, 12)->num().get_int(), 44);
  EXPECT_EQ(Roo::get_child(*result, 13)->num().get_int(), 30);
  EXPECT_EQ(Roo::get_child(*result, 14)->num().get_int(), 40);
  EXPECT_EQ(Roo::get_child(*result, 15)->num().get_int(), 50);
  EXPECT_EQ(Roo::get_child(*result, 16)->num().get_int(), 60);
  EXPECT_EQ(Roo::get_child(*result, 17)->num().get_int(), 70);
  EXPECT_EQ(Roo::get_child(*result, 18)->num().get_int(), 13);
  EXPECT_EQ(Roo::get_child(*result, 19)->num().get_int(), 6);
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
            (Pixils::Color{0, 0, 0, 0}));
  EXPECT_EQ(slider->effective_style.border->top_thickness(), 0);
  EXPECT_EQ(slider->effective_style.border->right_thickness(), 0);
  EXPECT_EQ(slider->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(slider->effective_style.border->left_thickness(), 0);
}

TEST_F(SliderTest, windows_95_theme_sets_orientation_cross_size)
{
  runtime.eval(R"(
    (pixils/defprogram slider-test-program
      {:theme 'pixils/windows-95
       :initial-mode 'root-mode})

    (pixils/defmode root-mode
      {:children [(pixils.ui.slider/make
                   {:style {:width 100}
                    :value 5
                    :min 0
                    :max 10
                    :step 1})
                  (pixils.ui.slider/make
                   {:style {:height 100}
                    :axis :y
                    :value 5
                    :min 0
                    :max 10
                    :step 1})]})
  )");

  Pixils::load_program(runtime, session);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);

  auto horizontal = session.active_mode->children[0];
  auto vertical = session.active_mode->children[1];
  ASSERT_TRUE(horizontal->effective_style.height.has_value());
  ASSERT_TRUE(vertical->effective_style.width.has_value());
  EXPECT_EQ(horizontal->effective_style.height->fixed_value_or(0), 21);
  EXPECT_EQ(vertical->effective_style.width->fixed_value_or(0), 21);
}

TEST_F(SliderTest, windows_95_dark_variant_uses_dark_slider_bevels)
{
  runtime.eval(R"(
    (pixils/defprogram slider-test-program
      {:theme 'pixils/windows-95
       :theme-variant :dark
       :initial-mode 'root-mode})

    (pixils/defmode root-mode
      {:children [(pixils.ui.slider/make
                   {:style {:width 100}
                    :value 5
                    :min 0
                    :max 10
                    :step 1})]})
  )");

  Pixils::load_program(runtime, session);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto slider = session.active_mode->children[0];
  ASSERT_TRUE(slider->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(slider->effective_theme.vars.at("dark").count("slider") > 0);

  auto slider_var = slider->effective_theme.vars.at("dark").at("slider");
  ASSERT_NE(slider_var, nullptr);
  EXPECT_EQ(slider_var->to_string(),
            "{:thumb-shape :windows :thumb-color {:__pixils-theme-var :panel-bg} "
            ":thumb-length 13 :thumb-point-size 6 :track-layout :handle-travel "
            ":track-shape :windows-track :track-thickness 4 :track-border? false "
            ":track-bevel {:outer-top-color {:__pixils-theme-var :panel-shadow} "
            ":outer-bottom-color {:__pixils-theme-var :panel-highlight} "
            ":inner-top-color {:__pixils-theme-var :border} "
            ":inner-bottom-color {:__pixils-theme-var :highlight}} "
            ":thumb-bevel {:outer-top-color {:__pixils-theme-var :highlight} "
            ":outer-bottom-color {:__pixils-theme-var :border} "
            ":inner-top-color {:__pixils-theme-var :panel-highlight} "
            ":inner-bottom-color {:__pixils-theme-var :panel-shadow}}}");
}

TEST_F(SliderTest, windows_3_theme_styles_slider_handle)
{
  auto slider = render_slider_for_theme("pixils/windows-3");
  ASSERT_NE(slider, nullptr);
  ASSERT_TRUE(slider->effective_style.background.has_value());
  ASSERT_TRUE(slider->effective_style.background->color.has_value());
  ASSERT_TRUE(slider->effective_style.border.has_value());

  EXPECT_EQ(*slider->effective_style.background->color,
            (Pixils::Color{0, 0, 0, 0}));
  EXPECT_EQ(slider->effective_style.border->top_thickness(), 0);
  EXPECT_EQ(slider->effective_style.border->right_thickness(), 0);
  EXPECT_EQ(slider->effective_style.border->bottom_thickness(), 0);
  EXPECT_EQ(slider->effective_style.border->left_thickness(), 0);
}

TEST_F(SliderTest, windows_3_dark_variant_uses_dark_slider_bevels)
{
  runtime.eval(R"(
    (pixils/defprogram slider-test-program
      {:theme 'pixils/windows-3
       :theme-variant :dark
       :initial-mode 'root-mode})

    (pixils/defmode root-mode
      {:children [(pixils.ui.slider/make
                   {:style {:width 100}
                    :value 5
                    :min 0
                    :max 10
                    :step 1})]})
  )");

  Pixils::load_program(runtime, session);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto slider = session.active_mode->children[0];
  ASSERT_TRUE(slider->effective_theme.vars.count("dark") > 0);
  ASSERT_TRUE(slider->effective_theme.vars.at("dark").count("slider") > 0);

  auto slider_var = slider->effective_theme.vars.at("dark").at("slider");
  ASSERT_NE(slider_var, nullptr);
  EXPECT_EQ(slider_var->to_string(),
            "{:thumb-shape :windows :thumb-color {:__pixils-theme-var :panel-bg} "
            ":thumb-length 13 :thumb-point-size 6 :track-layout :handle-travel "
            ":track-shape :windows-track :track-thickness 4 :track-border? false "
            ":track-bevel {:outer-top-color {:__pixils-theme-var :panel-shadow} "
            ":outer-bottom-color {:__pixils-theme-var :highlight} "
            ":inner-top-color {:__pixils-theme-var :control-border} "
            ":inner-bottom-color {:__pixils-theme-var :border}} "
            ":thumb-bevel {:outer-top-color {:__pixils-theme-var :highlight} "
            ":outer-bottom-color {:__pixils-theme-var :control-border} "
            ":inner-top-color {:__pixils-theme-var :border} "
            ":inner-bottom-color {:__pixils-theme-var :panel-shadow}}}");
}
