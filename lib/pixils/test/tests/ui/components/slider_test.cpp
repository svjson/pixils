#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using SliderTest = RenderFixture;

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

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({95, 5});
  update_cycle();

  auto zoom =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("zoom"));
  ASSERT_NE(zoom, nullptr);
  EXPECT_EQ(zoom->num().get_int(), 4);
}
