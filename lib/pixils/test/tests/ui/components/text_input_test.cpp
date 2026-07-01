#include "../../render_fixture.h"

#include <pixils/clipboard.h>
#include <pixils/program.h>
#include <pixils/ui/view_layout.h>

#include <SDL2/SDL_keycode.h>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <algorithm>
#include <set>

using TextInputTest = RenderFixture;

namespace
{
  void layout_active_mode(Roo::Runtime& runtime,
                          Pixils::Runtime::Session& session)
  {
    Pixils::UI::layout_view_tree(session.active_mode,
                                 {0, 0, session.render_ctx.buffer_dim.w,
                                  session.render_ctx.buffer_dim.h},
                                 runtime,
                                 session.hook_args.render_args[1]);
  }

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

  Roo::sptr_val get_keyword(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

  bool has_fill_rect(const std::vector<RenderOperation>& ops, const SDL_Rect& rect)
  {
    return std::any_of(ops.begin(), ops.end(), [&](const auto& op) {
      return op.type == RenderOpType::FILL_RECT && op.rendered_rect.x == rect.x &&
             op.rendered_rect.y == rect.y && op.rendered_rect.w == rect.w &&
             op.rendered_rect.h == rect.h;
    });
  }

  template <typename UpdateCycle>
  void press_ctrl_shortcut(InputSimulator& input,
                           UpdateCycle update_cycle,
                           SDL_Keycode key)
  {
    input.key_down(SDLK_LCTRL);
    update_cycle();
    input.key_down(key);
    update_cycle();
    input.key_up(key);
    update_cycle();
    input.key_up(SDLK_LCTRL);
    update_cycle();
  }

  class InMemoryClipboardBackend : public Pixils::Clipboard::Backend
  {
   public:
    std::string text;

    std::string get_text() override { return text; }
    bool has_text() override { return !text.empty(); }
    bool set_text(const std::string& next_text, std::string*) override
    {
      text = next_text;
      return true;
    }
  };

  class ScopedClipboardBackend
  {
   public:
    ScopedClipboardBackend() { Pixils::Clipboard::set_backend_for_testing(&backend); }
    ~ScopedClipboardBackend() { Pixils::Clipboard::set_backend_for_testing(nullptr); }

    InMemoryClipboardBackend backend;
  };
} // namespace

TEST_F(TextInputTest, text_input_edits_bound_value_and_emits_change)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "ab"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  input().key_down(SDLK_c);
  update_cycle();

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(text->to_string(), "\"abc\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"abc\"}");

  input().key_down(SDLK_BACKSPACE);
  update_cycle();

  text = get_keyword(session.active_mode->state, "text");
  last_change = get_keyword(session.active_mode->state, "last-change");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(text->to_string(), "\"ab\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"ab\"}");
}

TEST_F(TextInputTest, natural_height_uses_default_ttf_font_metrics)
{
  runtime.eval(R"(
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24
       :line-height 30})
    (pixils/deftheme large-text-theme
      {:defaults {:text {:font :font/large-font}}})
    (pixils/defmode root-mode
      {:theme ['pixils/windows-3 'large-text-theme]
       :children [{:mode 'ui/text-input
                   :state {:value "Alpha"}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto input_view = find_first_mode(session.active_mode, "ui/text-input");
  ASSERT_NE(input_view, nullptr);
  ASSERT_EQ(input_view->children.size(), 1u);
  auto inner = input_view->children[0];
  ASSERT_NE(inner, nullptr);

  EXPECT_GT(input_view->bounds.h, 22);
  EXPECT_EQ(inner->bounds.h, input_view->effective_style.content_rect(input_view->bounds).h);
}

TEST_F(TextInputTest, text_input_text_and_caret_are_positioned_on_first_render)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/text-input
                   :style {:width 260 :height 24}
                   :state {:value "Editable text"
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  layout_active_mode(runtime, session);

  auto text = find_first_mode(session.active_mode, "ui/text");
  auto caret = find_first_mode(session.active_mode, "ui/text-input-caret");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(caret, nullptr);

  EXPECT_EQ(caret->bounds.y, text->bounds.y);
  EXPECT_EQ(caret->bounds.h, text->bounds.h);
  EXPECT_GT(text->bounds.y, 0);
}

TEST_F(TextInputTest, read_only_text_input_focuses_and_navigates_without_mutating)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :read-only? true
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);
  ASSERT_TRUE(session.focus_state.has_focus());
  EXPECT_EQ(session.focus_state.focused.lock().get(), text_input_inner.get());

  input().key_down(SDLK_LEFT);
  update_cycle();

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  ASSERT_NE(cursor_index, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 3);

  input().key_down(SDLK_c);
  update_cycle();
  input().key_down(SDLK_BACKSPACE);
  update_cycle();
  input().key_down(SDLK_DELETE);
  update_cycle();

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  EXPECT_EQ(text->to_string(), "\"abcd\"");
  EXPECT_EQ(last_change->type, Roo::Value::Type::NIL);
  EXPECT_EQ(cursor_index->num().get_int(), 3);
}

TEST_F(TextInputTest, text_input_scrolls_horizontally_to_keep_caret_visible)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAAAAAAAAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 50
                           :max-width 50
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();
  update_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  auto scroll_x = get_keyword(text_input_inner->state, "scroll-x");
  auto caret_x = get_keyword(text_input_inner->state, "caret-x");
  ASSERT_NE(scroll_x, nullptr);
  ASSERT_NE(caret_x, nullptr);
  EXPECT_GT(scroll_x->num().get_int(), 0);
  EXPECT_LE(caret_x->num().get_int(), 40);

  input().key_down(SDLK_HOME);
  update_cycle();

  scroll_x = get_keyword(text_input_inner->state, "scroll-x");
  caret_x = get_keyword(text_input_inner->state, "caret-x");
  ASSERT_NE(scroll_x, nullptr);
  ASSERT_NE(caret_x, nullptr);
  EXPECT_EQ(scroll_x->num().get_int(), 0);
  EXPECT_EQ(caret_x->num().get_int(), 0);

  input().key_down(SDLK_END);
  update_cycle();

  scroll_x = get_keyword(text_input_inner->state, "scroll-x");
  ASSERT_NE(scroll_x, nullptr);
  EXPECT_GT(scroll_x->num().get_int(), 0);
}

TEST_F(TextInputTest, text_input_scrolls_one_pixel_when_text_exactly_fills_width)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAAAAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 50
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();
  update_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  auto scroll_x = get_keyword(text_input_inner->state, "scroll-x");
  auto caret_x = get_keyword(text_input_inner->state, "caret-x");
  ASSERT_NE(scroll_x, nullptr);
  ASSERT_NE(caret_x, nullptr);
  EXPECT_EQ(scroll_x->num().get_int(), 1);
  EXPECT_EQ(caret_x->num().get_int(), 39);
}

TEST_F(TextInputTest, text_input_shift_navigation_extends_and_plain_navigation_collapses_selection)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 3);
  EXPECT_EQ(selection_start->num().get_int(), 3);
  EXPECT_EQ(selection_end->num().get_int(), 4);

  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();

  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  selection_start = get_keyword(text_input_inner->state, "selection-start");
  selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 2);
  EXPECT_EQ(selection_start->num().get_int(), 2);
  EXPECT_EQ(selection_end->num().get_int(), 4);

  input().key_up(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_RIGHT);
  update_cycle();

  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  selection_start = get_keyword(text_input_inner->state, "selection-start");
  selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);
  EXPECT_EQ(selection_end->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_typing_replaces_selected_text)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_x);
  update_cycle();

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  EXPECT_EQ(text->to_string(), "\"abx\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"abx\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 3);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_backspace_deletes_selected_text)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_BACKSPACE);
  update_cycle();

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  EXPECT_EQ(text->to_string(), "\"ab\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"ab\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 2);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_shift_home_end_extend_selection_and_delete_removes_it)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_HOME);
  update_cycle();
  input().key_up(SDLK_HOME);
  update_cycle();

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 0);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);

  input().key_up(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_HOME);
  update_cycle();
  input().key_up(SDLK_HOME);
  update_cycle();
  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_END);
  update_cycle();
  input().key_up(SDLK_END);
  update_cycle();

  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  selection_start = get_keyword(text_input_inner->state, "selection-start");
  selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);

  input().key_up(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_DELETE);
  update_cycle();

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  selection_start = get_keyword(text_input_inner->state, "selection-start");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  EXPECT_EQ(text->to_string(), "\"\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 0);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_renders_selection_background_under_single_text_layer)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 80
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();
  input().key_down(SDLK_LEFT);
  update_cycle();
  input().key_up(SDLK_LEFT);
  update_cycle();
  render_cycle();

  auto selection_x = get_keyword(text_input_inner->state, "selection-x");
  auto selection_y = get_keyword(text_input_inner->state, "selection-y");
  auto selection_w = get_keyword(text_input_inner->state, "selection-w");
  auto selection_h = get_keyword(text_input_inner->state, "selection-h");
  ASSERT_NE(selection_x, nullptr);
  ASSERT_NE(selection_y, nullptr);
  ASSERT_NE(selection_w, nullptr);
  ASSERT_NE(selection_h, nullptr);
  EXPECT_GT(selection_w->num().get_int(), 0);
  auto selection_rect = SDL_Rect{selection_x->num().get_int(),
                                 selection_y->num().get_int(),
                                 selection_w->num().get_int(),
                                 selection_h->num().get_int()};
  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, selection_rect));

  auto selection_op = std::find_if(render_target()->render_ops.begin(),
                                   render_target()->render_ops.end(),
                                   [&](const auto& op) {
                                     return op.type == RenderOpType::FILL_RECT &&
                                            op.rendered_rect.x == selection_rect.x &&
                                            op.rendered_rect.y == selection_rect.y &&
                                            op.rendered_rect.w == selection_rect.w &&
                                            op.rendered_rect.h == selection_rect.h;
                                   });
  ASSERT_NE(selection_op, render_target()->render_ops.end());

  auto copy_ops_after_selection =
    std::count_if(selection_op,
                  render_target()->render_ops.end(),
                  [](const auto& op) {
                    return op.type == RenderOpType::RENDER_COPY;
                  });
  EXPECT_EQ(copy_ops_after_selection, 4);
}

TEST_F(TextInputTest, text_input_text_never_wraps)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "A A A A A A A A"})
       :children [{:mode 'ui/text-input
                   :style {:width 32
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  std::set<int> text_y_positions;
  for (const auto& op : render_target()->render_ops)
  {
    if (op.type == RenderOpType::RENDER_COPY)
    {
      text_y_positions.insert(op.rendered_rect.y);
    }
  }

  EXPECT_EQ(text_y_positions.size(), 1u);
}

TEST_F(TextInputTest, text_input_ctrl_shortcuts_select_copy_cut_and_paste)
{
  ScopedClipboardBackend clipboard;

  runtime.eval(R"(
    (pixils.clipboard/set-text! "")
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_a);

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_c);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"abcd\"");

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_x);

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  EXPECT_EQ(text->to_string(), "\"\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 0);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"abcd\"");

  runtime.eval("(pixils.clipboard/set-text! \"xy\")");
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_v);

  text = get_keyword(session.active_mode->state, "text");
  last_change = get_keyword(session.active_mode->state, "last-change");
  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  EXPECT_EQ(text->to_string(), "\"xy\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"xy\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 2);
}

TEST_F(TextInputTest, read_only_text_input_shortcuts_copy_without_cutting_or_pasting)
{
  ScopedClipboardBackend clipboard;

  runtime.eval(R"(
    (pixils.clipboard/set-text! "")
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "abcd"
                              :last-change nil})
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :text)
                           :read-only? true
                           :auto-focus? true}}]
       :on {:text-input/change (fn [state event ctx]
                                 (-> state
                                     (assoc :text (-> event :payload :value))
                                     (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_a);
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_c);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"abcd\"");

  runtime.eval("(pixils.clipboard/set-text! \"sentinel\")");
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_x);

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(text->to_string(), "\"abcd\"");
  EXPECT_EQ(last_change->type, Roo::Value::Type::NIL);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"sentinel\"");

  runtime.eval("(pixils.clipboard/set-text! \"xy\")");
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_v);

  text = get_keyword(session.active_mode->state, "text");
  last_change = get_keyword(session.active_mode->state, "last-change");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(text->to_string(), "\"abcd\"");
  EXPECT_EQ(last_change->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_mouse_down_focuses_and_places_caret)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 80
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);
  ASSERT_FALSE(session.focus_state.has_focus());

  auto click_y = text_input_inner->bounds.y + (text_input_inner->bounds.h / 2);
  input().mouse_down({text_input_inner->bounds.x + 1, click_y});
  update_cycle();
  input().mouse_up({text_input_inner->bounds.x + 1, click_y});
  update_cycle();

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_TRUE(session.focus_state.has_focus());
  EXPECT_EQ(session.focus_state.focused.lock().get(), text_input_inner.get());
  EXPECT_EQ(cursor_index->num().get_int(), 0);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);

  input().mouse_down({text_input_inner->bounds.x + text_input_inner->bounds.w - 1,
                      click_y});
  update_cycle();
  input().mouse_up({text_input_inner->bounds.x + text_input_inner->bounds.w - 1,
                    click_y});
  update_cycle();

  cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  selection_start = get_keyword(text_input_inner->state, "selection-start");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_shift_click_extends_selection)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 80
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  auto start_x = text_input_inner->bounds.x + 1;
  auto end_x = text_input_inner->bounds.x + text_input_inner->bounds.w - 1;
  auto click_y = text_input_inner->bounds.y + (text_input_inner->bounds.h / 2);
  input().mouse_down({start_x, click_y});
  update_cycle();
  input().mouse_up({start_x, click_y});
  update_cycle();

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().mouse_down({end_x, click_y});
  update_cycle();
  input().mouse_up({end_x, click_y});
  update_cycle();
  input().key_up(SDLK_LSHIFT);
  update_cycle();

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);
}

TEST_F(TextInputTest, text_input_mouse_drag_updates_selection_cursor)
{
  SDLMock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 80
                           :height 22
                           :text {:font :font/test-font}}
                   :state {:value (pixils.ui/bind-state :text)}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();
  render_cycle();

  auto text_input_inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);

  auto start_x = text_input_inner->bounds.x + 1;
  auto end_x = text_input_inner->bounds.x + text_input_inner->bounds.w - 1;
  auto drag_y = text_input_inner->bounds.y + (text_input_inner->bounds.h / 2);
  input().mouse_down({start_x, drag_y});
  update_cycle();
  input().mouse_move({end_x, drag_y});
  update_cycle();
  input().mouse_up({end_x, drag_y});
  update_cycle();

  auto cursor_index = get_keyword(text_input_inner->state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);
}
