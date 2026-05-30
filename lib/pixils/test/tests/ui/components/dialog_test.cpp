#include "../../render_fixture.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>

using DialogTest = RenderFixture;
using Pixils::Runtime::View;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

  void collect_button_labels(const std::shared_ptr<View>& view,
                             std::vector<std::string>& labels)
  {
    if (!view) return;
    if (view->mode && view->mode->name == "ui/button")
    {
      auto label = get_key(view->state, "label");
      if (label) labels.push_back(label->str());
    }

    for (const auto& child : view->children)
    {
      collect_button_labels(child, labels);
    }
  }

  std::shared_ptr<View> find_button_with_label(const std::shared_ptr<View>& view,
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
      auto match = find_button_with_label(child, label_text);
      if (match) return match;
    }

    return nullptr;
  }
} // namespace

TEST_F(DialogTest, make_confirm_renders_buttons_in_declared_order_with_label_overrides)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.dialog/make-confirm
                   {:title "Delete Layer"
                    :body {:mode 'ui/text
                           :state {:value "Delete this layer?"}}
                    :buttons [:dialog/cancel
                              {:type :dialog/ok
                               :label "Delete"}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  std::vector<std::string> labels;
  collect_button_labels(session.active_mode, labels);

  ASSERT_EQ(labels.size(), 2u);
  EXPECT_EQ(labels[0], "Cancel");
  EXPECT_EQ(labels[1], "Delete");
}

TEST_F(DialogTest, open_confirm_pops_selected_choice_and_payload_to_origin_event)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.dialog/open-confirm!
                  ctx
                  {:title "Delete Layer"
                   :body {:mode 'ui/text
                          :state {:value "Delete this layer?"}}
                   :buttons [{:type :dialog/ok
                              :label "Delete"}
                             :dialog/cancel]
                   :result-event :layer/delete-result
                   :payload {:index 3}})
                 {:result nil}))
       :on {:layer/delete-result (fn [state event ctx]
                                   (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.process_messages();
  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  session.render_mode();

  auto delete_button = find_button_with_label(session.active_mode, "Delete");
  ASSERT_NE(delete_button, nullptr);

  input().mouse_down({delete_button->bounds.x + (delete_button->bounds.w / 2),
                      delete_button->bounds.y + (delete_button->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({delete_button->bounds.x + (delete_button->bounds.w / 2),
                    delete_button->bounds.y + (delete_button->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  auto choice = get_key(result, "choice");
  auto payload = get_key(result, "payload");
  ASSERT_NE(choice, nullptr);
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(choice->str(), "dialog/ok");

  auto index = get_key(payload, "index");
  ASSERT_NE(index, nullptr);
  EXPECT_EQ(index->num().get_int(), 3);
}

TEST_F(DialogTest, dismissable_confirm_returns_dismiss_choice_on_escape)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.dialog/open-confirm!
                  ctx
                  {:title "Dismissable"
                   :body {:mode 'ui/text
                          :state {:value "Dismiss?"}}
                   :dismissable true
                   :result-event :dialog/dismissed
                   :payload {:source :escape-test}})
                 {:result nil}))
       :on {:dialog/dismissed (fn [state event ctx]
                                (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.process_messages();
  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  session.render_mode();

  input().key_down(SDLK_ESCAPE);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  auto choice = get_key(result, "choice");
  auto payload = get_key(result, "payload");
  ASSERT_NE(choice, nullptr);
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(choice->str(), "dialog/dismiss");
  EXPECT_EQ(get_key(payload, "source")->str(), "escape-test");
}

TEST_F(DialogTest, open_dialog_wraps_custom_result_event_handlers)
{
  runtime.eval(R"(
    (pixils/defcomponent form-body
      {:children [{:mode 'ui/button
                   :state {:label "Create"}
                   :on-click (fn [state event ctx]
                               (do
                                 (pixils.ui/emit! (:view ctx)
                                                  :dialog/confirm
                                                  {:source :form-button})
                                 state))}]})

    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.dialog/open-dialog!
                  ctx
                  {:title (pixils.ui/bind-state :title)
                   :style {:width 300}
                   :state {:title "New Ship"
                           :name "Falcon"
                           :type :ship}
                   :body {:mode 'form-body}
                   :result-event :component/dialog-result
                   :results {:dialog/confirm
                             (fn [state event ctx]
                               {:name (:name state)
                                :type (:type state)
                                :source (-> event :payload :source)})}})
                 {:result nil}))
       :on {:component/dialog-result (fn [state event ctx]
                                       (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.process_messages();
  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children[0]->mode->name, "ui/window");
  ASSERT_TRUE(session.active_mode->children[0]->effective_style.width.has_value());
  EXPECT_EQ(session.active_mode->children[0]->effective_style.width->fixed_value_or(0), 300);

  auto create_button = find_button_with_label(session.active_mode, "Create");
  ASSERT_NE(create_button, nullptr);
  EXPECT_GT(create_button->bounds.w, 0);
  EXPECT_GT(create_button->bounds.h, 0);
  input().mouse_down({create_button->bounds.x + (create_button->bounds.w / 2),
                      create_button->bounds.y + (create_button->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_up({create_button->bounds.x + (create_button->bounds.w / 2),
                    create_button->bounds.y + (create_button->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(get_key(result, "name")->str(), "Falcon");
  EXPECT_EQ(get_key(result, "type")->str(), "ship");
  EXPECT_EQ(get_key(result, "source")->str(), "form-button");
}
