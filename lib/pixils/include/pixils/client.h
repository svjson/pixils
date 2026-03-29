
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

#include "pixils/console.h"
#include <pixils/asset/registry.h>
#include <pixils/frame_events.h>

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

  struct Program;
  struct RenderContext;

  struct ActiveMode
  {
    int mode_index = -1;

    std::string init_fun;
    std::string update_fun;
    std::string render_fun;
  };

  struct HookArguments
  {
    Lisple::sptr_rtval events;
    Lisple::sptr_rtval ctx;

    Lisple::sptr_rtval_v init_args = {ctx};
    Lisple::sptr_rtval_v update_args = {events, ctx};
    Lisple::sptr_rtval_v render_args = {ctx};
  };

  class Client
  {
    Lisple::Runtime& lisple;
    RenderContext& ctx;
    Asset::Registry assets;
    Lisple::sptr_rtval mode_stack;
    Program* program;

    ActiveMode active_mode;
    FrameEvents events;

    HookArguments hook_args;

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

    void activate_mode();

    void handle_keydown(SDL_KeyboardEvent& event);
    void handle_keyup(SDL_KeyboardEvent& event);
  };

} // namespace Pixils

#endif
