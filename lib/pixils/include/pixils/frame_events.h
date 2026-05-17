
#ifndef _PIXILS__FRAME_EVENTS_H_
#define _PIXILS__FRAME_EVENTS_H_

#include <SDL2/SDL_events.h>
#include <lisple/runtime/value.h>

namespace Pixils
{
  struct FrameEvents
  {
    Lisple::sptr_val held_keys = Lisple::vector({});
    Lisple::sptr_val key_down = Lisple::Constant::NIL;
    Lisple::sptr_val key_up = Lisple::Constant::NIL;

    Lisple::sptr_val mouse_pos;
    Lisple::sptr_val mouse_button_down;
    Lisple::sptr_val mouse_button_up;
    uint8_t mouse_button_down_clicks = 1;
    uint8_t mouse_button_up_clicks = 1;
    Lisple::sptr_val mouse_held = Lisple::vector({});
    bool mouse_moved = false;

    FrameEvents();

    bool is_key_held(const Lisple::Value& key) const;

    void do_key_down(const SDL_KeyboardEvent& event);
    void do_key_up(const SDL_KeyboardEvent& event);

    void do_mouse_motion(int x, int y);
    void do_mouse_button_down(const SDL_MouseButtonEvent& event);
    void do_mouse_button_up(const SDL_MouseButtonEvent& event);
  };
} // namespace Pixils

#endif
