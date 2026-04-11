
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>

#include <SDL2/SDL_events.h>
#include <algorithm>
#include <lisple/runtime/seq.h>

namespace Pixils
{

  void FrameEvents::do_key_down(const SDL_KeyboardEvent& event)
  {
    auto key = Keyboard::key_event_to_lisple_key(event);
    if (*key == *Lisple::Constant::NIL)
    {
      return;
    }

    if (this->is_key_held(*key))
    {
      return;
    }

    Lisple::append(*this->held_keys, key);
    this->key_down = key;
  }

  void FrameEvents::do_key_up(const SDL_KeyboardEvent& event)
  {
    auto key = Keyboard::key_event_to_lisple_key(event);

    auto& children = this->held_keys->mut_elements();
    auto it =
      std::remove_if(children.begin(),
                     children.end(),
                     [key](const Lisple::sptr_rtval& hkey) { return *key == *hkey; });

    if (it != children.end())
    {
      children.erase(it);
    }
  }

  bool FrameEvents::is_key_held(const Lisple::RTValue& key) const
  {
    auto& children = this->held_keys->mut_elements();
    auto it = std::find_if(children.begin(),
                           children.end(),
                           [key](const Lisple::sptr_rtval& hkey) { return key == *hkey; });

    return it != children.end();
  }
} // namespace Pixils
