
#include <pixils/binding/point_namespace.h>
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <algorithm>
#include <lisple/runtime/seq.h>

namespace Pixils
{
  namespace
  {
    Lisple::sptr_rtval mouse_button_keyword(Uint8 button)
    {
      switch (button)
      {
      case SDL_BUTTON_LEFT:
        return Lisple::RTValue::keyword("left");
      case SDL_BUTTON_RIGHT:
        return Lisple::RTValue::keyword("right");
      case SDL_BUTTON_MIDDLE:
        return Lisple::RTValue::keyword("middle");
      default:
        return Lisple::Constant::NIL;
      }
    }
  } // namespace

  FrameEvents::FrameEvents()
    : mouse_pos(Script::PointAdapter::make_unique(0.0f, 0.0f))
    , mouse_button_down(Lisple::Constant::NIL)
    , mouse_button_up(Lisple::Constant::NIL)
  {
  }

  void FrameEvents::do_mouse_motion(int x, int y)
  {
    mouse_pos =
      Script::PointAdapter::make_unique(static_cast<float>(x), static_cast<float>(y));
    mouse_moved = true;
  }

  void FrameEvents::do_mouse_button_down(const SDL_MouseButtonEvent& event)
  {
    auto btn = mouse_button_keyword(event.button);
    if (*btn == *Lisple::Constant::NIL) return;
    mouse_button_down = btn;
    Lisple::append(*mouse_held, btn);
  }

  void FrameEvents::do_mouse_button_up(const SDL_MouseButtonEvent& event)
  {
    auto btn = mouse_button_keyword(event.button);
    if (*btn == *Lisple::Constant::NIL) return;
    mouse_button_up = btn;
    auto& children = mouse_held->mut_elements();
    auto it =
      std::remove_if(children.begin(),
                     children.end(),
                     [btn](const Lisple::sptr_rtval& hbtn) { return *btn == *hbtn; });
    if (it != children.end()) children.erase(it);
  }

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
