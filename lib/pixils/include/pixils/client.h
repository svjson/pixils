
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

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
    Lisple::sptr_sobject events;
    Lisple::sptr_sobject ctx;

    Lisple::sptr_sobject_v init_args = {ctx};
    Lisple::sptr_sobject_v update_args = {events, ctx};
    Lisple::sptr_sobject_v render_args = {ctx};
  };

  class Client
  {
    Lisple::Runtime& lisple;
    RenderContext& ctx;
    Asset::Registry assets;
    Lisple::Array& mode_stack;

    ActiveMode active_mode;
    FrameEvents events;

    HookArguments hook_args;

   public:
    Client(Lisple::Runtime& lisple_runtime, RenderContext& ctx, Runtime::Mode& root_mode);

    void run();

   private:
    void main_loop();

    void activate_mode();
  };

} // namespace Pixils

#endif
