
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

#include "pixils/console.h"
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/session.h>
#include <pixils/ui/style.h>

#include <SDL3/SDL_mouse.h>
#include <roo/runtime/value.h>
#include <map>
#include <stddef.h>
#include <string>

namespace Roo
{
  class Array;
  class Runtime;
} // namespace Roo

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
    Roo::Runtime& roo;
    RenderContext& ctx;
    FrameEvents events;
    HookContext hook_ctx;
    Runtime::Session session;
    Program* program = nullptr;

    std::unique_ptr<ConsoleOverlay> console = nullptr;
    std::unique_ptr<Text::Renderer> stats_text_renderer = nullptr;
    bool stats_overlay_visible = false;
    int last_frame_time_ms = 0;
    double last_frame_rate = 0.0;
    double last_update_time_ms = 0.0;
    double last_render_time_ms = 0.0;
    double last_pacing_wait_ms = 0.0;
    std::map<UI::SystemCursor, SDL_Cursor*> cursor_cache;
    std::map<std::string, SDL_Cursor*> image_cursor_cache;
    std::optional<UI::CursorSpec> active_cursor = std::nullopt;

   public:
    Client(Roo::Runtime& roo_runtime, RenderContext& ctx);
    Client(Roo::Runtime& roo_runtime, RenderContext& ctx, Runtime::Mode& root_mode);
    ~Client();

    void run();

   private:
    Client(Roo::Runtime& roo_runtime, RenderContext& ctx, bool init_mode);

    void init_console();
    void main_loop();
    void render_stats_overlay();

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
