#include <pixils/state/timer.h>

#include <algorithm>
#include <chrono>

namespace Pixils::State
{
  long timer_now_ms()
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  }

  Timer tick_timer(const Timer& timer)
  {
    return tick_timer_at(timer, timer_now_ms());
  }

  Timer tick_timer_at(const Timer& timer, long now_ms)
  {
    Timer next = timer;
    next.ticked = false;

    if (!next.started)
    {
      next.started = true;
      next.last_tick_ms = now_ms;
      return next;
    }

    if (now_ms - next.last_tick_ms >= std::max(0L, next.interval_ms))
    {
      next.last_tick_ms = now_ms;
      next.ticked = true;
    }

    return next;
  }
} // namespace Pixils::State
