
#ifndef PIXILS__TIMER_H
#define PIXILS__TIMER_H

namespace Pixils
{
  class Timer
  {
    long mark;

   public:
    Timer();

    void reset();
    long get_elapsed();
    bool is_elapsed(long nanoseconds);
  };
} // namespace Pixils

#endif
