#include <pixils/keyboard.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <gtest/gtest.h>
#include <roo/runtime/value.h>

TEST(KeyboardTest, key_to_char_treats_mode_modifier_as_altgr)
{
  SDL_KeyboardEvent event{};
  event.keysym.sym = SDLK_7;
  event.keysym.mod = KMOD_MODE;

  auto text = Pixils::Keyboard::key_to_char(event);

  ASSERT_NE(text, nullptr);
  EXPECT_EQ(*text, '{');
}

TEST(KeyboardTest, key_to_char_treats_right_alt_modifier_as_altgr)
{
  SDL_KeyboardEvent event{};
  event.keysym.sym = SDLK_PLUS;
  event.keysym.mod = KMOD_RALT;

  auto text = Pixils::Keyboard::key_to_char(event);

  ASSERT_NE(text, nullptr);
  EXPECT_EQ(*text, '\\');
}

TEST(KeyboardTest, key_event_to_roo_key_maps_tab)
{
  SDL_KeyboardEvent event{};
  event.keysym.sym = SDLK_TAB;

  auto key = Pixils::Keyboard::key_event_to_roo_key(event);

  EXPECT_EQ(key->to_string(), ":key/tab");
}
