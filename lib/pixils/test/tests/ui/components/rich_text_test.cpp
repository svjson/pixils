#include "../../render_fixture.h"

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
}
