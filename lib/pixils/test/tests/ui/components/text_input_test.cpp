#include "../../render_fixture.h"
#include <pixils/clipboard.h>
#include <pixils/program.h>
#include <pixils/ui/view_layout.h>

#include <SDL3/SDL_keycode.h>
#include <algorithm>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <set>

using TextInputTest = RenderFixture;

namespace
{
  void layout_active_mode(Roo::Runtime& runtime, Pixils::Runtime::Session& session)
  {
    Pixils::UI::layout_view_tree(
      session.active_mode,
      {0, 0, session.render_ctx.buffer_dim.w, session.render_ctx.buffer_dim.h},
      runtime,
      session.hook_args.render_args[1]);
  }

  std::shared_ptr<Pixils::Runtime::View> find_first_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->definition && view->definition->name == mode_name) return view;

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
    return std::any_of(ops.begin(),
                       ops.end(),
                       [&](const auto& op)
                       {
                         return op.type == RenderOpType::FILL_RECT &&
                                op.rendered_rect.x == rect.x &&
                                op.rendered_rect.y == rect.y &&
                                op.rendered_rect.w == rect.w && op.rendered_rect.h == rect.h;
                       });
  }

  template <typename UpdateCycle>
  void press_ctrl_shortcut(InputSimulator& input, UpdateCycle update_cycle, SDL_Keycode key)
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
    bool set_succeeds = true;

    std::string get_text() override { return text; }
    bool has_text() override { return !text.empty(); }
    bool set_text(const std::string& next_text, std::string* error) override
    {
      if (!set_succeeds)
      {
        if (error) *error = "test clipboard failure";
        return false;
      }
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

TEST_F(TextInputTest, text_input_caret_uses_text_metrics_on_first_layout)
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
  update_cycle();
  layout_active_mode(runtime, session);

  auto inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  auto caret = find_first_mode(session.active_mode, "ui/text-input-caret");
  ASSERT_NE(inner, nullptr);
  ASSERT_NE(caret, nullptr);

  auto layout = get_keyword(inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto cursor_y = get_keyword(layout, "cursor-y");
  auto cursor_h = get_keyword(layout, "cursor-h");
  ASSERT_NE(cursor_y, nullptr);
  ASSERT_NE(cursor_h, nullptr);
  EXPECT_EQ(
    caret->bounds.y,
    inner->effective_style.content_rect(inner->bounds).y + cursor_y->num().get_int());
  EXPECT_EQ(caret->bounds.h, cursor_h->num().get_int());
  EXPECT_GT(cursor_y->num().get_int(), 0);
}

TEST_F(TextInputTest, text_input_caret_tracks_edits_in_the_same_frame)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value "A"
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto inner = find_first_mode(session.active_mode, "ui/text-input-inner");
  auto caret = find_first_mode(session.active_mode, "ui/text-input-caret");
  ASSERT_NE(inner, nullptr);
  ASSERT_NE(caret, nullptr);

  input().key_down(SDLK_BACKSPACE);
  frame_cycle();

  auto layout = get_keyword(inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto caret_x = get_keyword(layout, "caret-x");
  ASSERT_NE(caret_x, nullptr);
  EXPECT_EQ(caret->bounds.x,
            inner->effective_style.content_rect(inner->bounds).x + caret_x->num().get_int());

  input().key_up(SDLK_BACKSPACE);
  input().key_down(SDLK_A);
  frame_cycle();

  layout = get_keyword(inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  caret_x = get_keyword(layout, "caret-x");
  ASSERT_NE(caret_x, nullptr);
  EXPECT_EQ(caret->bounds.x,
            inner->effective_style.content_rect(inner->bounds).x + caret_x->num().get_int());
}

TEST_F(TextInputTest, text_input_scrolls_horizontally_to_keep_caret_visible)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  auto layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto scroll_x = get_keyword(layout, "scroll-x");
  auto caret_x = get_keyword(layout, "caret-x");
  ASSERT_NE(scroll_x, nullptr);
  ASSERT_NE(caret_x, nullptr);
  EXPECT_GT(scroll_x->num().get_int(), 0);
  EXPECT_LE(caret_x->num().get_int(), 40);

  input().key_down(SDLK_HOME);
  update_cycle();
  layout_active_mode(runtime, session);

  layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  scroll_x = get_keyword(layout, "scroll-x");
  caret_x = get_keyword(layout, "caret-x");
  ASSERT_NE(scroll_x, nullptr);
  ASSERT_NE(caret_x, nullptr);
  EXPECT_EQ(scroll_x->num().get_int(), 0);
  EXPECT_EQ(caret_x->num().get_int(), 0);

  input().key_down(SDLK_END);
  update_cycle();
  layout_active_mode(runtime, session);

  layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  scroll_x = get_keyword(layout, "scroll-x");
  ASSERT_NE(scroll_x, nullptr);
  EXPECT_GT(scroll_x->num().get_int(), 0);
}

TEST_F(TextInputTest, text_input_scrolls_one_pixel_when_text_exactly_fills_width)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  auto layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto scroll_x = get_keyword(layout, "scroll-x");
  auto caret_x = get_keyword(layout, "caret-x");
  ASSERT_NE(scroll_x, nullptr);
  ASSERT_NE(caret_x, nullptr);
  EXPECT_EQ(scroll_x->num().get_int(), 1);
  EXPECT_EQ(caret_x->num().get_int(), 39);
}

TEST_F(TextInputTest, text_input_shift_home_copy_clips_scrolled_selection_highlight)
{
  ScopedClipboardBackend clipboard;
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};

  runtime.eval(R"(
    (pixils.clipboard/set-text! "")
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})

    (pixils/defmode root-mode
      {:init (fn [state ctx] {:text "AAAAAAAAAAAA"})
       :children [{:mode 'ui/text-input
                   :style {:width 30
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

  auto layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto scroll_x = get_keyword(layout, "scroll-x");
  ASSERT_NE(scroll_x, nullptr);
  EXPECT_GT(scroll_x->num().get_int(), 0);

  input().key_down(SDLK_LSHIFT);
  update_cycle();
  input().key_down(SDLK_HOME);
  update_cycle();
  input().key_up(SDLK_HOME);
  update_cycle();
  input().key_up(SDLK_LSHIFT);
  update_cycle();
  render_cycle();

  layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto selection_x = get_keyword(layout, "selection-x");
  auto selection_w = get_keyword(layout, "selection-w");
  ASSERT_NE(selection_x, nullptr);
  ASSERT_NE(selection_w, nullptr);
  EXPECT_GE(selection_x->num().get_int(), 0);
  EXPECT_GT(selection_w->num().get_int(), 0);
  EXPECT_LE(selection_x->num().get_int() + selection_w->num().get_int(),
            text_input_inner->bounds.w);

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_C);

  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"AAAAAAAAAAAA\"");
}

TEST_F(TextInputTest, text_input_renders_selection_background_under_single_text_layer)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  auto layout = get_keyword(text_input_inner->ui_state, "layout");
  ASSERT_NE(layout, nullptr);
  auto selection_x = get_keyword(layout, "selection-x");
  auto selection_y = get_keyword(layout, "cursor-y");
  auto selection_w = get_keyword(layout, "selection-w");
  auto selection_h = get_keyword(layout, "cursor-h");
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
                                   [&](const auto& op)
                                   {
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
                  [](const auto& op) { return op.type == RenderOpType::RENDER_COPY; });
  EXPECT_EQ(copy_ops_after_selection, 4);
}

TEST_F(TextInputTest, text_input_text_never_wraps)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_A);

  auto cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->ui_state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_C);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"abcd\"");

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_X);

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  EXPECT_EQ(text->to_string(), "\"\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 0);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"abcd\"");

  runtime.eval("(pixils.clipboard/set-text! \"xy\")");
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_V);

  text = get_keyword(session.active_mode->state, "text");
  last_change = get_keyword(session.active_mode->state, "last-change");
  cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  ASSERT_NE(cursor_index, nullptr);
  EXPECT_EQ(text->to_string(), "\"xy\"");
  EXPECT_EQ(last_change->to_string(), "{:value \"xy\"}");
  EXPECT_EQ(cursor_index->num().get_int(), 2);
}

TEST_F(TextInputTest, text_input_cut_does_not_delete_when_clipboard_write_fails)
{
  ScopedClipboardBackend clipboard;
  clipboard.backend.text = "sentinel";
  clipboard.backend.set_succeeds = false;

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

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_A);
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_C);
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_X);

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  auto cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->ui_state, "selection-end");
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
  EXPECT_EQ(clipboard.backend.text, "sentinel");
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

  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_A);
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_C);
  EXPECT_EQ(runtime.eval("(pixils.clipboard/get-text)")->to_string(), "\"abcd\"");

  runtime.eval("(pixils.clipboard/set-text! \"sentinel\")");
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_X);

  auto text = get_keyword(session.active_mode->state, "text");
  auto last_change = get_keyword(session.active_mode->state, "last-change");
  auto cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->ui_state, "selection-end");
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
  press_ctrl_shortcut(input(), [&]() { update_cycle(); }, SDLK_V);

  text = get_keyword(session.active_mode->state, "text");
  last_change = get_keyword(session.active_mode->state, "last-change");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(text->to_string(), "\"abcd\"");
  EXPECT_EQ(last_change->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_mouse_down_focuses_and_places_caret)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  auto cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_TRUE(session.focus_state.has_focus());
  EXPECT_EQ(session.focus_state.focused.lock().get(), text_input_inner.get());
  EXPECT_EQ(cursor_index->num().get_int(), 0);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);

  input().mouse_down({text_input_inner->bounds.x + text_input_inner->bounds.w - 1, click_y});
  update_cycle();
  input().mouse_up({text_input_inner->bounds.x + text_input_inner->bounds.w - 1, click_y});
  update_cycle();

  cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->type, Roo::Value::Type::NIL);
}

TEST_F(TextInputTest, text_input_shift_click_extends_selection)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  auto cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->ui_state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);
}

TEST_F(TextInputTest, text_input_mouse_drag_updates_selection_cursor)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
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

  auto cursor_index = get_keyword(text_input_inner->ui_state, "cursor-index");
  auto selection_start = get_keyword(text_input_inner->ui_state, "selection-start");
  auto selection_end = get_keyword(text_input_inner->ui_state, "selection-end");
  ASSERT_NE(cursor_index, nullptr);
  ASSERT_NE(selection_start, nullptr);
  ASSERT_NE(selection_end, nullptr);
  EXPECT_EQ(cursor_index->num().get_int(), 4);
  EXPECT_EQ(selection_start->num().get_int(), 0);
  EXPECT_EQ(selection_end->num().get_int(), 4);
}
