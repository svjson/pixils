
#include <pixils/asset/registry.h>
#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/font_registry.h>
#include <pixils/geom.h>
#include <pixils/sdl_render.h>

#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_properties.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <algorithm>

namespace Pixils
{
  namespace
  {
    float clamped_audio_volume(float volume)
    {
      return std::clamp(volume, 0.0f, 1.0f);
    }

    void destroy_audio_resources(MIX_Mixer*& mixer,
                                 std::vector<MIX_Track*>& tracks,
                                 MIX_Track*& music_track,
                                 MIX_Track*& music_fadeout_track)
    {
      for (auto* track : tracks)
      {
        if (track) MIX_DestroyTrack(track);
      }
      tracks.clear();
      if (music_track) MIX_DestroyTrack(music_track);
      music_track = nullptr;
      if (music_fadeout_track) MIX_DestroyTrack(music_fadeout_track);
      music_fadeout_track = nullptr;
      if (mixer) MIX_DestroyMixer(mixer);
      mixer = nullptr;
    }
  } // namespace

  RenderContext::RenderContext() = default;

  RenderContext::RenderContext(SDL_Window* window, SDL_Renderer* renderer, MIX_Mixer* audio_mixer)
    : window(window)
    , renderer(renderer)
    , audio_mixer(audio_mixer)
  {
  }

  RenderContext::~RenderContext()
  {
    destroy_audio_resources(audio_mixer, audio_tracks, music_track, music_fadeout_track);
  }

  RenderContext::RenderContext(RenderContext&& other) noexcept
    : window(other.window)
    , renderer(other.renderer)
    , audio_mixer(other.audio_mixer)
    , audio_tracks(std::move(other.audio_tracks))
    , music_track(other.music_track)
    , music_fadeout_track(other.music_fadeout_track)
    , buffer_texture(other.buffer_texture)
    , current_render_target(other.current_render_target)
    , current_clip_rect(other.current_clip_rect)
    , buffer_dim(other.buffer_dim)
    , window_rect(other.window_rect)
    , application_rect(other.application_rect)
    , pixel_size(other.pixel_size)
    , tile_size(other.tile_size)
    , asset_registry(std::move(other.asset_registry))
    , font_registry(std::move(other.font_registry))
    , pointer_registry(std::move(other.pointer_registry))
    , enable_render_geometry(other.enable_render_geometry)
  {
    other.audio_mixer = nullptr;
    other.audio_tracks.clear();
    other.music_track = nullptr;
    other.music_fadeout_track = nullptr;
  }

  RenderContext& RenderContext::operator=(RenderContext&& other) noexcept
  {
    if (this == &other) return *this;

    destroy_audio_resources(audio_mixer, audio_tracks, music_track, music_fadeout_track);

    window = other.window;
    renderer = other.renderer;
    audio_mixer = other.audio_mixer;
    audio_tracks = std::move(other.audio_tracks);
    music_track = other.music_track;
    music_fadeout_track = other.music_fadeout_track;
    buffer_texture = other.buffer_texture;
    current_render_target = other.current_render_target;
    current_clip_rect = other.current_clip_rect;
    buffer_dim = other.buffer_dim;
    window_rect = other.window_rect;
    application_rect = other.application_rect;
    pixel_size = other.pixel_size;
    tile_size = other.tile_size;
    asset_registry = std::move(other.asset_registry);
    font_registry = std::move(other.font_registry);
    pointer_registry = std::move(other.pointer_registry);
    enable_render_geometry = other.enable_render_geometry;

    other.audio_mixer = nullptr;
    other.audio_tracks.clear();
    other.music_track = nullptr;
    other.music_fadeout_track = nullptr;
    return *this;
  }

  Dimension RenderContext::get_window_dimension()
  {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    return Dimension{w, h};
  }

  Rect RenderContext::application_target_rect(Display& display) const
  {
    SDL_Rect target{0, 0, buffer_dim.w, buffer_dim.h};

    if (target.w <= 0 || target.h <= 0)
    {
      return {0, 0, 0, 0};
    }

    if (display.scaling == Display::Scaling::STRETCH)
    {
      target.w = window_rect.w;
      target.h = window_rect.h;
    }
    else if (display.scaling == Display::Scaling::FIT || display.resolution.pixel_scale > 1)
    {
      int scale = std::min(window_rect.w / target.w, window_rect.h / target.h);
      if (scale < 1) scale = 1;
      target.w *= scale;
      target.h *= scale;
    }

    if (display.align == Display::Alignment::CENTER)
    {
      target.x = window_rect.w / 2 - target.w / 2;
      target.y = window_rect.h / 2 - target.h / 2;
    }

    return {target.x, target.y, target.w, target.h};
  }

  Point RenderContext::window_to_buffer_point(Display& display, int x, int y) const
  {
    Rect target = application_target_rect(display);
    if (target.w <= 0 || target.h <= 0 || buffer_dim.w <= 0 || buffer_dim.h <= 0)
    {
      return {static_cast<float>(x), static_cast<float>(y)};
    }

    return {static_cast<float>(x - target.x) *
              (static_cast<float>(buffer_dim.w) / static_cast<float>(target.w)),
            static_cast<float>(y - target.y) *
              (static_cast<float>(buffer_dim.h) / static_cast<float>(target.h))};
  }

  Point RenderContext::buffer_to_window_point(const Point& point) const
  {
    if (application_rect.w <= 0 || application_rect.h <= 0 || buffer_dim.w <= 0 ||
        buffer_dim.h <= 0)
    {
      return point;
    }

    return {static_cast<float>(application_rect.x) +
              point.x * (static_cast<float>(application_rect.w) /
                         static_cast<float>(buffer_dim.w)),
            static_cast<float>(application_rect.y) +
              point.y * (static_cast<float>(application_rect.h) /
                         static_cast<float>(buffer_dim.h))};
  }

  void RenderContext::warp_mouse_to_buffer_point(const Point& point)
  {
    if (!window)
    {
      return;
    }

    Point window_point = buffer_to_window_point(point).round();
    SDL_WarpMouseInWindow(window, window_point.round_x(), window_point.round_y());
  }

  void RenderContext::begin_frame(Display& display)
  {
    Color& bg = display.background;

    SDL_GetWindowSize(window, &window_rect.w, &window_rect.h);
    application_rect = application_target_rect(display);
    set_render_target(nullptr);
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 0xff);
    SDL_RenderClear(renderer);
  }

  void RenderContext::prepare_application_frame(Display& display)
  {
    if (display.resolution.mode == Resolution::Mode::AUTO)
    {
      int ps = display.resolution.pixel_scale;
      display.resolution.dimension = {window_rect.w / ps, window_rect.h / ps};
    }

    Dimension& target_buffer_dim = display.resolution.dimension;

    if (this->buffer_texture == nullptr)
    {
      buffer_dim = target_buffer_dim;
      create_and_target_buffer();
    }
    else if (target_buffer_dim != buffer_dim)
    {
      buffer_dim = target_buffer_dim;
      SDL_DestroyTexture(this->buffer_texture);
      create_and_target_buffer();
    }

    application_rect = application_target_rect(display);
    clear_buffer();
  }

  void RenderContext::clear_buffer()
  {
    SDL_SetTextureBlendMode(buffer_texture, SDL_BLENDMODE_BLEND);
    set_render_target(this->buffer_texture);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
  }

  void RenderContext::create_and_target_buffer()
  {
    this->buffer_texture = create_texture_nearest(this->renderer,
                                                  SDL_PIXELFORMAT_RGBA8888,
                                                  SDL_TEXTUREACCESS_TARGET,
                                                  buffer_dim.w,
                                                  buffer_dim.h);
    SDL_SetTextureBlendMode(buffer_texture, SDL_BLENDMODE_BLEND);
    set_render_target(this->buffer_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
  }

  void RenderContext::flush_buffer(Display& display)
  {
    set_render_target(nullptr);

    application_rect = application_target_rect(display);
    SDL_Rect target = application_rect.to_SDL_rect();

    SDL_FRect target_rect{static_cast<float>(target.x),
                          static_cast<float>(target.y),
                          static_cast<float>(target.w),
                          static_cast<float>(target.h)};
    SDL_RenderTexture(this->renderer, this->buffer_texture, nullptr, &target_rect);
  }

  void RenderContext::finalize_frame()
  {
    SDL_RenderPresent(renderer);
  }

  void RenderContext::set_render_target(SDL_Texture* target)
  {
    SDL_SetRenderTarget(renderer, target);
    current_render_target = target;
  }

  void RenderContext::set_clip_rect(std::optional<Rect> rect)
  {
    current_clip_rect = rect;
    if (!rect)
    {
      SDL_SetRenderClipRect(renderer, nullptr);
      return;
    }

    SDL_Rect sdl_rect = rect->to_SDL_rect();
    SDL_SetRenderClipRect(renderer, &sdl_rect);
  }

  int RenderContext::play_audio(MIX_Audio* audio, int channel, int loops, float volume)
  {
    if (!audio_mixer || !audio) return -1;

    constexpr int default_track_count = 8;
    if (audio_tracks.empty())
    {
      for (int i = 0; i < default_track_count; i++)
      {
        MIX_Track* track = MIX_CreateTrack(audio_mixer);
        if (!track) break;
        audio_tracks.push_back(track);
      }
    }

    int target_channel = channel;
    if (target_channel < 0)
    {
      target_channel = -1;
      for (int i = 0; i < static_cast<int>(audio_tracks.size()); i++)
      {
        if (!MIX_TrackPlaying(audio_tracks[i]))
        {
          target_channel = i;
          break;
        }
      }
    }

    if (target_channel < 0 || target_channel >= static_cast<int>(audio_tracks.size()))
    {
      return -1;
    }

    MIX_Track* track = audio_tracks[target_channel];
    MIX_StopTrack(track, 0);
    if (!MIX_SetTrackAudio(track, audio)) return -1;

    if (!MIX_SetTrackGain(track, clamped_audio_volume(volume))) return -1;

    SDL_PropertiesID props = SDL_CreateProperties();
    if (props)
    {
      SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
    }

    bool played = MIX_PlayTrack(track, props);
    if (props) SDL_DestroyProperties(props);
    return played ? target_channel : -1;
  }

  bool RenderContext::play_music(MIX_Audio* audio,
                                 int loops,
                                 float volume,
                                 int fade_in_ms,
                                 int crossfade_ms)
  {
    if (!audio_mixer || !audio) return false;

    if (!music_track)
    {
      music_track = MIX_CreateTrack(audio_mixer);
      if (!music_track) return false;
    }

    const int transition_ms = std::max(0, crossfade_ms);
    if (transition_ms > 0 && MIX_TrackPlaying(music_track))
    {
      if (!music_fadeout_track)
      {
        music_fadeout_track = MIX_CreateTrack(audio_mixer);
        if (!music_fadeout_track) return false;
      }
      if (MIX_TrackPlaying(music_fadeout_track))
      {
        MIX_StopTrack(music_fadeout_track, 0);
      }

      std::swap(music_track, music_fadeout_track);

      Sint64 fade_out_frames = MIX_TrackMSToFrames(music_fadeout_track, transition_ms);
      if (fade_out_frames < 0) fade_out_frames = 0;
      if (!MIX_StopTrack(music_fadeout_track, fade_out_frames)) return false;
      fade_in_ms = transition_ms;
    }
    else
    {
      if (music_fadeout_track) MIX_StopTrack(music_fadeout_track, 0);
      MIX_StopTrack(music_track, 0);
      if (transition_ms > 0) fade_in_ms = transition_ms;
    }

    if (!MIX_SetTrackAudio(music_track, audio)) return false;
    if (!MIX_SetTrackGain(music_track, clamped_audio_volume(volume))) return false;

    SDL_PropertiesID props = SDL_CreateProperties();
    if (props)
    {
      SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
      SDL_SetNumberProperty(props,
                            MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER,
                            std::max(0, fade_in_ms));
    }

    bool played = MIX_PlayTrack(music_track, props);
    if (props) SDL_DestroyProperties(props);
    return played;
  }

  bool RenderContext::stop_music(int fade_out_ms)
  {
    bool ok = true;
    auto stop_track = [fade_out_ms, &ok](MIX_Track* track)
    {
      if (!track) return;
      Sint64 fade_out_frames = 0;
      if (fade_out_ms > 0)
      {
        fade_out_frames = MIX_TrackMSToFrames(track, fade_out_ms);
        if (fade_out_frames < 0) fade_out_frames = 0;
      }
      ok = MIX_StopTrack(track, fade_out_frames) && ok;
    };

    stop_track(music_track);
    stop_track(music_fadeout_track);
    return ok;
  }

  bool RenderContext::pause_music()
  {
    bool ok = true;
    if (music_track) ok = MIX_PauseTrack(music_track) && ok;
    if (music_fadeout_track && MIX_TrackPlaying(music_fadeout_track))
    {
      ok = MIX_PauseTrack(music_fadeout_track) && ok;
    }
    return ok;
  }

  bool RenderContext::resume_music()
  {
    bool ok = true;
    if (music_track) ok = MIX_ResumeTrack(music_track) && ok;
    if (music_fadeout_track && MIX_TrackPlaying(music_fadeout_track))
    {
      ok = MIX_ResumeTrack(music_fadeout_track) && ok;
    }
    return ok;
  }

  bool RenderContext::set_music_volume(float volume)
  {
    if (!music_track) return true;
    return MIX_SetTrackGain(music_track, clamped_audio_volume(volume));
  }

  bool RenderContext::music_playing() const
  {
    return music_track && MIX_TrackPlaying(music_track);
  }
} // namespace Pixils
