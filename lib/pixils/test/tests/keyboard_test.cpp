#include <pixils/keyboard.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <gtest/gtest.h>
#include <roo/runtime/value.h>

TEST(KeyboardTest, key_to_char_treats_mode_modifier_as_altgr)
{
  SDL_KeyboardEvent event{};
  event.key = SDLK_7;
  event.mod = SDL_KMOD_MODE;

  auto text = Pixils::Keyboard::key_to_char(event);

  ASSERT_NE(text, nullptr);
  EXPECT_EQ(*text, '{');
}

TEST(KeyboardTest, key_to_char_treats_right_alt_modifier_as_altgr)
{
  SDL_KeyboardEvent event{};
  event.key = SDLK_PLUS;
  event.mod = SDL_KMOD_RALT;

  auto text = Pixils::Keyboard::key_to_char(event);

  ASSERT_NE(text, nullptr);
  EXPECT_EQ(*text, '\\');
}

TEST(KeyboardTest, key_event_to_roo_key_maps_tab)
{
  SDL_KeyboardEvent event{};
  event.key = SDLK_TAB;

  auto key = Pixils::Keyboard::key_event_to_roo_key(event);

  EXPECT_EQ(key->to_string(), ":key/tab");
}
