
#ifndef __KEYBOARD_H_
#define __KEYBOARD_H_

#include <SDL2/SDL_events.h>
#include <cstdint>
#include <lisple/type.h>
#include <map>
#include <memory>
#include <string>

typedef int32_t SDL_Keycode;
typedef union SDL_Event SDL_Event;

namespace Pixils
{
  /*!
   * @brief Namespace for general keyboard-related functionality, chiefly
   * translating SDL2 keyboard events to char/string and an object
   * model for keyboard shortcuts
   */
  namespace Keyboard
  {
    /*!
     * @brief Represents a keycode and its corresponding symbol.
     *
     * This struct encapsulates an SDL keycode and its visual representation
     * as a human-readable symbol, such as "A", "Shift", etc.
     */
    struct KeyCode
    {
      /*!
       * @brief The SDL keycode representing the key.
       *
       * This member holds the actual keycode provided by the SDL library,
       * which corresponds to the key pressed on the keyboard.
       */
      SDL_Keycode code;

      /*!
       * @brief The human-readable symbol of the key.
       *
       * This member contains a string representation of the key,
       * like "A" for the 'A' key or "Shift" for the Shift key.
       */
      std::string symbol;

      /*!
       * @brief Equality operator for comparing KeyCode objects.
       *
       * Compares the keycode and symbol of two KeyCode objects to check for
       * equality.
       *
       * @param other Another KeyCode object to compare against.
       * @return true if both keycode and symbol are equal, false otherwise.
       */
      bool operator==(const KeyCode& other) const;
    };

    /*!
     * @brief A constant representing the absence of a key.
     *
     * This constant is used to indicate that no key is selected or assigned.
     * It serves as a null object for cases where a valid KeyCode is not present.
     */
    extern const KeyCode NO_KEY;

    /*!
     * @brief Represents a keyboard shortcut, including modifier keys.
     *
     * This struct encapsulates a keyboard shortcut, consisting of a primary key
     * (represented by a KeyCode) and optional modifier keys (Alt, Ctrl, and Shift).
     */
    struct Shortcut
    {
      /*!
       * @brief The primary keycode of the shortcut.
       *
       * This member represents the main key for the shortcut, such as 'A',
       * an arrow key, or function key.
       */
      KeyCode keycode;

      /*!
       * @brief Indicates whether the Alt key is part of the shortcut.
       *
       * If true, the Alt key is required for the shortcut.
       * Default is false.
       */
      bool alt = false;

      /*!
       * @brief Indicates whether the Ctrl key is part of the shortcut.
       *
       * If true, the Ctrl key is required for the shortcut.
       * Default is false.
       */
      bool ctrl = false;

      /*!
       * @brief Indicates whether the Shift key is part of the shortcut.
       *
       * If true, the Shift key is required for the shortcut.
       * Default is false.
       */
      bool shift = false;

      /*!
       * @brief Checks if the shortcut is a single key press without modifiers.
       *
       * @return true if the shortcut consists of only a single key and no modifiers.
       */
      bool is_single_key() const;

      /*!
       * @brief Checks if the shortcut is an actual shortcut that is bound to a key
       * (as opposed to NO_KEY)
       *
       * @return true if the instance contains a valid KeyCode.
       */
      bool is_shortcut() const;

      /*!
       * @brief A static constant representing an empty shortcut.
       *
       * This constant represents a shortcut with no keycode or modifiers, effectively
       * "none".
       */
      static const Shortcut NONE;

      /*!
       * @brief Checks if the shortcut matches a given SDL event.
       *
       * This function compares the current shortcut against an SDL event to see
       * if the key and modifier combination matches.
       *
       * @param event The SDL event to match against.
       * @return true if the event matches the shortcut, false otherwise.
       */
      bool matches(const SDL_Event& event) const;

      /*!
       * @brief Equality operator for comparing Shortcut objects.
       *
       * Compares the keycode and modifier keys (Alt, Ctrl, Shift) of two Shortcut objects.
       *
       * @param other Another Shortcut object to compare against.
       * @return true if both the keycode and modifiers match, false otherwise.
       */
      bool operator==(const Shortcut& other) const;

      /*!
       * @brief Converts the shortcut to a string representation.
       *
       * This function provides a human-readable string representation of the shortcut,
       * displaying the key and any modifier keys, suitable for output in a user
       * interface
       *
       * @return A string representing the shortcut, such as "Ctrl+Shift+A".
       */
      std::string to_string() const;
    };

    extern const std::map<std::string, SDL_Keycode> SYMBOL_TO_KEYCODE;

    std::unique_ptr<char> key_to_char(SDL_KeyboardEvent& key_event);

    Lisple::sptr_rtval key_event_to_lisple_key(const SDL_KeyboardEvent& event);
  } // namespace Keyboard

} // namespace Pixils

#endif
