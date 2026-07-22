#include "../../render_fixture.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/rect_namespace.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <sdl3_mock/mock_resources.h>

using RichTextTest = RenderFixture;

namespace
{
  Roo::sptr_val property(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }
} // namespace

TEST_F(RichTextTest, rich_text_emits_click_payload_for_interactive_run)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 8}
                "B" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:on {:rich-text/click
            (fn [state event ctx]
              (assoc state :clicked (:payload event)))}
       :children [{:mode 'ui/rich-text
                   :style {:width 80
                           :height :shrink
                           :text {:font :font/test-font
                                  :marked-style {:font-styles :underline}}}
                   :state {:runs ["AA" {:text "BB"
                                         :value :subject/bb
                                         :marked? true}]}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto rich_text = session.active_mode->children[0];
  ASSERT_NE(rich_text, nullptr);
  EXPECT_EQ(rich_text->mode->name, "ui/rich-text");
  EXPECT_GT(rich_text->bounds.w, 0);
  EXPECT_GT(rich_text->bounds.h, 0);

  input().mouse_down({rich_text->bounds.x + 12, rich_text->bounds.y + 4});
  update_cycle();
  input().mouse_up({rich_text->bounds.x + 12, rich_text->bounds.y + 4});
  update_cycle();

  auto clicked = property(session.active_mode->state, "clicked");
  ASSERT_NE(clicked, nullptr);
  EXPECT_EQ(property(clicked, "index")->num().get_int(), 1);
  EXPECT_EQ(property(clicked, "text")->str(), "BB");
  EXPECT_EQ(property(clicked, "value")->to_string(), ":subject/bb");
  auto click_anchor = property(clicked, "anchor");
  ASSERT_NE(click_anchor, nullptr);
  EXPECT_TRUE(Pixils::Script::HostType::VIEW.is_type_of(*click_anchor));
  auto click_anchor_bounds = property(clicked, "anchor-bounds");
  ASSERT_NE(click_anchor_bounds, nullptr);
  EXPECT_TRUE(Pixils::Script::HostType::RECT.is_type_of(*click_anchor_bounds));
  const auto& click_rect = Roo::obj<Pixils::Rect>(*click_anchor_bounds);
  EXPECT_EQ(click_rect.x, 10);
  EXPECT_EQ(click_rect.y, 0);
  EXPECT_EQ(click_rect.w, 10);
  EXPECT_EQ(click_rect.h, 8);
}

TEST_F(RichTextTest, rich_text_emits_hover_payload_for_interactive_run)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 8}
                "B" {:x 4 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:on {:rich-text/hover
            (fn [state event ctx]
              (assoc state :hovered (:payload event)))}
       :children [{:mode 'ui/rich-text
                   :style {:width 80
                           :height :shrink
                           :text {:font :font/test-font}}
                   :state {:runs [{:text "AA"
                                   :value :subject/aa}
                                  "BB"]}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto rich_text = session.active_mode->children[0];
  input().mouse_move({rich_text->bounds.x + 2, rich_text->bounds.y + 4});
  update_cycle();

  auto hovered = property(session.active_mode->state, "hovered");
  ASSERT_NE(hovered, nullptr);
  EXPECT_EQ(property(hovered, "index")->num().get_int(), 0);
  EXPECT_EQ(property(hovered, "text")->str(), "AA");
  EXPECT_EQ(property(hovered, "value")->to_string(), ":subject/aa");
  auto hover_anchor = property(hovered, "anchor");
  ASSERT_NE(hover_anchor, nullptr);
  EXPECT_TRUE(Pixils::Script::HostType::VIEW.is_type_of(*hover_anchor));
  auto hover_anchor_bounds = property(hovered, "anchor-bounds");
  ASSERT_NE(hover_anchor_bounds, nullptr);
  EXPECT_TRUE(Pixils::Script::HostType::RECT.is_type_of(*hover_anchor_bounds));
  const auto& hover_rect = Roo::obj<Pixils::Rect>(*hover_anchor_bounds);
  EXPECT_EQ(hover_rect.x, 0);
  EXPECT_EQ(hover_rect.y, 0);
  EXPECT_EQ(hover_rect.w, 10);
  EXPECT_EQ(hover_rect.h, 8);
}

TEST_F(RichTextTest, rich_text_anchor_bounds_cover_run_segment_on_line)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {20, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 8}
                "B" {:x 4 :y 0 :w 4 :h 8}
                " " {:x 8 :y 0 :w 4 :h 8}}})
    (pixils/defmode root-mode
      {:on {:rich-text/click
            (fn [state event ctx]
              (assoc state :clicked (:payload event)))}
       :children [{:mode 'ui/rich-text
                   :style {:width 120
                           :height :shrink
                           :text {:font :font/test-font}}
                   :state {:runs [{:text "AA BB"
                                   :value :subject/run}]}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  auto rich_text = session.active_mode->children[0];
  input().mouse_down({rich_text->bounds.x + 2, rich_text->bounds.y + 4});
  update_cycle();
  input().mouse_up({rich_text->bounds.x + 2, rich_text->bounds.y + 4});
  update_cycle();

  auto first_clicked = property(session.active_mode->state, "clicked");
  ASSERT_NE(first_clicked, nullptr);
  auto first_anchor_bounds = property(first_clicked, "anchor-bounds");
  ASSERT_NE(first_anchor_bounds, nullptr);
  const auto first_rect = Roo::obj<Pixils::Rect>(*first_anchor_bounds);

  input().mouse_down({rich_text->bounds.x + 18, rich_text->bounds.y + 4});
  update_cycle();
  input().mouse_up({rich_text->bounds.x + 18, rich_text->bounds.y + 4});
  update_cycle();

  auto second_clicked = property(session.active_mode->state, "clicked");
  ASSERT_NE(second_clicked, nullptr);
  auto second_anchor_bounds = property(second_clicked, "anchor-bounds");
  ASSERT_NE(second_anchor_bounds, nullptr);
  const auto& second_rect = Roo::obj<Pixils::Rect>(*second_anchor_bounds);

  EXPECT_EQ(second_rect.x, first_rect.x);
  EXPECT_EQ(second_rect.y, first_rect.y);
  EXPECT_EQ(second_rect.w, first_rect.w);
  EXPECT_EQ(second_rect.h, first_rect.h);
  EXPECT_GT(first_rect.w, 10);
}

TEST_F(RichTextTest, rich_text_run_font_styles_render_bold_text)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/rich-text
                   :style {:width 80
                           :height :shrink
                           :text {:font :font/test-font}}
                   :state {:runs [{:text "A"
                                   :style {:font-styles :bold}}]}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  const auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.x, ops[0].rendered_rect.x + 1);
  EXPECT_EQ(ops[1].rendered_rect.y, ops[0].rendered_rect.y);
}
