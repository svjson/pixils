#include "../fixture.h"
#include <pixils/state/timer.h>

#include <gtest/gtest.h>

class StateTimerTest : public BaseFixture
{
};

TEST_F(StateTimerTest, tick_starts_timer_without_firing)
{
  auto timer = Pixils::State::tick_timer_at(Pixils::State::Timer{.interval_ms = 100}, 1000);

  EXPECT_TRUE(timer.started);
  EXPECT_FALSE(timer.ticked);
  EXPECT_EQ(timer.last_tick_ms, 1000);
}

TEST_F(StateTimerTest, tick_fires_when_interval_has_elapsed)
{
  Pixils::State::Timer timer{.interval_ms = 100, .last_tick_ms = 1000, .started = true};

  auto ticked = Pixils::State::tick_timer_at(timer, 1120);

  EXPECT_TRUE(ticked.ticked);
  EXPECT_EQ(ticked.last_tick_ms, 1120);
}

TEST_F(StateTimerTest, tick_ignores_missed_intervals)
{
  Pixils::State::Timer timer{.interval_ms = 100, .last_tick_ms = 1000, .started = true};

  auto first = Pixils::State::tick_timer_at(timer, 1350);
  auto second = Pixils::State::tick_timer_at(first, 1400);

  EXPECT_TRUE(first.ticked);
  EXPECT_EQ(first.last_tick_ms, 1350);
  EXPECT_FALSE(second.ticked);
  EXPECT_EQ(second.last_tick_ms, 1350);
}

TEST_F(StateTimerTest, tick_at_updates_timer_in_state)
{
  auto state = runtime.eval(R"(
    (pixils.state.timer/tick-at
      {:physics-timer (pixils.state.timer/make {:interval-ms 100
                                                :started? true
                                                :last-tick-ms 0})
       :value 42}
      :physics-timer)
  )");
  runtime.get_current_namespace().store("state", state);

  auto ticked = runtime.eval("(pixils.state.timer/ticked-at? state :physics-timer)");
  auto value = runtime.eval("(:value state)");

  ASSERT_NE(ticked, nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_TRUE(std::get<bool>(ticked->value));
  EXPECT_EQ(value->num().get_int(), 42);
}
