
#ifndef PIXILS__TEST__FIXTURE_H
#define PIXILS__TEST__FIXTURE_H

#include <pixils/context.h>
#include <pixils/script.h>

#include <gtest/gtest.h>
#include <roo/runtime.h>
#include <roo/runtime/value.h>
#include <utility>

class BaseFixture : public ::testing::Test
{
 protected:
  Pixils::RenderContext render_ctx{};
  Roo::Runtime runtime;

  BaseFixture()
    : BaseFixture(Pixils::RenderContext{})
  {
  }

  explicit BaseFixture(Pixils::RenderContext initial_render_ctx)
    : render_ctx(std::move(initial_render_ctx))
    , runtime(Pixils::init_roo_runtime(this->render_ctx, "test", {}))
  {
  }
};

#endif
