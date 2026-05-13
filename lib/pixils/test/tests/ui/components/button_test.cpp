#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <sdl2_mock/mock_resources.h>

using ButtonTest = RenderFixture;

TEST_F(ButtonTest, button_can_use_fitted_background_image_from_style)
{
  SDLMock::prepared_surfaces["./brush.png"] = {8, 16};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:brush "brush.png"}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 24
                           :height 24
                           :background {:image :icons/brush
                                        :fit :contain
                                        :align :center}}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_GE(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 6);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 12);
  EXPECT_EQ(ops[0].rendered_rect.h, 24);
}
