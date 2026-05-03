#include <pixils/state/counter.h>

#include <algorithm>

namespace Pixils::State
{
  Counter advance_counter(const Counter& counter)
  {
    Counter next = counter;
    next.wrapped = false;

    next.phase += 1;
    if (next.phase < next.every)
    {
      return next;
    }

    next.phase = 0;

    switch (next.mode)
    {
    case CounterMode::LOOP:
      if (next.end <= 0)
      {
        next.value = next.start;
        return next;
      }
      next.value = (next.value + 1) % next.end;
      next.wrapped = next.value == next.start;
      return next;

    case CounterMode::STOP:
      next.value = std::min(next.value + 1, next.end);
      return next;
    }

    return next;
  }

  std::string counter_mode_name(CounterMode mode)
  {
    switch (mode)
    {
    case CounterMode::LOOP:
      return "loop";
    case CounterMode::STOP:
      return "stop";
    }

    return "loop";
  }
} // namespace Pixils::State
