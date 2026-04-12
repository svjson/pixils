
#ifndef PIXILS__TEST__RENDER_FIXTURE_H
#define PIXILS__TEST__RENDER_FIXTURE_H

#include "runtime/session_fixture.h"

#include <sdl2_mock/mock_resources.h>

#include <SDL2/SDL_render.h>

/**
 * Extends SessionFixture with a live mock renderer. Use this for any test
 * that invokes render hooks - SDL draw calls will be recorded on the mock
 * renderer's render target and can be inspected via render_target().
 */
class RenderFixture : public SessionFixture
{
protected:
  RenderFixture()
  {
    render_ctx.renderer     = SDL_CreateRenderer(nullptr, 0, 0);
    render_ctx.buffer_texture = render_ctx.renderer->default_render_target;
    render_ctx.buffer_dim   = {320, 200};
  }

  void TearDown() override { SDLMock::reset_mocks(); }

  SDL_Texture* render_target() { return render_ctx.renderer->render_target; }
};

#endif /* PIXILS__TEST__RENDER_FIXTURE_H */
