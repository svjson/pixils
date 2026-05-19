#include "../../render_fixture.h"

#include <SDL2/SDL_keycode.h>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using NumberInputTest = RenderFixture;

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
