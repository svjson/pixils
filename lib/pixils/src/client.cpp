
#include "pixils/client.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/console.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/keyboard.h>
#include <pixils/program.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <algorithm>
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

  int frame_budget_ms(const Program& program)
  {
    if (program.target_frame_rate <= 0) return 0;
    return std::max(1, 1000 / program.target_frame_rate);
  }

  Client::Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, bool init_mode)
    : lisple(lisple_runtime)
    , ctx(ctx)
    , hook_ctx{&this->events, &this->ctx}
    , session(lisple_runtime,
              *ctx.asset_registry,
              ctx,
              {Pixils::Script::HookContextAdapter::make_ref(this->hook_ctx)})
  {
    session.hook_args.events = &this->events;
    init_console();

    if (init_mode)
    {
      this->program = &Pixils::load_program(lisple_runtime, session);
      SDL_ShowCursor(this->program->pointer_visible ? SDL_ENABLE : SDL_DISABLE);
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

  Client::~Client()
  {
    for (auto& [_, cursor] : cursor_cache)
    {
      if (cursor) SDL_FreeCursor(cursor);
    }
  }

  void Client::init_console()
  {
    ctx.asset_registry->load_embedded_assets();

    SDL_Texture* console_font_texture =
      ctx.asset_registry->get_image("pixils", "console-font");
    SDL_Texture* console_font_tint =
      ctx.asset_registry->get_tint_mask("pixils", "console-font");

    ctx.font_registry->register_font("font/console",
                                     console_font_texture,
                                     console_font_tint,
                                     console_font_map);

    this->console = std::make_unique<ConsoleOverlay>(ctx, lisple, console_font_texture);
  }

  void Client::run()
  {
    this->ctx.prepare_application_frame(program->get_display());
    this->main_loop();
  }

  void Client::main_loop()
  {
    SDL_Event event;

    bool quit = false;
    bool error_state = false;

    [[maybe_unused]] long frame = 0;

    while (!quit)
    {
      long long frame_start = now();

      ctx.begin_frame(program->get_display());

      events.key_down = Lisple::Constant::NIL;
      events.key_up = Lisple::Constant::NIL;
      events.mouse_button_down = Lisple::Constant::NIL;
      events.mouse_button_up = Lisple::Constant::NIL;
      events.mouse_moved = false;

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
        case SDL_MOUSEMOTION:
        {
          Point pos = ctx.window_to_buffer_point(program->get_display(),
                                                 event.motion.x,
                                                 event.motion.y);
          events.do_mouse_motion(pos.round_x(), pos.round_y());
          break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
          Point pos = ctx.window_to_buffer_point(program->get_display(),
                                                 event.button.x,
                                                 event.button.y);
          events.do_mouse_motion(pos.round_x(), pos.round_y());
          events.do_mouse_button_down(event.button);
          break;
        }
        case SDL_MOUSEBUTTONUP:
        {
          Point pos = ctx.window_to_buffer_point(program->get_display(),
                                                 event.button.x,
                                                 event.button.y);
          events.do_mouse_motion(pos.round_x(), pos.round_y());
          events.do_mouse_button_up(event.button);
          break;
        }
        }
      }
      frame++;

      if (error_state)
      {
        ctx.flush_buffer(program->get_display());
      }
      else
      {
        try
        {
          ctx.prepare_application_frame(program->get_display());

          session.update_mode();
          update_cursor();
          session.render_mode();
        }
        catch (std::exception& e)
        {
          std::cout << "ERROR: " << e.what() << std::endl;
          error_state = true;
        }
      }

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

      int target_frame_ms = frame_budget_ms(*program);
      [[maybe_unused]] int frame_margin = target_frame_ms - (now() - frame_start);

      // std::cout << "frame #" << frame << " - margin: " << frame_margin << std::endl;

      while (target_frame_ms > 0 && now() - frame_start < target_frame_ms)
      {
      }

      session.process_messages();
      quit = quit || session.quit_requested;
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

  SDL_Cursor* Client::system_cursor(UI::Style::Cursor cursor)
  {
    if (auto it = cursor_cache.find(cursor); it != cursor_cache.end())
    {
      return it->second;
    }

    SDL_SystemCursor sdl_cursor = SDL_SYSTEM_CURSOR_ARROW;
    switch (cursor)
    {
    case UI::Style::Cursor::DEFAULT:
      sdl_cursor = SDL_SYSTEM_CURSOR_ARROW;
      break;
    case UI::Style::Cursor::POINTER:
      sdl_cursor = SDL_SYSTEM_CURSOR_HAND;
      break;
    case UI::Style::Cursor::TEXT:
      sdl_cursor = SDL_SYSTEM_CURSOR_IBEAM;
      break;
    case UI::Style::Cursor::CROSSHAIR:
      sdl_cursor = SDL_SYSTEM_CURSOR_CROSSHAIR;
      break;
    case UI::Style::Cursor::MOVE:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZEALL;
      break;
    case UI::Style::Cursor::NOT_ALLOWED:
      sdl_cursor = SDL_SYSTEM_CURSOR_NO;
      break;
    case UI::Style::Cursor::WAIT:
      sdl_cursor = SDL_SYSTEM_CURSOR_WAIT;
      break;
    case UI::Style::Cursor::PROGRESS:
      sdl_cursor = SDL_SYSTEM_CURSOR_WAITARROW;
      break;
    case UI::Style::Cursor::RESIZE_X:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZEWE;
      break;
    case UI::Style::Cursor::RESIZE_Y:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZENS;
      break;
    case UI::Style::Cursor::RESIZE_NWSE:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZENWSE;
      break;
    case UI::Style::Cursor::RESIZE_NESW:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZENESW;
      break;
    }

    SDL_Cursor* created = SDL_CreateSystemCursor(sdl_cursor);
    cursor_cache[cursor] = created;
    return created;
  }

  void Client::update_cursor()
  {
    if (!program || !program->pointer_visible)
    {
      SDL_ShowCursor(SDL_DISABLE);
      active_cursor = std::nullopt;
      return;
    }

    auto next_cursor = UI::Style::Cursor::DEFAULT;
    for (auto& weak_view : session.mouse_state.hovered_chain)
    {
      auto view = weak_view.lock();
      if (view && view->effective_style.cursor)
      {
        next_cursor = *view->effective_style.cursor;
        break;
      }
    }

    SDL_ShowCursor(SDL_ENABLE);
    if (active_cursor && *active_cursor == next_cursor)
    {
      return;
    }

    if (auto* cursor = system_cursor(next_cursor))
    {
      SDL_SetCursor(cursor);
      active_cursor = next_cursor;
    }
  }

  void Client::handle_keyup(SDL_KeyboardEvent& key_event)
  {

    events.do_key_up(key_event);
  }
} // namespace Pixils
