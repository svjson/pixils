#include "../fixture.h"

#include <pixils/asset/registry.h>

#include <gtest/gtest.h>
#include <sdl2_mock/mock_resources.h>

class AudioTest : public BaseFixture
{
 protected:
  void TearDown() override { SDLMock::reset_mocks(); }
};

TEST_F(AudioTest, defbundle_declares_sounds_that_can_be_loaded_on_demand)
{
  // Given
  SDLMock::prepared_wave_audio.insert("./laser.wav");
  runtime.eval("(pixils/defbundle sfx {:sounds {:laser \"laser.wav\"}})");

  // When
  Mix_Chunk* chunk = render_ctx.asset_registry->get_sound("sfx", "laser");

  // Then
  EXPECT_NE(chunk, nullptr);
  EXPECT_EQ(SDLMock::created_mix_chunks.size(), 1u);
}

TEST_F(AudioTest, play_bang_plays_sound_from_qualified_keyword)
{
  // Given
  SDLMock::prepared_wave_audio.insert("./laser.wav");
  runtime.eval("(pixils/defbundle sfx {:sounds {:laser \"laser.wav\"}})");

  // When
  auto result = runtime.eval("(pixils.audio/play! :sfx/laser)");

  // Then
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->num().get_int(), 0);
}

TEST_F(AudioTest, play_bang_accepts_per_invocation_volume)
{
  // Given
  SDLMock::prepared_wave_audio.insert("./laser.wav");
  runtime.eval("(pixils/defbundle sfx {:sounds {:laser \"laser.wav\"}})");

  // When
  auto result = runtime.eval("(pixils.audio/play! :sfx/laser {:volume 0.35})");

  // Then
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->num().get_int(), 0);
}
