
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

    Lisple::sptr_rtval mouse_pos;
    Lisple::sptr_rtval mouse_button_down;
    Lisple::sptr_rtval mouse_button_up;
    Lisple::sptr_rtval mouse_held = Lisple::RTValue::vector({});
    bool mouse_moved = false;

    FrameEvents();

    bool is_key_held(const Lisple::RTValue& key) const;

    void do_key_down(const SDL_KeyboardEvent& event);
    void do_key_up(const SDL_KeyboardEvent& event);

    void do_mouse_motion(int x, int y);
    void do_mouse_button_down(const SDL_MouseButtonEvent& event);
    void do_mouse_button_up(const SDL_MouseButtonEvent& event);
  };
} // namespace Pixils

#endif
