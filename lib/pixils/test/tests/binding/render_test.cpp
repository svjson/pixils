
#include "../render_fixture.h"
#include <pixils/font_registry.h>
#include <pixils/text.h>

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>
#include <vector>

class RenderTest : public RenderFixture
{
};

namespace
{
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
    }
    return false;
  }
} // namespace

TEST_F(RenderTest, rect_accepts_map_style_points)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  {:x 5 :y 10}
                  {:x 25 :y 30}
                  {:fill true}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].rendered_rect.x, 5);
  EXPECT_EQ(ops[0].rendered_rect.y, 10);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);
}

TEST_F(RenderTest, rect_accepts_inline_color_map_in_options)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  {:x 0 :y 0}
                  {:x 10 :y 10}
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_EQ(render_target()->render_ops.size(), 1u);
}

TEST_F(RenderTest, with_clip_rect_restores_previous_clip)
{
  // Given
  render_ctx.set_clip_rect(Pixils::Rect{1, 2, 30, 40});

  // When
  ASSERT_NO_THROW(runtime.eval(R"(
    (pixils.render/with-clip-rect {:x 4 :y 5 :w 6 :h 7}
      (pixils.render/rect! {:x 0 :y 0 :w 20 :h 20} {:fill true}))
  )"));

  // Then
  ASSERT_TRUE(render_ctx.current_clip_rect.has_value());
  EXPECT_EQ(*render_ctx.current_clip_rect, (Pixils::Rect{1, 2, 30, 40}));
}

TEST_F(RenderTest, with_clip_rect_restores_clip_after_body_error)
{
  // Given
  render_ctx.set_clip_rect(Pixils::Rect{1, 2, 30, 40});

  // When
  EXPECT_THROW(runtime.eval(R"(
    (pixils.render/with-clip-rect {:x 4 :y 5 :w 6 :h 7}
      (/ 1 0))
  )"),
               Lisple::LispleException);

  // Then
  ASSERT_TRUE(render_ctx.current_clip_rect.has_value());
  EXPECT_EQ(*render_ctx.current_clip_rect, (Pixils::Rect{1, 2, 30, 40}));
}

TEST_F(RenderTest, image_accepts_rotation_in_radians)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :rotation 1.5707963}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When
  ASSERT_NO_THROW(session.render_mode());

  // Then
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
  EXPECT_NEAR(ops[0].rotation_degrees, 90.0, 0.01);
}

TEST_F(RenderTest, image_accepts_source_rect)
{
  SDLMock::prepared_surfaces["./tiles.png"] = {32, 16};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:tiles "tiles.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/tiles
                  {:pos {:x 3 :y 4}
                   :source {:x 16 :y 0 :w 8 :h 8}
                   :scale 2}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 3);
  EXPECT_EQ(ops[0].rendered_rect.y, 4);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 16);
}

TEST_F(RenderTest, image_accepts_flip_options)
{
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :flip-x? true
                   :flip-y? true}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
}

TEST_F(RenderTest, image_accepts_opacity)
{
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :opacity 0.5}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
}

TEST_F(RenderTest, image_missing_asset_is_noop)
{
  runtime.eval(R"(
    (pixils/defbundle-dynamic project-assets)
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :project-assets/missing
                  {:pos {:x 12 :y 18}}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_TRUE(render_target()->render_ops.empty());
}

TEST_F(RenderTest, style_background_image_renders_once_without_repeat)
{
  SDLMock::prepared_surfaces["./checkmark.png"] = {7, 7};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:checkmark "checkmark.png"}})
    (pixils/defmode test-mode
      {:style {:background {:image :icons/checkmark}}})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 7);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
}

TEST_F(RenderTest, style_background_image_accepts_opacity)
{
  SDLMock::prepared_surfaces["./checkmark.png"] = {7, 7};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:checkmark "checkmark.png"}})
    (pixils/defmode test-mode
      {:style {:background {:image :icons/checkmark
                            :opacity 0.5}}})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.w, 7);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
}

TEST_F(RenderTest, style_background_image_can_fit_source_and_align)
{
  SDLMock::prepared_surfaces["./icons.png"] = {32, 32};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:sheet "icons.png"}})
    (pixils/defmode test-mode
      {:style {:width 20
               :height 20
               :background {:image :icons/sheet
                            :source {:x 8 :y 4 :w 8 :h 4}
                            :fit :contain
                            :align :center}}})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 5);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 10);
}

TEST_F(RenderTest, style_corner_radius_rounds_background_fill)
{
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:style {:width 10
               :height 6
               :background {:r 200 :g 0 :b 0}
               :corner-radius 3}})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 8);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 2, 10, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{0, 0, 10, 1}));
}

TEST_F(RenderTest, style_directional_corner_radius_only_rounds_selected_corners)
{
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:style {:width 10
               :height 6
               :background {:r 200 :g 0 :b 0}
               :corner-radius {:tl 3 :tr 0 :br 3 :bl 0}}})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].rendered_rect.w, 9);
  EXPECT_EQ(ops.back().rendered_rect.x, 0);
  EXPECT_EQ(ops.back().rendered_rect.w, 9);
}

TEST_F(RenderTest, style_corner_radius_rounds_border)
{
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:style {:width 10
               :height 6
               :corner-radius 3
               :border {:thickness 1
                        :line-style :solid
                        :color {:r 0 :g 0 :b 0}}}})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_FALSE(ops.empty());
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 8);
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{0, 0, 10, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 2, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{9, 2, 1, 1}));
}

TEST_F(RenderTest, absolute_positioned_children_render_after_flow_siblings)
{
  runtime.eval(R"(
    (pixils/defmode box {})
    (pixils/defmode test-mode
      {:children [{:mode 'box
                   :style {:position :absolute
                           :width 10
                           :height 10
                           :background {:r 200 :g 0 :b 0}}}
                  {:mode 'box
                   :style {:width 20
                           :height 20
                           :background {:r 0 :g 200 :b 0}}}]})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[1].rendered_rect.w, 10);
  EXPECT_EQ(ops[1].rendered_rect.h, 10);
}

TEST_F(RenderTest, scaled_view_renders_to_logical_texture_and_copies_to_scaled_footprint)
{
  runtime.eval(R"(
    (pixils/defmode scaled-panel
      {:style {:width 20 :height 10 :scale 2}
       :render (fn [state ctx]
                 (pixils.render/rect!
                   {:x 0 :y 0}
                   {:x 20 :y 10}
                   {:fill true}))})
  )");
  session.push_mode("scaled-panel", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 40);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);

  ASSERT_EQ(ops[0].sub_ops.size(), 1u);
  EXPECT_EQ(ops[0].sub_ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.h, 10);
}

TEST_F(RenderTest, translucent_view_renders_to_texture_and_copies_whole_subtree)
{
  runtime.eval(R"(
    (pixils/defmode translucent-panel
      {:style {:width 20 :height 10 :opacity 0.5}
       :render (fn [state ctx]
                 (pixils.render/rect!
                   {:x 0 :y 0}
                   {:x 20 :y 10}
                   {:fill true}))})
  )");
  session.push_mode("translucent-panel", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 10);

  ASSERT_EQ(ops[0].sub_ops.size(), 1u);
  EXPECT_EQ(ops[0].sub_ops[0].type, RenderOpType::FILL_RECT);
}

TEST_F(RenderTest, text_without_explicit_color_uses_original_font_texture)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 8 :h 8}}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/text! "A" {:x 12 :y 18} {:font :font/test-font}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  SDL_Texture* original_texture = render_ctx.asset_registry->get_image("fonts", "atlas");
  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");

  ASSERT_NE(original_texture, nullptr);
  ASSERT_NE(tint_texture, nullptr);
  EXPECT_NE(original_texture, tint_texture);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, original_texture);
}

TEST_F(RenderTest, text_with_explicit_color_uses_tint_mask_texture)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 8 :h 8}}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/text!
                  "A"
                  {:x 12 :y 18}
                  {:font :font/test-font
                   :color {:r 200 :g 40 :b 60}}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  SDL_Texture* original_texture = render_ctx.asset_registry->get_image("fonts", "atlas");
  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");

  ASSERT_NE(original_texture, nullptr);
  ASSERT_NE(tint_texture, nullptr);
  EXPECT_NE(original_texture, tint_texture);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, tint_texture);
}

TEST_F(RenderTest, text_accepts_vector_scale_as_x_y)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text!
                   "A"
                   {:x 12 :y 18}
                   {:font :font/test-font
                    :scale [2 1]}))})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 8);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
}

TEST_F(RenderTest, text_marked_style_accepts_vector_scale_as_x_y)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont base-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/deffont marked-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text!
                   "A@B@"
                   {:x 12 :y 18}
                   {:font :font/base-font
                    :marked-style {:enabled true
                                   :marker "@"
                                   :font :font/marked-font
                                   :scale [2 1]}}))})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.w, 4);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.x, 17);
  EXPECT_EQ(ops[1].rendered_rect.w, 8);
  EXPECT_EQ(ops[1].rendered_rect.h, 7);
}

TEST_F(RenderTest, text_bang_respects_explicit_newlines)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/text! "AA
B" {:x 12 :y 18} {:font :font/test-font}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.y, 18);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[2].rendered_rect.y, 25);
}

TEST_F(RenderTest, text_size_uses_explicit_font_line_height)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :line-height 10
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "A" {:font :font/test-font}))");
  auto w = Lisple::Dict::get_property(result, Lisple::keyword("w"));
  auto h = Lisple::Dict::get_property(result, Lisple::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 5);
  EXPECT_EQ(h->num().get_int(), 10);
}

TEST_F(RenderTest, text_size_respects_explicit_newlines)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "AA
B" {:font :font/test-font}))");
  auto w = Lisple::Dict::get_property(result, Lisple::keyword("w"));
  auto h = Lisple::Dict::get_property(result, Lisple::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 10);
  EXPECT_EQ(h->num().get_int(), 14);
}

TEST_F(RenderTest, text_size_accepts_vector_scale_as_x_y)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
  )");

  auto result =
    runtime.eval(R"((pixils.render/text-size "AA" {:font :font/test-font :scale [2 1]}))");
  auto w = Lisple::Dict::get_property(result, Lisple::keyword("w"));
  auto h = Lisple::Dict::get_property(result, Lisple::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 20);
  EXPECT_EQ(h->num().get_int(), 7);
}

TEST_F(RenderTest, deffont_replaces_existing_font_definition)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 7 :h 4}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "A" {:font :font/test-font}))");
  auto w = Lisple::Dict::get_property(result, Lisple::keyword("w"));

  ASSERT_TRUE(w);
  EXPECT_EQ(w->num().get_int(), 8);
}

TEST_F(RenderTest, text_size_ignores_inline_toggle_markers)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
  )");

  auto result = runtime.eval("(pixils.render/text-size \"A@A@\" {:font :font/test-font "
                             ":marked-style {:enabled true :marker \"@\"}})");

  auto w = Lisple::Dict::get_property(result, Lisple::keyword("w"));
  auto h = Lisple::Dict::get_property(result, Lisple::keyword("h"));
  ASSERT_NE(w, nullptr);
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(w->num().get_int(), 8);
  EXPECT_EQ(h->num().get_int(), 7);
}

TEST_F(RenderTest, text_cursor_treats_at_markers_as_console_style_toggles)
{
  Pixils::Text::FontMap font_map(
    {{'A', SDL_Rect{0, 0, 4, 7}}, {'B', SDL_Rect{4, 0, 4, 7}}, {'@', SDL_Rect{8, 0, 4, 7}}});
  Pixils::Text::Renderer renderer(render_ctx.buffer_texture, font_map, 1, 1);
  Pixils::Text::Cursor cursor(renderer,
                              SDL_Color{0xff, 0xff, 0xff, 0xff},
                              SDL_Color{0x2b, 0x83, 0x14, 0xff});

  cursor.print(render_ctx, "@A@B");

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[1].rendered_rect.x, 5);

  auto rect = cursor.get_rendered_rect(render_ctx, "@A@B");
  EXPECT_EQ(rect.w, 10);
}

TEST_F(RenderTest, text_cursor_escapes_double_at_as_literal_at_glyph)
{
  Pixils::Text::FontMap font_map({{'@', SDL_Rect{0, 0, 4, 7}}});
  Pixils::Text::Renderer renderer(render_ctx.buffer_texture, font_map, 1, 1);
  Pixils::Text::Cursor cursor(renderer,
                              SDL_Color{0xff, 0xff, 0xff, 0xff},
                              SDL_Color{0x2b, 0x83, 0x14, 0xff});

  cursor.print(render_ctx, "@@");

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
}

TEST_F(RenderTest, deffont_registers_baseline_and_underline_metrics)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 1 :thickness 2}}
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
  )");

  auto* font = render_ctx.font_registry->get_font("font/test-font");
  ASSERT_NE(font, nullptr);
  EXPECT_EQ(font->definition.baseline, 5);
  ASSERT_TRUE(font->definition.underline.has_value());
  EXPECT_EQ(font->definition.underline->offset, 1);
  EXPECT_EQ(font->definition.underline->thickness, 2);
}

TEST_F(RenderTest, defbundle_declares_font_resources_for_ttf_fonts)
{
  runtime.eval(R"(
    (pixils/defbundle fonts {:fonts {:autoega "assets/Ac437_STB_AutoEGA_8x14.ttf"}})
  )");

  auto font_path = render_ctx.asset_registry->get_font_path("fonts", "autoega");

  ASSERT_TRUE(font_path.has_value());
  EXPECT_EQ(*font_path, "./assets/Ac437_STB_AutoEGA_8x14.ttf");
}

TEST_F(RenderTest, deffont_ttf_reports_load_failure)
{
  runtime.eval(R"(
    (pixils/defbundle fonts {:fonts {:missing "assets/missing.ttf"}})
  )");

  EXPECT_THROW(runtime.eval(R"(
    (pixils/deffont missing-font
      {:type :ttf
       :resource :fonts/missing
       :size 14})
  )"),
               Lisple::InvocationException);
}

TEST_F(RenderTest, text_with_underline_font_style_renders_fill_rect_for_underline)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 1 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text! "A"
                                      {:x 10 :y 12}
                                      {:font :font/test-font
                                       :color {:r 255 :g 255 :b 255}
                                       :font-styles :underline}))})
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[1].rendered_rect.x, 10);
  EXPECT_EQ(ops[1].rendered_rect.y, 18);
  EXPECT_EQ(ops[1].rendered_rect.w, 4);
  EXPECT_EQ(ops[1].rendered_rect.h, 1);
}

TEST_F(RenderTest, text_size_infers_font_line_height_from_tallest_glyph)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "A" {:font :font/test-font}))");
  auto w = Lisple::Dict::get_property(result, Lisple::keyword("w"));
  auto h = Lisple::Dict::get_property(result, Lisple::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 5);
  EXPECT_EQ(h->num().get_int(), 7);
}

TEST_F(RenderTest, built_in_text_node_renders_and_measures_without_definition)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/test-font}}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);

  auto& child = session.active_mode->children.at(0);
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->mode->name, "ui/text");

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 5);
  EXPECT_EQ(child->bounds.h, 7);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
}

TEST_F(RenderTest, built_in_text_node_falls_back_to_console_font_when_style_font_is_missing)
{
  SDLMock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont console
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 8 :h 8}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/missing}}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
}

TEST_F(RenderTest, built_in_text_node_renders_in_local_viewport_coordinates)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent padded-box
      {:style {:padding [5 7]}
       :children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/test-font}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'padded-box}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
}

TEST_F(RenderTest, built_in_text_node_respects_explicit_newlines)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA
B"}
                   :style {:text {:font :font/test-font}}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);

  auto& child = session.active_mode->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 10);
  EXPECT_EQ(child->bounds.h, 14);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.y, 0);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[2].rendered_rect.y, 7);
}

TEST_F(RenderTest, built_in_text_node_inherits_marked_style_and_renders_underline)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 2 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent menu-like-item
      {:style {:text {:font :font/test-font
                      :color {:r 0 :g 0 :b 0}
                      :marked-style {:enabled true
                                     :marker "@"
                                     :font-styles :underline}}}
       :children [{:mode 'ui/text
                   :state {:value "@A@"}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'menu-like-item}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
}

TEST_F(RenderTest,
       built_in_text_node_marked_style_explicit_color_uses_tint_texture_for_mnemonic)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 2 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent menu-like-item
      {:style {:text {:font :font/test-font
                      :color {:r 0 :g 0 :b 0}
                      :marked-style {:enabled true
                                     :marker "@"
                                     :font-styles :underline
                                     :color {:r 255 :g 255 :b 255}}}}
       :children [{:mode 'ui/text
                   :state {:value "@A@B"}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'menu-like-item}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NO_THROW(session.render_mode());

  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");
  ASSERT_NE(tint_texture, nullptr);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, tint_texture);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
}

TEST_F(RenderTest, built_in_text_node_marked_style_inherits_parent_tint_for_mnemonic)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 2 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent menu-like-item
      {:style {:text {:font :font/test-font
                      :color {:r 255 :g 255 :b 255}
                      :marked-style {:enabled true
                                     :marker "@"
                                     :font-styles :underline}}}
       :children [{:mode 'ui/text
                   :state {:value "@A@B"}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'menu-like-item}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NO_THROW(session.render_mode());

  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");
  ASSERT_NE(tint_texture, nullptr);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, tint_texture);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
}

TEST_F(RenderTest, built_in_text_node_wraps_wordwise_when_fill_width_is_constrained)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/defcomponent wrap-box
      {:style {:width 12}
       :children [{:mode 'ui/text
                   :state {:value "AA AA AA"}
                   :style {:width :fill
                           :text {:font :font/test-font}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'wrap-box}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  auto& child = session.active_mode->children.at(0)->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 12);
  EXPECT_EQ(child->bounds.h, 21);
}

TEST_F(RenderTest, built_in_text_node_wraps_wordwise_when_parent_width_is_constrained)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/defcomponent wrap-box
      {:style {:width 12}
       :children [{:mode 'ui/text
                   :state {:value "AA AA AA"}
                   :style {:text {:font :font/test-font}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'wrap-box}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  auto& child = session.active_mode->children.at(0)->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 10);
  EXPECT_EQ(child->bounds.h, 21);
}

TEST_F(RenderTest, built_in_text_node_wrap_none_stays_single_line_when_width_is_constrained)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/defcomponent wrap-box
      {:style {:width 12}
       :children [{:mode 'ui/text
                   :state {:value "AA AA AA"}
                   :style {:width :fill
                           :text {:font :font/test-font
                                  :wrap :none}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'wrap-box}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  auto& child = session.active_mode->children.at(0)->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 12);
  EXPECT_EQ(child->bounds.h, 7);
}

TEST_F(RenderTest, built_in_text_node_wrap_preserves_leading_spaces)
{
  SDLMock::prepared_surfaces["./font.png"] = {24, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"O" {:x 0 :y 0 :w 4 :h 7}
                "K" {:x 4 :y 0 :w 4 :h 7}
                " " {:x 8 :y 0 :w 1 :h 7}}})
  )");

  auto text_op = Pixils::Text::make_text_render_op(render_ctx, "font/test-font", 1);
  ASSERT_TRUE(text_op.has_value());

  auto layout = Pixils::Text::layout_text(render_ctx,
                                          *text_op,
                                          "  OK  ",
                                          Pixils::Text::WrapMode::WORD,
                                          24);
  ASSERT_EQ(layout.lines.size(), 1u);
  EXPECT_EQ(layout.lines[0].text, "  OK  ");
  EXPECT_EQ(layout.size.w, 18);
  EXPECT_EQ(layout.size.h, 7);
}
