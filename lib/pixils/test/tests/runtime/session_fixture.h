
#ifndef PIXILS__TEST__SESSION_FIXTURE_H
#define PIXILS__TEST__SESSION_FIXTURE_H

#include "../fixture.h"
#include "../input_simulator.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/script.h>

#include <gtest/gtest.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

/**
 * Test fixture providing a minimal Pixils runtime and Session with no SDL
 * dependency. The RenderContext is zero-initialized (null SDL pointers); tests
 * must not call any code path that renders or loads image assets.
 */
class SessionFixture : public BaseFixture
{
 protected:
  Pixils::Asset::Registry assets;
  Pixils::FrameEvents events;
  InputSimulator input_simulator;
  Pixils::HookContext hook_ctx;
  Pixils::Runtime::HookArguments hook_args;
  Pixils::Runtime::Session session;

  SessionFixture()
    : BaseFixture()
    , assets(render_ctx)
    , input_simulator(events)
    , hook_ctx{&events, &render_ctx}
    , hook_args{Pixils::Script::HookContextAdapter::make_ref(hook_ctx)}
    , session(runtime, assets, render_ctx, hook_args)
  {
    session.hook_args.events = &events;
  }

  InputSimulator& input() { return input_simulator; }
  void update_cycle()
  {
    session.update_mode();
    session.process_messages();
    input_simulator.clear_transients();
  }
  void render_cycle()
  {
    session.render_mode();
    input_simulator.clear_transients();
  }
  void frame_cycle()
  {
    session.update_mode();
    session.process_messages();
    session.render_mode();
    input_simulator.clear_transients();
  }
};

#endif /* PIXILS__TEST__SESSION_FIXTURE_H */
