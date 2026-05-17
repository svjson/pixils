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

TEST_F(ButtonTest, button_label_uses_base_theme_padding)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :state {:label "OK"}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto button = session.active_mode->children[0];
  ASSERT_NE(button, nullptr);
  ASSERT_EQ(button->children.size(), 1u);
  auto button_inner = button->children[0];
  ASSERT_NE(button_inner, nullptr);
  ASSERT_FALSE(button_inner->mode->selector_modes.empty());
  EXPECT_EQ(button_inner->mode->selector_modes[0], "ui/button-inner");
  ASSERT_EQ(button_inner->children.size(), 1u);
  auto label = button_inner->children[0];
  ASSERT_NE(label, nullptr);
  ASSERT_EQ(label->mode->name, "ui/text");
  ASSERT_FALSE(label->mode->selector_modes.empty());
  EXPECT_EQ(label->mode->selector_modes[0], "ui/text");
  ASSERT_TRUE(label->effective_style.padding.has_value());
  EXPECT_GT(label->effective_style.padding->t, 0);
  EXPECT_GT(label->effective_style.padding->r, 0);
  EXPECT_GT(label->effective_style.padding->b, 0);
  EXPECT_GT(label->effective_style.padding->l, 0);
}
