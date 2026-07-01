
#ifndef PIXILS__TEST__FIXTURE_H
#define PIXILS__TEST__FIXTURE_H

#include <pixils/context.h>
#include <pixils/script.h>

#include <SDL3/SDL_audio.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <gtest/gtest.h>
#include <roo/runtime.h>
#include <roo/runtime/value.h>
#include <utility>

class BaseFixture : public ::testing::Test
{
 protected:
  Pixils::RenderContext render_ctx{};
  Roo::Runtime runtime;

  BaseFixture()
    : BaseFixture(Pixils::RenderContext{
        nullptr,
        nullptr,
        MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)})
  {
  }

  explicit BaseFixture(Pixils::RenderContext initial_render_ctx)
    : render_ctx(std::move(initial_render_ctx))
    , runtime(Pixils::init_roo_runtime(this->render_ctx, "test", {}))
  {
  }
};

#endif
