
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>

#include <SDL2/SDL_events.h>
#include <algorithm>

namespace Pixils
{

  void FrameEvents::do_key_down(const SDL_KeyboardEvent& event)
  {
    auto key = Keyboard::key_event_to_lisple_key(event);
    if (key == Lisple::NIL)
    {
      return;
    }

    if (this->is_key_held(key->as<Lisple::Key>()))
    {
      return;
    }

    this->held_keys->append(Lisple::Key::make(key->as<Lisple::Key>().value));
    this->key_down = Lisple::Key::make(key->as<Lisple::Key>().value);
  }

  void FrameEvents::do_key_up(const SDL_KeyboardEvent& event)
  {
    auto key = Keyboard::key_event_to_lisple_key(event);

    auto it = std::remove_if(this->held_keys->children.begin(), this->held_keys->children.end(),
                             [key](const Lisple::sptr_sobject& hkey)
                             { return key->has_value(hkey->as<Lisple::Key>().value); });

    if (it != this->held_keys->children.end())
    {
      this->held_keys->children.erase(it);
    }
  }

  bool FrameEvents::is_key_held(const Lisple::Key& key) const
  {
    auto it = std::find_if(this->held_keys->children.begin(), this->held_keys->children.end(),
                           [key](const Lisple::sptr_sobject& hkey)
                           { return key.has_value(hkey->as<Lisple::Key>().value); });

    return it != this->held_keys->children.end();
  }
} // namespace Pixils
