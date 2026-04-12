
#ifndef PIXILS__TEST__SESSION_FIXTURE_H
#define PIXILS__TEST__SESSION_FIXTURE_H

#include "../fixture.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/session.h>
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
  Pixils::Runtime::HookArguments hook_args;
  Pixils::Runtime::Session session;

  SessionFixture()
    : BaseFixture()
    , assets(render_ctx)
    , hook_args{Lisple::Constant::NIL, Pixils::Script::RenderContextAdapter::make_ref(render_ctx)}
    , session(runtime, assets, render_ctx, hook_args)
  {
  }
};

#endif /* PIXILS__TEST__SESSION_FIXTURE_H */
