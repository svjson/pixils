
#include <pixils/binding/pixils_namespace.h>
#include <pixils/client.h>
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

  Client::Client(Lisple::Runtime& lisple_runtime,
                 RenderContext& ctx,
                 Runtime::Mode& root_mode)
    : lisple(lisple_runtime)
    , ctx(ctx)
    , mode_stack(lisple.lookup(Script::ID__PIXILS__MODE_STACK)->as<Lisple::Array>())
    , hook_args({Pixils::Script::FrameEventsAdapter::make_ref(this->events),
                 Pixils::Script::RenderContextAdapter::make_ref(this->ctx)})
  {
    this->mode_stack.append(Script::ModeAdapter::make_ref(root_mode));
  }

  void Client::run()
  {
    this->ctx.prepare_frame();
    this->activate_mode();
    this->main_loop();
  }

  void Client::activate_mode()
  {
    bool pushed =
      static_cast<int>(mode_stack.get_children().size()) > active_mode.mode_index + 1;

    Runtime::Mode& mode =
      mode_stack.get_children().back()->as<Script::ModeAdapter>().get_object();

    this->active_mode.mode_index = mode_stack.size() - 1;
    this->active_mode.render_fun = mode.render->to_string();
    this->active_mode.update_fun = mode.update->to_string();
    this->active_mode.init_fun = mode.init->to_string();

    if (pushed) this->lisple.call_fn(this->active_mode.init_fun, this->hook_args.init_args);
  }

  void Client::main_loop()
  {
    auto l_events = Pixils::Script::FrameEventsAdapter::make_ref(events);
    auto l_ctx = Pixils::Script::RenderContextAdapter::make_ref(ctx);

    SDL_Rect surface_rect{0, 0, ctx.buffer_dim.w, ctx.buffer_dim.h};
    SDL_Event event;

    bool quit = false;

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

      this->lisple.call_fn(this->active_mode.update_fun, this->hook_args.update_args);

      if (static_cast<int>(mode_stack.size()) - 1 != this->active_mode.mode_index)
      {
        this->activate_mode();
      }

      lisple.call_fn(this->active_mode.render_fun, this->hook_args.render_args);

      ctx.flush_buffer();
      ctx.finalize_frame();

      while (now() - frame_start < 25)
      {
      }
    }
  }
} // namespace Pixils
