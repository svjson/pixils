
#ifndef PIXILS__TEST__FIXTURE_H
#define PIXILS__TEST__FIXTURE_H

#include <pixils/context.h>
#include <pixils/script.h>

#include <gtest/gtest.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

class BaseFixture : public ::testing::Test
{
 protected:
  Pixils::RenderContext render_ctx{};
  Lisple::Runtime runtime;

  BaseFixture()
    : render_ctx{}
    , runtime(Pixils::init_lisple_runtime(render_ctx, "test", {}))
  {
  }
};

#endif
