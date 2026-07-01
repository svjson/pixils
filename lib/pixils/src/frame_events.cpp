
#include <pixils/binding/point_namespace.h>
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <algorithm>
#include <roo/runtime/seq.h>

namespace Pixils
{
  namespace
  {
    Roo::sptr_val mouse_button_keyword(Uint8 button)
    {
      switch (button)
      {
      case SDL_BUTTON_LEFT:
        return Roo::keyword("left");
      case SDL_BUTTON_RIGHT:
        return Roo::keyword("right");
      case SDL_BUTTON_MIDDLE:
        return Roo::keyword("middle");
      default:
        return Roo::Constant::NIL;
      }
    }
  } // namespace

  FrameEvents::FrameEvents()
    : mouse_pos(Script::PointAdapter::make_unique(0.0f, 0.0f))
    , mouse_button_down(Roo::Constant::NIL)
    , mouse_button_up(Roo::Constant::NIL)
  {
  }

  void FrameEvents::clear_keyboard_transients()
  {
    key_down = Roo::Constant::NIL;
    key_up = Roo::Constant::NIL;
  }

  void FrameEvents::clear_transients()
  {
    clear_keyboard_transients();
    mouse_button_down = Roo::Constant::NIL;
    mouse_button_up = Roo::Constant::NIL;
    mouse_button_down_clicks = 1;
    mouse_button_up_clicks = 1;
    mouse_moved = false;
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
    if (*btn == *Roo::Constant::NIL) return;
    mouse_button_down = btn;
    mouse_button_down_clicks = std::max<uint8_t>(event.clicks, 1);
    Roo::append(*mouse_held, btn);
  }

  void FrameEvents::do_mouse_button_up(const SDL_MouseButtonEvent& event)
  {
    auto btn = mouse_button_keyword(event.button);
    if (*btn == *Roo::Constant::NIL) return;
    mouse_button_up = btn;
    mouse_button_up_clicks = std::max<uint8_t>(event.clicks, 1);
    auto& children = mouse_held->mut_elements();
    auto it = std::remove_if(children.begin(),
                             children.end(),
                             [btn](const Roo::sptr_val& hbtn) { return *btn == *hbtn; });
    if (it != children.end()) children.erase(it);
  }

  void FrameEvents::do_key_down(const SDL_KeyboardEvent& event)
  {
    auto key = Keyboard::key_event_to_roo_key(event);
    if (*key == *Roo::Constant::NIL)
    {
      return;
    }

    if (this->is_key_held(*key))
    {
      return;
    }

    Roo::append(*this->held_keys, key);
    this->key_down = key;
  }

  void FrameEvents::do_key_up(const SDL_KeyboardEvent& event)
  {
    auto key = Keyboard::key_event_to_roo_key(event);
    key_up = key;

    auto& children = this->held_keys->mut_elements();
    auto it = std::remove_if(children.begin(),
                             children.end(),
                             [key](const Roo::sptr_val& hkey) { return *key == *hkey; });

    if (it != children.end())
    {
      children.erase(it);
    }
  }

  bool FrameEvents::is_key_held(const Roo::Value& key) const
  {
    auto& children = this->held_keys->mut_elements();
    auto it = std::find_if(children.begin(),
                           children.end(),
                           [key](const Roo::sptr_val& hkey) { return key == *hkey; });

    return it != children.end();
  }
} // namespace Pixils
