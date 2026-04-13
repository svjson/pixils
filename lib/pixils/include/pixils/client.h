
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

#include "pixils/console.h"
#include <pixils/asset/registry.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>

#include <lisple/runtime/value.h>
#include <stddef.h>

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
    Asset::Registry assets;
    FrameEvents events;
    HookContext hook_ctx;
    Runtime::Session session;
    Program* program;

    Asset::Bundle client_bundle;

    std::unique_ptr<ConsoleOverlay> console = nullptr;

   public:
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx);
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, Runtime::Mode& root_mode);

    void run();

   private:
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, bool init_mode);

    void init_console();
    void main_loop();

    void handle_keydown(SDL_KeyboardEvent& event);
    void handle_keyup(SDL_KeyboardEvent& event);
  };

} // namespace Pixils

#endif
