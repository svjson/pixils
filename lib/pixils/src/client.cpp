
#include "pixils/console.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/client.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/keyboard.h>
#include <pixils/program.h>
#include <pixils/runtime/mode.h>

#include <SDL2/SDL_render.h>
#include <chrono>
#include <iostream>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>
#include <memory>

namespace Pixils
{
  long long now()
  {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
      .count();
  }

  Client::Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, bool init_mode)
    : lisple(lisple_runtime)
    , ctx(ctx)
    , assets(ctx)
    , session(
        lisple_runtime,
        assets,
        {Lisple::RTValue::object(Pixils::Script::FrameEventsAdapter::make_ref(this->events)),
         Lisple::RTValue::object(Pixils::Script::RenderContextAdapter::make_ref(this->ctx))})
  {
    ctx.asset_registry = &assets;
    init_console();

    if (init_mode)
    {
      Lisple::sptr_rtval programs =
        lisple_runtime.lookup_value(Script::ID__PIXILS__PROGRAMS);
      auto program_keys = Lisple::Dict::map_keys(*programs);
      Lisple::sptr_rtval program_key;

      if (program_keys.size() == 0)
      {
        lisple_runtime.eval("(pixils/defprogram program {})");
        program_key = Lisple::RTValue::symbol("program");
      }
      else
      {
        program_key = Lisple::RTValue::symbol(program_keys.front()->str());
      }

      auto program_val = Lisple::Dict::get_property(programs, program_key);

      Lisple::sptr_sobject prg_obj = std::get<Lisple::sptr_sobject>(program_val->value);

      auto& program = Lisple::obj<Program>(*program_val);

      this->program = &program;

      auto modes = lisple_runtime.lookup_value(Script::ID__PIXILS__MODES);

      if (program.initial_mode == "")
      {
        auto mode_keys = Lisple::Dict::map_keys(*modes);
        if (mode_keys.size() == 0)
        {
          throw Lisple::LispleException("No modes defined");
        }
        else
        {
          program.initial_mode = mode_keys.front()->str();
        }
      }

      session.push_mode(program.initial_mode, Lisple::Constant::NIL);
    }
  }

  Client::Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx)
    : Client(lisple_runtime, ctx, true)
  {
  }

  Client::Client(Lisple::Runtime& lisple_runtime,
                 RenderContext& ctx,
                 Runtime::Mode& root_mode)
    : Client(lisple_runtime, ctx, false)
  {
    session.push_mode(root_mode.name, Lisple::Constant::NIL);
  }

  void Client::init_console()
  {
    ctx.asset_registry->load("pixils",
                             {.images = {{"console-font", "assets/console_font.png"}}});

    this->console = std::make_unique<ConsoleOverlay>(
      ctx,
      lisple,
      ctx.asset_registry->get_image("pixils", "console-font"));
  }

  void Client::run()
  {
    this->ctx.prepare_frame(program->get_display());
    this->main_loop();
  }

  void Client::main_loop()
  {
    auto l_events = Pixils::Script::FrameEventsAdapter::make_ref(events);
    auto l_ctx = Pixils::Script::RenderContextAdapter::make_ref(ctx);

    //    SDL_Rect surface_rect{0, 0, ctx.buffer_dim.w, ctx.buffer_dim.h};
    SDL_Event event;

    bool quit = false;

    while (!quit)
    {
      long long frame_start = now();

      // surface_rect.w = ctx.buffer_dim.w;
      // surface_rect.h = ctx.buffer_dim.h;
      ctx.prepare_frame(program->get_display());

      events.key_down = Lisple::NIL;

      while (SDL_PollEvent(&event))
      {
        switch (event.type)
        {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN:
          handle_keydown(event.key);
          break;
        case SDL_KEYUP:
          handle_keyup(event.key);
          break;
        }
      }

      SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_NONE);
      SDL_SetRenderDrawColor(ctx.renderer, 0x00, 0x00, 0x00, 0xff);
      SDL_RenderClear(ctx.renderer);
      SDL_SetRenderDrawColor(ctx.renderer, 0xff, 0xff, 0xff, 0xff);

      session.update_mode();
      session.render_mode();

      ctx.flush_buffer(program->get_display());

      if (console->get_open_state() == ConsoleOverlay::State::OPEN ||
          console->get_open_state() == ConsoleOverlay::OPENING ||
          console->get_open_state() == ConsoleOverlay::State::CLOSING)
      {
        SDL_SetRenderTarget(ctx.renderer, nullptr);
        console->set_window_size(
          {ctx.window_rect.x, ctx.window_rect.y, ctx.window_rect.w, ctx.window_rect.h});
        console->tick();
        console->render(ctx);
      }

      ctx.finalize_frame();

      while (now() - frame_start < 25)
      {
      }

      session.process_messages();
    }
  }

  void Client::handle_keydown(SDL_KeyboardEvent& key_event)
  {
    switch (key_event.keysym.sym)
    {
    case SDLK_F10:
      if (this->console->get_open_state() == ConsoleOverlay::State::CLOSED ||
          this->console->get_open_state() == ConsoleOverlay::State::CLOSING)
      {
        this->console->open();
      }
      else
      {
        this->console->close();
      }
      break;
    default:
      if (this->console->get_open_state() == ConsoleOverlay::State::OPEN ||
          this->console->get_open_state() == ConsoleOverlay::State::CLOSING)
      {
        this->console->on_keydown(key_event);
      }
      else
      {
        events.do_key_down(key_event);
      }
      break;
    }
  }

  void Client::handle_keyup(SDL_KeyboardEvent& key_event)
  {

    events.do_key_up(key_event);
  }
} // namespace Pixils
