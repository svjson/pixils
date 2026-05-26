#include "../../render_fixture.h"

#include <pixils/program.h>
#include <pixils/ui/view_layout.h>

#include <SDL2/SDL_keycode.h>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using NumberInputTest = RenderFixture;

namespace
{
  void layout_active_mode(Lisple::Runtime& runtime,
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
} // namespace

TEST_F(NumberInputTest, number_input_accepts_digits_and_rejects_other_text)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:amount nil
                              :last-change nil})
       :children [{:mode 'ui/number-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :amount)
                           :auto-focus? true}}]
       :on {:number-input/change (fn [state event ctx]
                                   (assoc state :last-change (:payload event)))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  update_cycle();
  render_cycle();

  input().key_down(SDLK_1);
  update_cycle();
  input().key_up(SDLK_1);
  update_cycle();
  input().key_down(SDLK_a);
  update_cycle();
  input().key_up(SDLK_a);
  update_cycle();
  input().key_down(SDLK_2);
  update_cycle();

  auto amount =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("amount"));
  auto last_change =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("last-change"));

  ASSERT_NE(amount, nullptr);
  ASSERT_NE(last_change, nullptr);
  EXPECT_EQ(amount->num().get_int(), 12);
  EXPECT_EQ(last_change->to_string(), "{:value 12 :text \"12\"}");
}

TEST_F(NumberInputTest, number_input_arrow_keys_step_and_clamp_bound_value)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:amount 10})
       :children [{:mode 'ui/number-input
                   :style {:width 80 :height 22}
                   :state {:value (pixils.ui/bind-state :amount)
                           :min 0
                           :max 12
                           :step 5
                           :auto-focus? true}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  update_cycle();
  render_cycle();

  input().key_down(SDLK_UP);
  update_cycle();
  input().key_up(SDLK_UP);
  update_cycle();

  auto amount =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("amount"));
  ASSERT_NE(amount, nullptr);
  EXPECT_EQ(amount->num().get_int(), 12);

  input().key_down(SDLK_DOWN);
  update_cycle();
  input().key_up(SDLK_DOWN);
  update_cycle();
  input().key_down(SDLK_DOWN);
  update_cycle();
  input().key_up(SDLK_DOWN);
  update_cycle();
  input().key_down(SDLK_DOWN);
  update_cycle();

  amount = Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("amount"));
  ASSERT_NE(amount, nullptr);
  EXPECT_EQ(amount->num().get_int(), 0);
}

TEST_F(NumberInputTest, windows_3_text_input_caret_aligns_with_text)
{
  runtime.eval(R"(
    (pixils/defprogram text-input-caret-test-program
      {:theme 'pixils/windows-3
       :initial-mode 'root-mode})

    (pixils/defmode root-mode
      {:children [{:mode 'ui/text-input
                   :style {:width 80 :height 22}
                   :state {:value "A"
                           :auto-focus? true}}]})
  )");

  Pixils::load_program(runtime, session);
  update_cycle();
  layout_active_mode(runtime, session);
  update_cycle();
  layout_active_mode(runtime, session);

  auto text = find_first_mode(session.active_mode, "ui/text");
  auto caret = find_first_mode(session.active_mode, "ui/text-input-caret");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(caret, nullptr);

  EXPECT_EQ(caret->bounds.y, text->bounds.y);
  EXPECT_EQ(caret->bounds.h, text->bounds.h);
}

TEST_F(NumberInputTest, windows_3_number_input_caret_aligns_with_text)
{
  runtime.eval(R"(
    (pixils/defprogram number-input-caret-test-program
      {:theme 'pixils/windows-3
       :initial-mode 'root-mode})

    (pixils/defmode root-mode
      {:children [{:mode 'ui/number-input
                   :style {:width 80 :height 22}
                   :state {:value 12
                           :auto-focus? true}}]})
  )");

  Pixils::load_program(runtime, session);
  update_cycle();
  layout_active_mode(runtime, session);
  update_cycle();
  layout_active_mode(runtime, session);

  auto text = find_first_mode(session.active_mode, "ui/text");
  auto caret = find_first_mode(session.active_mode, "ui/text-input-caret");
  ASSERT_NE(text, nullptr);
  ASSERT_NE(caret, nullptr);

  EXPECT_EQ(caret->bounds.y, text->bounds.y);
  EXPECT_EQ(caret->bounds.h, text->bounds.h);
}
