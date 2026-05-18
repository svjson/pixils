#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>

using ButtonTest = RenderFixture;

namespace
{
  Lisple::sptr_val get_key(const Lisple::sptr_val& value, const std::string& key)
  {
    return Lisple::Dict::get_property(value, Lisple::keyword(key));
  }
} // namespace

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
  auto label_value = get_key(label->state, "value");
  ASSERT_NE(label_value, nullptr);
  EXPECT_EQ(label_value->str(), "OK");
  EXPECT_GT(button->bounds.w, 20);
  EXPECT_GT(button->bounds.h, 10);
}

TEST_F(ButtonTest, disabled_button_does_not_fire_click_handler)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/button
                   :style {:width 40 :height 24}
                   :state {:label "OK"
                           :clicks 0
                           :disabled? true}
                   :on-click (fn [state event ctx]
                               (assoc state :clicks (+ (:clicks state) 1)))}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_up({10, 10});
  update_cycle();

  auto clicks = get_key(session.active_mode->children[0]->state, "clicks");
  ASSERT_NE(clicks, nullptr);
  EXPECT_EQ(clicks->to_string(), "0");
}

TEST_F(ButtonTest, button_label_has_natural_size_inside_window_body)
{
  runtime.eval(R"(
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
                   :state {:label "Create"}}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Window"}
                    :style {:width 300}
                    :body [{:mode 'form-body}]})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.render_mode();

  auto window = session.active_mode->children[0];
  auto body = window->children[1];
  auto form = body->children[0];
  auto button = form->children[0];
  EXPECT_GT(button->bounds.w, 40);
  EXPECT_GT(button->bounds.h, 10);
}

TEST_F(ButtonTest, button_label_has_natural_size_inside_pushed_dialog_frame)
{
  runtime.eval(R"(
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
                   :state {:label "Create"}}]})

    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils/push-mode!
                  'ui/dialog-frame
                  {}
                  {:children [(pixils.ui.window/make
                               {:title-bar {:title "Window"}
                                :style {:width 300}
                                :body [{:mode 'form-body}]})]})
                 state))})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.process_messages();
  session.render_mode();

  auto window = session.active_mode->children[0];
  auto body = window->children[1];
  auto form = body->children[0];
  auto button = form->children[0];
  EXPECT_GT(window->bounds.h, 0);
  EXPECT_GT(button->bounds.w, 40);
  EXPECT_GT(button->bounds.h, 10);
}
