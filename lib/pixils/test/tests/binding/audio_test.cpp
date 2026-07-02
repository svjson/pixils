#include "../fixture.h"

#include <pixils/asset/registry.h>

#include <gtest/gtest.h>
#include <sdl3_mock/mock_resources.h>

class AudioTest : public BaseFixture
{
 protected:
  void TearDown() override { SDL3Mock::reset_mocks(); }
};

TEST_F(AudioTest, defbundle_declares_sounds_that_can_be_loaded_on_demand)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./laser.wav");
  runtime.eval("(pixils/defbundle sfx {:sounds {:laser \"laser.wav\"}})");

  // When
  MIX_Audio* audio = render_ctx.asset_registry->get_sound("sfx", "laser");

  // Then
  EXPECT_NE(audio, nullptr);
  EXPECT_EQ(SDL3Mock::created_audio.size(), 1u);
}

TEST_F(AudioTest, play_bang_plays_sound_from_qualified_keyword)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./laser.wav");
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
  SDL3Mock::prepared_wave_audio.insert("./laser.wav");
  runtime.eval("(pixils/defbundle sfx {:sounds {:laser \"laser.wav\"}})");

  // When
  auto result = runtime.eval("(pixils.audio/play! :sfx/laser {:volume 0.35})");

  // Then
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->num().get_int(), 0);
}

TEST_F(AudioTest, defbundle_declares_music_loaded_without_predecoding)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./theme.mp3");
  runtime.eval("(pixils/defbundle soundtrack {:music {:theme \"theme.mp3\"}})");

  // When
  MIX_Audio* audio = render_ctx.asset_registry->get_music("soundtrack", "theme");

  // Then
  ASSERT_NE(audio, nullptr);
  EXPECT_EQ(audio->file_name, "./theme.mp3");
  EXPECT_FALSE(audio->predecoded);
}

TEST_F(AudioTest, play_music_bang_plays_music_on_managed_track)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./theme.mp3");
  runtime.eval("(pixils/defbundle soundtrack {:music {:theme \"theme.mp3\"}})");

  // When
  auto result =
    runtime.eval("(pixils.audio/play-music! :soundtrack/theme {:volume 0.4})");

  // Then
  EXPECT_EQ(result, Roo::Constant::BOOL_TRUE);
  ASSERT_NE(render_ctx.music_track, nullptr);
  EXPECT_TRUE(render_ctx.music_track->playing);
  EXPECT_FLOAT_EQ(render_ctx.music_track->gain, 0.4f);
}

TEST_F(AudioTest, set_music_volume_clamps_current_music_track_gain)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./theme.mp3");
  runtime.eval("(pixils/defbundle soundtrack {:music {:theme \"theme.mp3\"}})");
  runtime.eval("(pixils.audio/play-music! :soundtrack/theme)");

  // When
  auto result = runtime.eval("(pixils.audio/set-music-volume! 1.5)");

  // Then
  EXPECT_EQ(result, Roo::Constant::BOOL_TRUE);
  ASSERT_NE(render_ctx.music_track, nullptr);
  EXPECT_FLOAT_EQ(render_ctx.music_track->gain, 1.0f);
}

TEST_F(AudioTest, stop_music_bang_accepts_fade_out)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./theme.mp3");
  runtime.eval("(pixils/defbundle soundtrack {:music {:theme \"theme.mp3\"}})");
  runtime.eval("(pixils.audio/play-music! :soundtrack/theme)");

  // When
  auto result = runtime.eval("(pixils.audio/stop-music! {:fade-out-ms 750})");

  // Then
  EXPECT_EQ(result, Roo::Constant::BOOL_TRUE);
  ASSERT_NE(render_ctx.music_track, nullptr);
  EXPECT_FALSE(render_ctx.music_track->playing);
  EXPECT_EQ(render_ctx.music_track->fade_out_frames, 750);
}

TEST_F(AudioTest, music_playing_reports_current_music_track_state)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./theme.mp3");
  runtime.eval("(pixils/defbundle soundtrack {:music {:theme \"theme.mp3\"}})");

  // Then
  EXPECT_EQ(runtime.eval("(pixils.audio/music-playing?)"), Roo::Constant::BOOL_FALSE);

  // When
  runtime.eval("(pixils.audio/play-music! :soundtrack/theme)");

  // Then
  EXPECT_EQ(runtime.eval("(pixils.audio/music-playing?)"), Roo::Constant::BOOL_TRUE);
}

TEST_F(AudioTest, pause_and_resume_music_control_managed_track)
{
  // Given
  SDL3Mock::prepared_wave_audio.insert("./theme.mp3");
  runtime.eval("(pixils/defbundle soundtrack {:music {:theme \"theme.mp3\"}})");
  runtime.eval("(pixils.audio/play-music! :soundtrack/theme)");

  // When
  auto pause_result = runtime.eval("(pixils.audio/pause-music!)");

  // Then
  EXPECT_EQ(pause_result, Roo::Constant::BOOL_TRUE);
  ASSERT_NE(render_ctx.music_track, nullptr);
  EXPECT_TRUE(render_ctx.music_track->paused);

  // When
  auto resume_result = runtime.eval("(pixils.audio/resume-music!)");

  // Then
  EXPECT_EQ(resume_result, Roo::Constant::BOOL_TRUE);
  EXPECT_FALSE(render_ctx.music_track->paused);
}
