
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>
#include <pixils/runtime/mode.h>

#include <SDL2/SDL_render.h>
#include <chrono>
#include <lisple/runtime.h>

namespace Pixils
{
  long long now()
  {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
        .count();
  }

  void main_loop(Lisple::Runtime& lisple_runtime, Runtime::Mode& mode, RenderContext& ctx)
  {
    auto root_mode = Script::ModeAdapter::make_ref(mode);
    Lisple::Array& mode_stack =
        lisple_runtime.lookup(Script::ID__PIXILS__MODE_STACK)->as<Lisple::Array>();
    mode_stack.append(root_mode);

    Runtime::Mode* current_mode = &mode;
    size_t current_mode_index = 0;

    ctx.prepare_frame();
    Pixils::FrameEvents events;
    bool quit = false;
    SDL_Rect surface_rect{0, 0, ctx.buffer_dim.w, ctx.buffer_dim.h};

    SDL_Event event;

    std::string render_fun = current_mode->render->to_string();
    std::string update_fun = current_mode->update->to_string();
    std::string init_fun = current_mode->init->to_string();

    auto l_events = Pixils::Script::FrameEventsAdapter::make_ref(events);
    auto l_ctx = Pixils::Script::RenderContextAdapter::make_ref(ctx);

    Lisple::sptr_sobject_v init_args = {l_ctx};
    Lisple::sptr_sobject_v update_args = {l_events, l_ctx};
    Lisple::sptr_sobject_v render_args = {l_ctx};

    lisple_runtime.call_fn(init_fun, init_args);

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

      lisple_runtime.call_fn(update_fun, update_args);

      if (mode_stack.size() - 1 != current_mode_index)
      {
        current_mode =
            &mode_stack.get_children().back()->as<Script::ModeAdapter>().get_object();
        current_mode_index = mode_stack.get_children().size() - 1;

        render_fun = current_mode->render->to_string();
        update_fun = current_mode->update->to_string();
        init_fun = current_mode->init->to_string();
      }

      lisple_runtime.call_fn(render_fun, render_args);

      ctx.flush_buffer();

      ctx.finalize_frame();

      while (now() - frame_start < 25)
      {
      }
    }
  }

} // namespace Pixils
