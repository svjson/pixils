#ifndef PIXILS__STATE__TIMER_H
#define PIXILS__STATE__TIMER_H

namespace Pixils::State
{
  struct Timer
  {
    long interval_ms = 1000;
    long last_tick_ms = 0;
    bool started = false;
    bool ticked = false;
  };

  long timer_now_ms();
  Timer tick_timer(const Timer& timer);
  Timer tick_timer_at(const Timer& timer, long now_ms);
} // namespace Pixils::State

#endif /* PIXILS__STATE__TIMER_H */
