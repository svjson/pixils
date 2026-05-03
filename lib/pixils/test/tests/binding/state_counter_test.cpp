#include "../fixture.h"

#include <gtest/gtest.h>

class StateCounterTest : public BaseFixture
{
};

TEST_F(StateCounterTest, advance_returns_new_counter_value)
{
  auto value = runtime.eval(
    "(pixils.state.counter/value "
    "(pixils.state.counter/advance (pixils.state.counter/make {:end 4})))");

  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 1);
}

TEST_F(StateCounterTest, every_only_steps_value_on_configured_pulse)
{
  auto first = runtime.eval(
    "(pixils.state.counter/value "
    "(pixils.state.counter/advance (pixils.state.counter/make {:end 5 :every 2})))");
  auto second = runtime.eval(
    "(pixils.state.counter/value "
    "(pixils.state.counter/advance "
    "(pixils.state.counter/advance (pixils.state.counter/make {:end 5 :every 2}))))");

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->num().get_int(), 0);
  EXPECT_EQ(second->num().get_int(), 1);
}

TEST_F(StateCounterTest, advance_at_updates_counter_and_runs_on_step_callback_only_when_stepped)
{
  auto first = runtime.eval(R"(
    (:image
      (pixils.state.counter/advance-at
        {:counter (pixils.state.counter/make {:end 5 :every 2})
         :image :idle}
        :counter
        {:on-step (fn [entity] (assoc entity :image :active))}))
  )");
  auto second = runtime.eval(R"(
    (:image
      (pixils.state.counter/advance-at
        (pixils.state.counter/advance-at
          {:counter (pixils.state.counter/make {:end 5 :every 2})
           :image :idle}
          :counter
          {:on-step (fn [entity] (assoc entity :image :active))})
        :counter
        {:on-step (fn [entity] (assoc entity :image :active))}))
  )");

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->str(), "idle");
  EXPECT_EQ(second->str(), "active");
}

TEST_F(StateCounterTest, value_at_reads_counter_value_from_host_map)
{
  auto value = runtime.eval(
    "(pixils.state.counter/value-at "
    "{:counter (pixils.state.counter/advance (pixils.state.counter/make {:end 3}))} "
    ":counter)");

  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 1);
}
