
#include <pixils/timer.h>

#include <bits/chrono.h>

namespace Pixils
{
  long epoch_time()
  {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  }

  Timer::Timer()
    : mark(epoch_time())
  {
  }

  void Timer::reset()
  {
    this->mark = epoch_time();
  }

  long Timer::get_elapsed()
  {
    return epoch_time() - mark;
  }

  bool Timer::is_elapsed(long nanoseconds)
  {
    return get_elapsed() >= nanoseconds;
  }

} // namespace Pixils
