
#ifndef _PIXILS__FRAME_EVENTS_H_
#define _PIXILS__FRAME_EVENTS_H_

#include <SDL2/SDL_events.h>
#include <lisple/form.h>

namespace Pixils
{
  struct FrameEvents
  {
    std::shared_ptr<Lisple::Array> held_keys = std::make_shared<Lisple::Array>();

    Lisple::sptr_sobject key_down;

    bool is_key_held(const Lisple::Key& key) const;

    void do_key_down(const SDL_KeyboardEvent& event);
    void do_key_up(const SDL_KeyboardEvent& event);
  };
} // namespace Pixils

#endif
