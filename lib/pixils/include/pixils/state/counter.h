#ifndef PIXILS__STATE__COUNTER_H
#define PIXILS__STATE__COUNTER_H

#include <optional>
#include <string>

namespace Pixils::State
{
  enum class CounterMode
  {
    LOOP,
    STOP,
  };

  struct Counter
  {
    int start = 0;
    int value = 0;
    int end = 10;
    CounterMode mode = CounterMode::LOOP;
    bool wrapped = false;
    int every = 1;
    int phase = 0;
  };

  Counter advance_counter(const Counter& counter);
  std::string counter_mode_name(CounterMode mode);
} // namespace Pixils::State

#endif /* PIXILS__STATE__COUNTER_H */
