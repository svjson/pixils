
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
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>
#include <memory>
#include <sstream>

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
    for (auto& [_, cursor] : image_cursor_cache)
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
          render_app_cursor();
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
        ctx.set_render_target(nullptr);
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

  SDL_Cursor* Client::system_cursor(UI::SystemCursor cursor)
  {
    if (auto it = cursor_cache.find(cursor); it != cursor_cache.end())
    {
      return it->second;
    }

    SDL_SystemCursor sdl_cursor = SDL_SYSTEM_CURSOR_ARROW;
    switch (cursor)
    {
    case UI::SystemCursor::DEFAULT:
      sdl_cursor = SDL_SYSTEM_CURSOR_ARROW;
      break;
    case UI::SystemCursor::POINTER:
      sdl_cursor = SDL_SYSTEM_CURSOR_HAND;
      break;
    case UI::SystemCursor::TEXT:
      sdl_cursor = SDL_SYSTEM_CURSOR_IBEAM;
      break;
    case UI::SystemCursor::CROSSHAIR:
      sdl_cursor = SDL_SYSTEM_CURSOR_CROSSHAIR;
      break;
    case UI::SystemCursor::MOVE:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZEALL;
      break;
    case UI::SystemCursor::NOT_ALLOWED:
      sdl_cursor = SDL_SYSTEM_CURSOR_NO;
      break;
    case UI::SystemCursor::WAIT:
      sdl_cursor = SDL_SYSTEM_CURSOR_WAIT;
      break;
    case UI::SystemCursor::PROGRESS:
      sdl_cursor = SDL_SYSTEM_CURSOR_WAITARROW;
      break;
    case UI::SystemCursor::RESIZE_X:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZEWE;
      break;
    case UI::SystemCursor::RESIZE_Y:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZENS;
      break;
    case UI::SystemCursor::RESIZE_NWSE:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZENWSE;
      break;
    case UI::SystemCursor::RESIZE_NESW:
      sdl_cursor = SDL_SYSTEM_CURSOR_SIZENESW;
      break;
    }

    SDL_Cursor* created = SDL_CreateSystemCursor(sdl_cursor);
    cursor_cache[cursor] = created;
    return created;
  }

  std::string image_cursor_key(const UI::ImageCursor& cursor)
  {
    std::ostringstream key;
    if (cursor.image)
    {
      key << cursor.image->first << "/" << cursor.image->second;
    }
    key << "|";
    if (cursor.source)
    {
      key << cursor.source->x << "," << cursor.source->y << "," << cursor.source->w << ","
          << cursor.source->h;
    }
    key << "|" << cursor.hotspot.round_x() << "," << cursor.hotspot.round_y() << "|"
        << cursor.scale;
    return key.str();
  }

  SDL_Cursor* Client::image_cursor(const UI::ImageCursor& cursor)
  {
    std::string key = image_cursor_key(cursor);
    if (auto it = image_cursor_cache.find(key); it != image_cursor_cache.end())
    {
      return it->second;
    }

    if (!cursor.image) return nullptr;

    SDL_Surface* source_surface =
      ctx.asset_registry->get_image_surface(cursor.image->first, cursor.image->second);
    if (!source_surface) return nullptr;

    SDL_Surface* formatted_source =
      SDL_ConvertSurfaceFormat(source_surface, SDL_PIXELFORMAT_RGBA32, 0);
    if (!formatted_source) return nullptr;

    SDL_Rect source_rect{0, 0, source_surface->w, source_surface->h};
    if (cursor.source)
    {
      source_rect = cursor.source->to_SDL_rect();
    }
    if (source_rect.w <= 0 || source_rect.h <= 0)
    {
      SDL_FreeSurface(formatted_source);
      return nullptr;
    }

    int scale = std::max(1, cursor.scale);
    SDL_Rect target_rect{0, 0, source_rect.w * scale, source_rect.h * scale};
    SDL_Surface* final_surface = SDL_CreateRGBSurfaceWithFormat(0,
                                                                target_rect.w,
                                                                target_rect.h,
                                                                32,
                                                                SDL_PIXELFORMAT_RGBA32);
    if (!final_surface)
    {
      SDL_FreeSurface(formatted_source);
      return nullptr;
    }

    SDL_SetSurfaceBlendMode(formatted_source, SDL_BLENDMODE_NONE);
    if (SDL_BlitScaled(formatted_source, &source_rect, final_surface, &target_rect) != 0)
    {
      SDL_FreeSurface(formatted_source);
      SDL_FreeSurface(final_surface);
      return nullptr;
    }

    int hot_x = std::clamp(cursor.hotspot.round_x() * scale, 0, target_rect.w - 1);
    int hot_y = std::clamp(cursor.hotspot.round_y() * scale, 0, target_rect.h - 1);
    SDL_Cursor* created = SDL_CreateColorCursor(final_surface, hot_x, hot_y);
    SDL_FreeSurface(formatted_source);
    SDL_FreeSurface(final_surface);

    image_cursor_cache[key] = created;
    return created;
  }

  SDL_Cursor* Client::resolved_cursor(const UI::CursorSpec& cursor)
  {
    switch (cursor.kind)
    {
    case UI::CursorSpec::Kind::SYSTEM:
      return system_cursor(cursor.system);
    case UI::CursorSpec::Kind::IMAGE:
      if (cursor.image.render_mode == UI::ImageCursor::RenderMode::APP)
      {
        return nullptr;
      }
      return image_cursor(cursor.image);
    case UI::CursorSpec::Kind::NAMED:
    {
      auto it = ctx.pointer_registry.find(cursor.name);
      if (it == ctx.pointer_registry.end()) return nullptr;
      if (it->second.render_mode == UI::ImageCursor::RenderMode::APP)
      {
        return nullptr;
      }
      return image_cursor(it->second);
    }
    }

    return nullptr;
  }

  std::optional<UI::ImageCursor> Client::app_rendered_cursor(
    const UI::CursorSpec& cursor) const
  {
    switch (cursor.kind)
    {
    case UI::CursorSpec::Kind::IMAGE:
      if (cursor.image.render_mode == UI::ImageCursor::RenderMode::APP)
      {
        return cursor.image;
      }
      return std::nullopt;
    case UI::CursorSpec::Kind::NAMED:
    {
      auto it = ctx.pointer_registry.find(cursor.name);
      if (it != ctx.pointer_registry.end() &&
          it->second.render_mode == UI::ImageCursor::RenderMode::APP)
      {
        return it->second;
      }
      return std::nullopt;
    }
    case UI::CursorSpec::Kind::SYSTEM:
      return std::nullopt;
    }

    return std::nullopt;
  }

  void Client::render_app_cursor()
  {
    if (!program || !program->pointer_visible || !active_cursor) return;

    auto cursor = app_rendered_cursor(*active_cursor);
    if (!cursor || !cursor->image) return;

    SDL_Texture* texture =
      ctx.asset_registry->get_image(cursor->image->first, cursor->image->second);
    if (!texture) return;

    SDL_Rect source_rect{0, 0, 0, 0};
    SDL_Rect* source_ptr = nullptr;
    if (cursor->source)
    {
      source_rect = cursor->source->to_SDL_rect();
      source_ptr = &source_rect;
    }
    else if (SDL_QueryTexture(texture, nullptr, nullptr, &source_rect.w, &source_rect.h) ==
             0)
    {
      source_ptr = &source_rect;
    }
    if (source_rect.w <= 0 || source_rect.h <= 0) return;

    int mouse_x = 0;
    int mouse_y = 0;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    Point mouse_pos = ctx.window_to_buffer_point(program->get_display(), mouse_x, mouse_y);
    Point snapped = mouse_pos.round();
    int scale = std::max(1, cursor->scale);
    SDL_Rect dest{snapped.round_x() - (cursor->hotspot.round_x() * scale),
                  snapped.round_y() - (cursor->hotspot.round_y() * scale),
                  source_rect.w * scale,
                  source_rect.h * scale};

    ctx.set_render_target(ctx.buffer_texture);
    SDL_RenderCopy(ctx.renderer, texture, source_ptr, &dest);
  }

  void Client::update_cursor()
  {
    if (!program || !program->pointer_visible)
    {
      SDL_ShowCursor(SDL_DISABLE);
      active_cursor = std::nullopt;
      return;
    }

    auto next_cursor = UI::CursorSpec::system_cursor(UI::SystemCursor::DEFAULT);
    for (auto& weak_view : session.mouse_state.hovered_chain)
    {
      auto view = weak_view.lock();
      if (view && view->effective_style.cursor)
      {
        next_cursor = *view->effective_style.cursor;
        break;
      }
    }

    if (active_cursor && *active_cursor == next_cursor)
    {
      return;
    }

    if (app_rendered_cursor(next_cursor))
    {
      SDL_ShowCursor(SDL_DISABLE);
      active_cursor = next_cursor;
      return;
    }

    SDL_ShowCursor(SDL_ENABLE);
    if (auto* cursor = resolved_cursor(next_cursor))
    {
      SDL_SetCursor(cursor);
      active_cursor = next_cursor;
      return;
    }

    if (auto* cursor = system_cursor(UI::SystemCursor::DEFAULT))
    {
      SDL_SetCursor(cursor);
      active_cursor = UI::CursorSpec::system_cursor(UI::SystemCursor::DEFAULT);
    }
  }

  void Client::handle_keyup(SDL_KeyboardEvent& key_event)
  {

    events.do_key_up(key_event);
  }
} // namespace Pixils
