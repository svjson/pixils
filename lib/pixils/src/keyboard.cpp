
#include <pixils/keyboard.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <ctype.h>
#include <lisple/form.h>
#include <lisple/runtime/value.h>
#include <lisple/type.h>
#include <utility>

namespace Pixils
{
  namespace Keyboard
  {
    /*
     * KeyCode
     */
    bool KeyCode::operator==(const KeyCode& other) const
    {
      return symbol == other.symbol && code == other.code;
    }

    /* Shortcut */
    bool Shortcut::operator==(const Shortcut& other) const
    {
      return keycode.code == other.keycode.code && alt == other.alt && ctrl == other.ctrl &&
             shift == other.shift;
    }

    bool Shortcut::matches(const SDL_Event& event) const
    {
      if (keycode.code != event.key.keysym.sym) return false;

      if (((event.key.keysym.mod & KMOD_CTRL) > 0) != ctrl) return false;
      if (((event.key.keysym.mod & KMOD_ALT) > 0) != alt) return false;
      if (((event.key.keysym.mod & KMOD_SHIFT) > 0) != shift) return false;

      return true;
    }

    bool Shortcut::is_single_key() const
    {
      return (!alt && !ctrl && !shift) && !(keycode == NO_KEY) && keycode.symbol.size() == 1;
    }

    bool Shortcut::is_shortcut() const
    {
      return !(keycode == NO_KEY);
    }

    std::string Shortcut::to_string() const
    {
      std::string str = "";
      if (ctrl) str += "ctrl";
      if (alt)
      {
        if (!str.empty()) str += "+";
        str += "alt";
      }
      if (shift)
      {
        if (!str.empty()) str += "+";
        str += "shift";
      }
      if (!str.empty()) str += "+";
      str += keycode.symbol;
      return str;
    }

    const Shortcut Shortcut::NONE = Shortcut{NO_KEY};

    /*
     * KeyCode NULL-object
     */
    const KeyCode NO_KEY{0, ""};

    /*
     * SYMBOL_TO_KEYCODE map table
     */
    const std::map<std::string, SDL_Keycode> SYMBOL_TO_KEYCODE = {
      {"A", SDLK_a},
      {"B", SDLK_b},
      {"C", SDLK_c},
      {"D", SDLK_d},
      {"E", SDLK_e},
      {"F", SDLK_f},
      {"G", SDLK_g},
      {"H", SDLK_h},
      {"I", SDLK_i},
      {"J", SDLK_j},
      {"K", SDLK_k},
      {"L", SDLK_l},
      {"M", SDLK_m},
      {"N", SDLK_n},
      {"O", SDLK_o},
      {"P", SDLK_p},
      {"Q", SDLK_q},
      {"R", SDLK_r},
      {"S", SDLK_s},
      {"T", SDLK_t},
      {"U", SDLK_u},
      {"V", SDLK_v},
      {"W", SDLK_w},
      {"X", SDLK_x},
      {"Y", SDLK_y},
      {"Z", SDLK_z},
      {"1", SDLK_1},
      {"2", SDLK_2},
      {"3", SDLK_3},
      {"4", SDLK_4},
      {"5", SDLK_5},
      {"6", SDLK_6},
      {"7", SDLK_7},
      {"8", SDLK_8},
      {"9", SDLK_9},
      {"0", SDLK_0},
      {"F1", SDLK_F1},
      {"F2", SDLK_F2},
      {"F3", SDLK_F3},
      {"F4", SDLK_F4},
      {"F5", SDLK_F5},
      {"F6", SDLK_F6},
      {"F7", SDLK_F7},
      {"F8", SDLK_F8},
      {"F9", SDLK_F9},
      {"F10", SDLK_F10},
      {"F11", SDLK_F11},
      {"F12", SDLK_F12},
      {"left-ctrl", SDLK_LCTRL},
      {"right-ctrl", SDLK_RCTRL},
      {"left-shift", SDLK_LSHIFT},
      {"right-shift", SDLK_RSHIFT},
      {"left-alt", SDLK_LALT},
      {"right-alt", SDLK_RALT},
      {"left-super", SDLK_LGUI},
      {"right-super", SDLK_RGUI},
      {"space", SDLK_SPACE},
      {"enter", SDLK_RETURN},
      {"escape", SDLK_ESCAPE},
      {"left", SDLK_LEFT},
      {"right", SDLK_RIGHT},
      {"down", SDLK_DOWN},
      {"up", SDLK_UP},
      {"keypad-1", SDLK_KP_1},
      {"keypad-2", SDLK_KP_2},
      {"keypad-3", SDLK_KP_3},
      {"keypad-4", SDLK_KP_4},
      {"keypad-5", SDLK_KP_5},
      {"keypad-6", SDLK_KP_6},
      {"keypad-7", SDLK_KP_7},
      {"keypad-8", SDLK_KP_8},
      {"keypad-9", SDLK_KP_9}};

    /*!
     * @brief key_to_char - Translate keyboard event to char. Used for keyboard input.
     */
    std::unique_ptr<char> key_to_char(SDL_KeyboardEvent& key_event)
    {
      std::unique_ptr<char> typed_char = nullptr;

      if (key_event.keysym.mod & KMOD_SHIFT)
      {
        switch (key_event.keysym.sym)
        {
        case SDLK_0:
          return std::make_unique<char>('=');
        case SDLK_1:
          return std::make_unique<char>('!');
        case SDLK_2:
          return std::make_unique<char>('"');
        case SDLK_3:
          return std::make_unique<char>('#');
        case SDLK_5:
          return std::make_unique<char>('%');
        case SDLK_6:
          return std::make_unique<char>('&');
        case SDLK_7:
          return std::make_unique<char>('/');
        case SDLK_8:
          return std::make_unique<char>('(');
        case SDLK_9:
          return std::make_unique<char>(')');
        case SDLK_PERIOD:
          return std::make_unique<char>(':');
        case SDLK_COMMA:
          return std::make_unique<char>(';');
        case SDLK_MINUS:
          return std::make_unique<char>('_');
        case SDLK_PLUS:
          return std::make_unique<char>('?');
        case SDLK_QUOTE:
          return std::make_unique<char>('*');
        case SDLK_LESS:
          return std::make_unique<char>('>');
        }
      }

      if (key_event.keysym.mod & KMOD_ALT || key_event.keysym.mod & KMOD_RALT)
      {
        switch (key_event.keysym.sym)
        {
        case SDLK_4:
          return std::make_unique<char>('$');
        case SDLK_8:
          return std::make_unique<char>('[');
        case SDLK_9:
          return std::make_unique<char>(']');
        case SDLK_0:
          return std::make_unique<char>('}');
        case SDLK_7:
          return std::make_unique<char>('{');
        case SDLK_PLUS:
          return std::make_unique<char>('\\');
        }
      }

      switch (key_event.keysym.sym)
      {
      case SDLK_0:
        typed_char = std::make_unique<char>('0');
        break;
      case SDLK_1:
        typed_char = std::make_unique<char>('1');
        break;
      case SDLK_2:
        typed_char = std::make_unique<char>('2');
        break;
      case SDLK_3:
        typed_char = std::make_unique<char>('3');
        break;
      case SDLK_4:
        typed_char = std::make_unique<char>('4');
        break;
      case SDLK_5:
        typed_char = std::make_unique<char>('5');
        break;
      case SDLK_6:
        typed_char = std::make_unique<char>('6');
        break;
      case SDLK_7:
        typed_char = std::make_unique<char>('7');
        break;
      case SDLK_8:
        typed_char = std::make_unique<char>('8');
        break;
      case SDLK_9:
        typed_char = std::make_unique<char>('9');
        break;
      case SDLK_a:
        typed_char = std::make_unique<char>('a');
        break;
      case SDLK_b:
        typed_char = std::make_unique<char>('b');
        break;
      case SDLK_c:
        typed_char = std::make_unique<char>('c');
        break;
      case SDLK_d:
        typed_char = std::make_unique<char>('d');
        break;
      case SDLK_e:
        typed_char = std::make_unique<char>('e');
        break;
      case SDLK_f:
        typed_char = std::make_unique<char>('f');
        break;
      case SDLK_g:
        typed_char = std::make_unique<char>('g');
        break;
      case SDLK_h:
        typed_char = std::make_unique<char>('h');
        break;
      case SDLK_i:
        typed_char = std::make_unique<char>('i');
        break;
      case SDLK_j:
        typed_char = std::make_unique<char>('j');
        break;
      case SDLK_k:
        typed_char = std::make_unique<char>('k');
        break;
      case SDLK_l:
        typed_char = std::make_unique<char>('l');
        break;
      case SDLK_m:
        typed_char = std::make_unique<char>('m');
        break;
      case SDLK_n:
        typed_char = std::make_unique<char>('n');
        break;
      case SDLK_o:
        typed_char = std::make_unique<char>('o');
        break;
      case SDLK_p:
        typed_char = std::make_unique<char>('p');
        break;
      case SDLK_q:
        typed_char = std::make_unique<char>('q');
        break;
      case SDLK_r:
        typed_char = std::make_unique<char>('r');
        break;
      case SDLK_s:
        typed_char = std::make_unique<char>('s');
        break;
      case SDLK_t:
        typed_char = std::make_unique<char>('t');
        break;
      case SDLK_u:
        typed_char = std::make_unique<char>('u');
        break;
      case SDLK_v:
        typed_char = std::make_unique<char>('v');
        break;
      case SDLK_w:
        typed_char = std::make_unique<char>('w');
        break;
      case SDLK_x:
        typed_char = std::make_unique<char>('x');
        break;
      case SDLK_y:
        typed_char = std::make_unique<char>('y');
        break;
      case SDLK_z:
        typed_char = std::make_unique<char>('z');
        break;
      case SDLK_PLUS:
        typed_char = std::make_unique<char>('+');
        break;
      case SDLK_MINUS:
        typed_char = std::make_unique<char>('-');
        break;
      case SDLK_QUOTE:
        typed_char = std::make_unique<char>('\'');
        break;
      case SDLK_COMMA:
        typed_char = std::make_unique<char>(',');
        break;
      case SDLK_PERIOD:
        typed_char = std::make_unique<char>('.');
        break;
      case SDLK_LESS:
        typed_char = std::make_unique<char>('<');
        break;
      case SDLK_SPACE:
        typed_char = std::make_unique<char>(' ');
        break;
      }

      if (key_event.keysym.mod & KMOD_SHIFT)
      {
        if (typed_char)
        {
          *typed_char = toupper(*typed_char);
        }
      }

      return typed_char;
    }

    Lisple::sptr_rtval key_event_to_lisple_key(const SDL_KeyboardEvent& event)
    {
      for (auto& [str, keycode] : SYMBOL_TO_KEYCODE)
      {
        if (keycode == event.keysym.sym)
        {
          return Lisple::RTValue::keyword("key/" + str);
        }
      }

      return Lisple::Constant::NIL;
    }

  } // namespace Keyboard
} // namespace Pixils
