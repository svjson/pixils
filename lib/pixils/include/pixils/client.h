
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

#include "pixils/console.h"
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/session.h>
#include <pixils/ui/style.h>

#include <SDL2/SDL_mouse.h>
#include <lisple/runtime/value.h>
#include <map>
#include <stddef.h>
#include <string>

namespace Lisple
{
  class Array;
  class Runtime;
} // namespace Lisple

namespace Pixils
{
  namespace Runtime
  {
    struct Mode;
  }

  namespace Script
  {
    class ModeAdapter;
  }

  class Program;
  struct RenderContext;

  class Client
  {
    Lisple::Runtime& lisple;
    RenderContext& ctx;
    FrameEvents events;
    HookContext hook_ctx;
    Runtime::Session session;
    Program* program = nullptr;

    std::unique_ptr<ConsoleOverlay> console = nullptr;
    std::map<UI::SystemCursor, SDL_Cursor*> cursor_cache;
    std::map<std::string, SDL_Cursor*> image_cursor_cache;
    std::optional<UI::CursorSpec> active_cursor = std::nullopt;

   public:
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx);
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, Runtime::Mode& root_mode);
    ~Client();

    void run();

   private:
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, bool init_mode);

    void init_console();
    void main_loop();

    void handle_keydown(SDL_KeyboardEvent& event);
    void handle_keyup(SDL_KeyboardEvent& event);
    void update_cursor();
    SDL_Cursor* system_cursor(UI::SystemCursor cursor);
    SDL_Cursor* image_cursor(const UI::ImageCursor& cursor);
    SDL_Cursor* resolved_cursor(const UI::CursorSpec& cursor);
    std::optional<UI::ImageCursor> app_rendered_cursor(const UI::CursorSpec& cursor) const;
    void render_app_cursor();
  };

} // namespace Pixils

#endif
