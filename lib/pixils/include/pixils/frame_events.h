
#ifndef _PIXILS__FRAME_EVENTS_H_
#define _PIXILS__FRAME_EVENTS_H_

#include <SDL2/SDL_events.h>
#include <roo/runtime/value.h>

namespace Pixils
{
  struct FrameEvents
  {
    Roo::sptr_val held_keys = Roo::vector({});
    Roo::sptr_val key_down = Roo::Constant::NIL;
    Roo::sptr_val key_up = Roo::Constant::NIL;

    Roo::sptr_val mouse_pos;
    Roo::sptr_val mouse_button_down;
    Roo::sptr_val mouse_button_up;
    uint8_t mouse_button_down_clicks = 1;
    uint8_t mouse_button_up_clicks = 1;
    Roo::sptr_val mouse_held = Roo::vector({});
    bool mouse_moved = false;

    FrameEvents();

    bool is_key_held(const Roo::Value& key) const;

    void do_key_down(const SDL_KeyboardEvent& event);
    void do_key_up(const SDL_KeyboardEvent& event);

    void do_mouse_motion(int x, int y);
    void do_mouse_button_down(const SDL_MouseButtonEvent& event);
    void do_mouse_button_up(const SDL_MouseButtonEvent& event);
  };
} // namespace Pixils

#endif
