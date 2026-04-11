
#ifndef _PIXILS__FRAME_EVENTS_H_
#define _PIXILS__FRAME_EVENTS_H_

#include <SDL2/SDL_events.h>
#include <lisple/runtime/value.h>

namespace Pixils
{
  struct FrameEvents
  {
    Lisple::sptr_rtval held_keys = Lisple::RTValue::vector({});
    Lisple::sptr_rtval key_down;

    bool is_key_held(const Lisple::RTValue& key) const;

    void do_key_down(const SDL_KeyboardEvent& event);
    void do_key_up(const SDL_KeyboardEvent& event);
  };
} // namespace Pixils

#endif
