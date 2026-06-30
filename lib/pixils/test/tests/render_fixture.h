
#ifndef PIXILS__TEST__RENDER_FIXTURE_H
#define PIXILS__TEST__RENDER_FIXTURE_H

#include "runtime/session_fixture.h"
#include <pixils/runtime/view.h>

#include <SDL2/SDL_render.h>
#include <sdl2_mock/mock_resources.h>

/**
 * Extends SessionFixture with a live mock renderer. Use this for any test
 * that invokes render hooks - SDL draw calls will be recorded on the mock
 * renderer's render target and can be inspected via render_target().
 */
class RenderFixture : public SessionFixture
{
 protected:
  RenderFixture()
    : SessionFixture(make_render_context())
  {
  }

  void TearDown() override { SDLMock::reset_mocks(); }

  SDL_Texture* render_target() { return render_ctx.renderer->render_target; }

 private:
  static Pixils::RenderContext make_render_context()
  {
    Pixils::RenderContext ctx{};
    ctx.renderer = SDL_CreateRenderer(nullptr, 0, 0);
    ctx.buffer_texture = ctx.renderer->default_render_target;
    ctx.buffer_dim = {320, 200};
    ctx.enable_render_geometry = false;
    return ctx;
  }
};

#endif /* PIXILS__TEST__RENDER_FIXTURE_H */
