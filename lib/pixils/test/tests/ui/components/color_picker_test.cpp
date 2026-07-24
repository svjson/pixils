#include "../../render_fixture.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_mouse.h>
#include <gtest/gtest.h>
#include <pixils/runtime/view.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <sdl3_mock/mock_resources.h>

using ColorPickerTest = RenderFixture;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

  bool has_class(const std::shared_ptr<Pixils::Runtime::View>& view,
                 const std::string& class_name)
  {
    return view && view->mode &&
           std::find(view->mode->class_names.begin(),
                     view->mode->class_names.end(),
                     class_name) != view->mode->class_names.end();
  }

  std::shared_ptr<Pixils::Runtime::View> find_first_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;
    for (const auto& child : view->children)
    {
      if (auto found = find_first_mode(child, mode_name)) return found;
    }
    return nullptr;
  }

  std::shared_ptr<Pixils::Runtime::View> find_button_with_label(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& label_text)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/button")
    {
      auto label = get_key(view->state, "label");
      if (label && label->str() == label_text) return view;
    }
    for (const auto& child : view->children)
    {
      if (auto found = find_button_with_label(child, label_text)) return found;
    }
    return nullptr;
  }

  bool has_fill_rect(const std::vector<RenderOperation>& ops, const SDL_Rect& rect)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::FILL_RECT && op.rendered_rect.x == rect.x &&
          op.rendered_rect.y == rect.y && op.rendered_rect.w == rect.w &&
          op.rendered_rect.h == rect.h)
      {
        return true;
      }
      if (has_fill_rect(op.sub_ops, rect)) return true;
    }
    return false;
  }
} // namespace

TEST_F(ColorPickerTest, color_picker_make_constructs_swatch_and_normalizes_color)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.color-picker/make
                   {:class :paint/accent
                    :swatch-class :paint/accent-swatch
                    :value {:r 10 :g 20 :b 30}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto picker = session.active_mode->children[0];
  ASSERT_NE(picker, nullptr);
  EXPECT_EQ(picker->mode->name, "ui/color-picker");
  EXPECT_TRUE(has_class(picker, "paint/accent"));

  auto color = get_key(picker->state, "value");
  ASSERT_NE(color, nullptr);
  EXPECT_EQ(get_key(color, "r")->num().get_int(), 10);
  EXPECT_EQ(get_key(color, "g")->num().get_int(), 20);
  EXPECT_EQ(get_key(color, "b")->num().get_int(), 30);
  EXPECT_EQ(get_key(color, "a")->num().get_int(), 255);

  ASSERT_EQ(picker->children.size(), 1u);
  auto swatch = picker->children[0];
  ASSERT_NE(swatch, nullptr);
  EXPECT_EQ(swatch->mode->name, "ui/color-swatch");
  EXPECT_TRUE(has_class(swatch, "ui/color-picker-swatch"));
  EXPECT_TRUE(has_class(swatch, "paint/accent-swatch"));
  EXPECT_GT(swatch->bounds.w, 0);
  EXPECT_GT(swatch->bounds.h, 0);
  EXPECT_TRUE(swatch->effective_style.background.has_value());
  EXPECT_TRUE(swatch->effective_style.border.has_value());
}

TEST_F(ColorPickerTest, color_picker_swatch_renders_selected_color)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.color-picker/make
                   {:value {:r 10 :g 20 :b 30 :a 255}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto picker = session.active_mode->children[0];
  ASSERT_NE(picker, nullptr);
  ASSERT_EQ(picker->children.size(), 1u);
  auto swatch = picker->children[0];
  ASSERT_NE(swatch, nullptr);
  EXPECT_TRUE(has_fill_rect(render_target()->render_ops,
                            SDL_Rect{0, 0, swatch->bounds.w, swatch->bounds.h}));
}

TEST_F(ColorPickerTest, clicking_color_picker_opens_default_dialog_editor)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.color-picker/make
                   {:value {:r 10 :g 20 :b 30 :a 40}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto picker = session.active_mode->children[0];
  ASSERT_NE(picker, nullptr);
  input().mouse_down({picker->bounds.x + (picker->bounds.w / 2),
                      picker->bounds.y + (picker->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({picker->bounds.x + (picker->bounds.w / 2),
                    picker->bounds.y + (picker->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/color-picker-dialog");
  auto editor = find_first_mode(session.active_mode, "ui/color-editor");
  ASSERT_NE(editor, nullptr);
  auto color = get_key(editor->state, "value");
  ASSERT_NE(color, nullptr);
  EXPECT_EQ(get_key(color, "r")->num().get_int(), 10);
  EXPECT_EQ(get_key(color, "g")->num().get_int(), 20);
  EXPECT_EQ(get_key(color, "b")->num().get_int(), 30);
  EXPECT_EQ(get_key(color, "a")->num().get_int(), 40);
}

TEST_F(ColorPickerTest, color_picker_dialog_renders_over_underlying_mode)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:render (fn [state ctx]
                 (pixils.render/rect!
                  {:x 0 :y 0 :w 320 :h 200}
                  {:fill true
                   :color {:r 60 :g 70 :b 80 :a 255}}))
       :children [(pixils.ui.color-picker/make
                   {:value {:r 10 :g 20 :b 30 :a 40}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto picker = session.active_mode->children[0];
  ASSERT_NE(picker, nullptr);
  input().mouse_down({picker->bounds.x + (picker->bounds.w / 2),
                      picker->bounds.y + (picker->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({picker->bounds.x + (picker->bounds.w / 2),
                    picker->bounds.y + (picker->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();
  render_target()->render_ops.clear();
  session.render_mode();

  ASSERT_EQ(session.active_mode->mode->name, "ui/color-picker-dialog");
  EXPECT_TRUE(has_fill_rect(render_target()->render_ops, SDL_Rect{0, 0, 320, 200}));
  EXPECT_NE(find_first_mode(session.active_mode, "ui/color-editor"), nullptr);
}

TEST_F(ColorPickerTest, default_dialog_ok_commits_edited_color_to_picker)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:color {:r 0 :g 20 :b 30 :a 255}
                :last-change nil})
       :children [(pixils.ui.color-picker/make
                   {:value (pixils.ui/bind-state :color)})]
       :on {:color-picker/change
            (fn [state event ctx]
              (-> state
                  (assoc :color (-> event :payload :value))
                  (assoc :last-change (:payload event))))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto picker = session.active_mode->children[0];
  input().mouse_down({picker->bounds.x + (picker->bounds.w / 2),
                      picker->bounds.y + (picker->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({picker->bounds.x + (picker->bounds.w / 2),
                    picker->bounds.y + (picker->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();

  auto slider = find_first_mode(session.active_mode, "ui/slider");
  ASSERT_NE(slider, nullptr);
  input().mouse_down({slider->bounds.x + slider->bounds.w - 2,
                      slider->bounds.y + (slider->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({slider->bounds.x + slider->bounds.w - 2,
                    slider->bounds.y + (slider->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();

  auto ok_button = find_button_with_label(session.active_mode, "OK");
  ASSERT_NE(ok_button, nullptr);
  input().mouse_down({ok_button->bounds.x + (ok_button->bounds.w / 2),
                      ok_button->bounds.y + (ok_button->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({ok_button->bounds.x + (ok_button->bounds.w / 2),
                    ok_button->bounds.y + (ok_button->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto color = get_key(session.active_mode->state, "color");
  auto last_change = get_key(session.active_mode->state, "last-change");
  ASSERT_NE(color, nullptr);
  ASSERT_NE(last_change, nullptr);

  auto changed = get_key(last_change, "value");
  ASSERT_NE(changed, nullptr);
  EXPECT_GT(get_key(color, "r")->num().get_int(), 200);
  EXPECT_EQ(get_key(color, "g")->num().get_int(), 20);
  EXPECT_EQ(get_key(color, "b")->num().get_int(), 30);
  EXPECT_EQ(get_key(color, "a")->num().get_int(), 255);
  EXPECT_EQ(color->to_string(), changed->to_string());
}
