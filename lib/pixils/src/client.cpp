
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>

#include <SDL2/SDL_render.h>
#include <chrono>
#include <lisple/runtime.h>

namespace Pixils
{
  long long now()
  {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  }

  void main_loop(Lisple::Runtime& lisple_runtime, RenderContext& ctx)
  {
    Pixils::FrameEvents events;
    bool quit = false;
    SDL_Rect surface_rect{0, 0, ctx.buffer_dim.w, ctx.buffer_dim.h};
    SDL_Event event;

    Lisple::sptr_sobject_v frame_args = {Pixils::Script::FrameEventsAdapter::make_ref(events),
                                         Pixils::Script::RenderContextAdapter::make_ref(ctx)};

    while (!quit)
    {
      long long frame_start = now();

      surface_rect.w = ctx.buffer_dim.w;
      surface_rect.h = ctx.buffer_dim.h;
      ctx.prepare_frame();

      events.key_down = Lisple::NIL;

      while (SDL_PollEvent(&event))
      {
        switch (event.type)
        {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN:
          events.do_key_down(event.key);
          break;
        case SDL_KEYUP:
          events.do_key_up(event.key);
          break;
        }
      }

      SDL_SetRenderDrawColor(ctx.renderer, 0x00, 0x00, 0x00, 0xff);
      SDL_RenderFillRect(ctx.renderer, &surface_rect);

      SDL_SetRenderDrawColor(ctx.renderer, 0xff, 0xff, 0xff, 0xff);

      lisple_runtime.call_fn("asteroids/frame-fn", frame_args);

      ctx.flush_buffer();

      ctx.finalize_frame();

      while (now() - frame_start < 25)
      {
      }
    }
  }

} // namespace Pixils
