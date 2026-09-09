#include <pixils/init_sdl.h>

#include <gtest/gtest.h>

#if defined(PIXILS_SDL_LIFECYCLE_WRAP)

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <sdl3_mock/mock_resources.h>
#include <vector>

namespace
{
  enum class SDLEvent
  {
    NONE,
    INIT,
    CREATE_WINDOW,
    CREATE_RENDERER,
    MIX_INIT,
    CREATE_MIXER,
    UNBIND_RENDER_TARGET,
    DESTROY_TRACK,
    DESTROY_TEXTURE,
    DESTROY_MIXER,
    DESTROY_RENDERER,
    DESTROY_WINDOW,
    MIX_QUIT,
    QUIT,
    QUIT_SUBSYSTEM
  };

  bool record_events = false;
  SDLEvent failure = SDLEvent::NONE;
  std::vector<SDLEvent> events;

  void record(SDLEvent event)
  {
    if (record_events) events.push_back(event);
  }

  class SDLSessionTest : public ::testing::Test
  {
   protected:
    void TearDown() override
    {
      record_events = false;
      failure = SDLEvent::NONE;
      events.clear();
      SDL3Mock::reset_mocks();
    }
  };
} // namespace

extern "C"
{
  bool __real_SDL_Init(SDL_InitFlags flags);
  SDL_Window* __real_SDL_CreateWindow(const char* title,
                                      int width,
                                      int height,
                                      SDL_WindowFlags flags);
  SDL_Renderer* __real_SDL_CreateRenderer(SDL_Window* window, const char* name);
  bool __real_SDL_SetRenderTarget(SDL_Renderer* renderer, SDL_Texture* texture);
  void __real_SDL_DestroyTexture(SDL_Texture* texture);
  void __real_SDL_DestroyRenderer(SDL_Renderer* renderer);
  void __real_SDL_DestroyWindow(SDL_Window* window);
  void __real_SDL_Quit();
  bool __real_MIX_Init();
  MIX_Mixer* __real_MIX_CreateMixerDevice(SDL_AudioDeviceID device,
                                          const SDL_AudioSpec* spec);
  void __real_MIX_DestroyTrack(MIX_Track* track);
  void __real_MIX_DestroyMixer(MIX_Mixer* mixer);
  void __real_MIX_Quit();

  bool __wrap_SDL_Init(SDL_InitFlags flags)
  {
    record(SDLEvent::INIT);
    if (failure == SDLEvent::INIT) return false;
    return __real_SDL_Init(flags);
  }

  SDL_Window* __wrap_SDL_CreateWindow(const char* title,
                                      int width,
                                      int height,
                                      SDL_WindowFlags flags)
  {
    record(SDLEvent::CREATE_WINDOW);
    if (failure == SDLEvent::CREATE_WINDOW) return nullptr;
    return __real_SDL_CreateWindow(title, width, height, flags);
  }

  SDL_Renderer* __wrap_SDL_CreateRenderer(SDL_Window* window, const char* name)
  {
    record(SDLEvent::CREATE_RENDERER);
    if (failure == SDLEvent::CREATE_RENDERER) return nullptr;
    return __real_SDL_CreateRenderer(window, name);
  }

  bool __wrap_SDL_SetRenderTarget(SDL_Renderer* renderer, SDL_Texture* texture)
  {
    if (!texture) record(SDLEvent::UNBIND_RENDER_TARGET);
    return __real_SDL_SetRenderTarget(renderer, texture);
  }

  void __wrap_SDL_DestroyTexture(SDL_Texture* texture)
  {
    record(SDLEvent::DESTROY_TEXTURE);
    __real_SDL_DestroyTexture(texture);
  }

  void __wrap_SDL_DestroyRenderer(SDL_Renderer* renderer)
  {
    record(SDLEvent::DESTROY_RENDERER);
    __real_SDL_DestroyRenderer(renderer);
  }

  void __wrap_SDL_DestroyWindow(SDL_Window* window)
  {
    record(SDLEvent::DESTROY_WINDOW);
    __real_SDL_DestroyWindow(window);
  }

  void __wrap_SDL_Quit()
  {
    record(SDLEvent::QUIT);
    __real_SDL_Quit();
  }

  void __wrap_SDL_QuitSubSystem([[maybe_unused]] SDL_InitFlags flags)
  {
    record(SDLEvent::QUIT_SUBSYSTEM);
  }

  bool __wrap_MIX_Init()
  {
    record(SDLEvent::MIX_INIT);
    if (failure == SDLEvent::MIX_INIT) return false;
    return __real_MIX_Init();
  }

  MIX_Mixer* __wrap_MIX_CreateMixerDevice(SDL_AudioDeviceID device,
                                          const SDL_AudioSpec* spec)
  {
    record(SDLEvent::CREATE_MIXER);
    if (failure == SDLEvent::CREATE_MIXER) return nullptr;
    return __real_MIX_CreateMixerDevice(device, spec);
  }

  void __wrap_MIX_DestroyTrack(MIX_Track* track)
  {
    record(SDLEvent::DESTROY_TRACK);
    __real_MIX_DestroyTrack(track);
  }

  void __wrap_MIX_DestroyMixer(MIX_Mixer* mixer)
  {
    record(SDLEvent::DESTROY_MIXER);
    __real_MIX_DestroyMixer(mixer);
  }

  void __wrap_MIX_Quit()
  {
    record(SDLEvent::MIX_QUIT);
    __real_MIX_Quit();
  }
}

TEST_F(SDLSessionTest, process_lifetime_releases_children_before_parent_resources)
{
  auto sdl = Pixils::init_sdl("SDL lifecycle test");
  ASSERT_NE(sdl, nullptr);

  auto& ctx = sdl->render_context();
  ctx.buffer_texture = SDL_CreateTexture(ctx.renderer,
                                         SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         16,
                                         16);
  ctx.set_render_target(ctx.buffer_texture);
  ctx.audio_tracks.push_back(MIX_CreateTrack(ctx.audio_mixer));

  events.clear();
  record_events = true;
  sdl.reset();
  record_events = false;

  EXPECT_EQ(events,
            (std::vector<SDLEvent>{SDLEvent::UNBIND_RENDER_TARGET,
                                   SDLEvent::DESTROY_TRACK,
                                   SDLEvent::DESTROY_TEXTURE,
                                   SDLEvent::DESTROY_MIXER,
                                   SDLEvent::DESTROY_RENDERER,
                                   SDLEvent::DESTROY_WINDOW,
                                   SDLEvent::MIX_QUIT,
                                   SDLEvent::QUIT}));
}

TEST_F(SDLSessionTest, hosted_lifetime_balances_subsystems_without_global_quit)
{
  auto sdl = Pixils::init_sdl("SDL lifecycle test", Pixils::SDLLifetime::HOSTED);
  ASSERT_NE(sdl, nullptr);

  events.clear();
  record_events = true;
  sdl.reset();
  record_events = false;

  EXPECT_EQ(events,
            (std::vector<SDLEvent>{SDLEvent::UNBIND_RENDER_TARGET,
                                   SDLEvent::DESTROY_MIXER,
                                   SDLEvent::DESTROY_RENDERER,
                                   SDLEvent::DESTROY_WINDOW,
                                   SDLEvent::MIX_QUIT,
                                   SDLEvent::QUIT_SUBSYSTEM}));
}

TEST_F(SDLSessionTest, partial_initialization_releases_each_completed_stage)
{
  struct FailureCase
  {
    SDLEvent failure;
    std::vector<SDLEvent> expected;
  };

  const std::vector<FailureCase> cases{
    {SDLEvent::INIT, {SDLEvent::INIT, SDLEvent::QUIT}},
    {SDLEvent::CREATE_WINDOW, {SDLEvent::INIT, SDLEvent::CREATE_WINDOW, SDLEvent::QUIT}},
    {SDLEvent::CREATE_RENDERER,
     {SDLEvent::INIT,
      SDLEvent::CREATE_WINDOW,
      SDLEvent::CREATE_RENDERER,
      SDLEvent::DESTROY_WINDOW,
      SDLEvent::QUIT}},
    {SDLEvent::MIX_INIT,
     {SDLEvent::INIT,
      SDLEvent::CREATE_WINDOW,
      SDLEvent::CREATE_RENDERER,
      SDLEvent::MIX_INIT,
      SDLEvent::UNBIND_RENDER_TARGET,
      SDLEvent::DESTROY_RENDERER,
      SDLEvent::DESTROY_WINDOW,
      SDLEvent::QUIT}},
    {SDLEvent::CREATE_MIXER,
     {SDLEvent::INIT,
      SDLEvent::CREATE_WINDOW,
      SDLEvent::CREATE_RENDERER,
      SDLEvent::MIX_INIT,
      SDLEvent::CREATE_MIXER,
      SDLEvent::UNBIND_RENDER_TARGET,
      SDLEvent::DESTROY_RENDERER,
      SDLEvent::DESTROY_WINDOW,
      SDLEvent::MIX_QUIT,
      SDLEvent::QUIT}},
  };

  testing::internal::CaptureStderr();
  for (const auto& failure_case : cases)
  {
    SCOPED_TRACE(static_cast<int>(failure_case.failure));
    events.clear();
    failure = failure_case.failure;
    record_events = true;
    auto sdl = Pixils::init_sdl("SDL lifecycle test");
    record_events = false;

    EXPECT_EQ(sdl, nullptr);
    EXPECT_EQ(events, failure_case.expected);
    SDL3Mock::reset_mocks();
  }
  testing::internal::GetCapturedStderr();
}

#endif /* PIXILS_SDL_LIFECYCLE_WRAP */
