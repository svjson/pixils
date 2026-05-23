#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_keycode.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest, menu_shortcuts_dispatch_from_main_mode_action_map)
{
  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  session.update_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_NE(session.active_mode->state->to_string().find(":show-grid? true"),
            std::string::npos);
  EXPECT_NE(session.active_mode->state->to_string().find(":show-terrain-rules? true"),
            std::string::npos);
  EXPECT_NE(session.active_mode->state->to_string().find(":terrain-rule-application :preview"),
            std::string::npos);

  input().key_down(SDLK_LCTRL);
  input().clear_transients();
  input().key_down(SDLK_g);
  session.update_mode();
  input().key_up(SDLK_g);
  input().clear_transients();

  EXPECT_NE(session.active_mode->state->to_string().find(":show-grid? false"),
            std::string::npos);

  input().key_down(SDLK_t);
  session.update_mode();
  input().key_up(SDLK_t);
  input().clear_transients();

  EXPECT_NE(session.active_mode->state->to_string().find(":show-terrain-rules? false"),
            std::string::npos);

  input().key_down(SDLK_LALT);
  input().clear_transients();
  input().key_down(SDLK_b);
  session.update_mode();
  input().key_up(SDLK_b);
  input().clear_transients();

  EXPECT_NE(
    session.active_mode->state->to_string().find(":terrain-rule-application :paint-baked"),
    std::string::npos);

  input().key_down(SDLK_p);
  session.update_mode();
  input().key_up(SDLK_p);
  input().key_up(SDLK_LALT);
  input().key_up(SDLK_LCTRL);
  session.update_mode();

  EXPECT_NE(session.active_mode->state->to_string().find(":terrain-rule-application :preview"),
            std::string::npos);
}
