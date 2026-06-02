#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

using IconTest = RenderFixture;

TEST_F(IconTest, plain_icon_is_not_draggable_or_absolute_positioned)
{
  auto modes = runtime.lookup("pixils/modes");
  ASSERT_NE(modes, nullptr);

  auto icon_mode_value = Roo::Dict::get_property(modes, Roo::symbol("ui/icon"));
  ASSERT_NE(icon_mode_value, nullptr);
  const auto& icon_mode = Roo::obj<Pixils::Runtime::Mode>(*icon_mode_value);
  EXPECT_FALSE(icon_mode.drag.has_value());

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:style {:width 100
               :height 20
               :layout {:direction :row}}
       :children [{:mode 'ui/icon
                   :style {:width 20
                           :height 20}
                   :state {:item {:id :one}}}
                  {:style {:width 20
                           :height 20}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto icon = session.active_mode->children[0];
  auto sibling = session.active_mode->children[1];
  ASSERT_NE(icon, nullptr);
  ASSERT_NE(sibling, nullptr);
  EXPECT_EQ(icon->bounds.x, 0);
  EXPECT_EQ(sibling->bounds.x, 20);
}

TEST_F(IconTest, desktop_icon_provides_focusable_image_and_label_shell)
{
  auto modes = runtime.lookup("pixils/modes");
  ASSERT_NE(modes, nullptr);

  auto desktop_icon_mode_value =
    Roo::Dict::get_property(modes, Roo::symbol("ui/desktop-icon"));
  ASSERT_NE(desktop_icon_mode_value, nullptr);
  const auto& desktop_icon_mode =
    Roo::obj<Pixils::Runtime::Mode>(*desktop_icon_mode_value);
  EXPECT_TRUE(desktop_icon_mode.focusable);

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:style {:width 160
               :height 140}
       :children [{:mode 'ui/desktop-icon
                   :state {:item {:id :disk
                                  :label "Disk"
                                  :position {:x 7 :y 9}}
                           :selected-id :disk}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto icon = session.active_mode->children[0];
  ASSERT_NE(icon, nullptr);
  ASSERT_NE(icon->mode, nullptr);
  EXPECT_EQ(icon->mode->name, "ui/desktop-icon");
  EXPECT_EQ(icon->bounds.x, 7);
  EXPECT_EQ(icon->bounds.y, 9);

  ASSERT_EQ(icon->children.size(), 2u);
  auto image = icon->children[0];
  auto label = icon->children[1];
  ASSERT_NE(image, nullptr);
  ASSERT_NE(label, nullptr);
  ASSERT_NE(image->mode, nullptr);
  ASSERT_NE(label->mode, nullptr);
  EXPECT_EQ(image->mode->name, "ui/desktop-icon-image");
  EXPECT_EQ(label->mode->name, "ui/desktop-icon-label");

  ASSERT_EQ(label->children.size(), 1u);
  auto label_box = label->children[0];
  ASSERT_NE(label_box, nullptr);
  ASSERT_NE(label_box->mode, nullptr);
  ASSERT_FALSE(label_box->mode->class_names.empty());
  EXPECT_EQ(label_box->mode->class_names[0], "ui/desktop-icon-label-box");

  ASSERT_EQ(label_box->children.size(), 1u);
  auto label_text = label_box->children[0];
  ASSERT_NE(label_text, nullptr);
  ASSERT_NE(label_text->mode, nullptr);
  EXPECT_EQ(label_text->mode->name, "ui/text");
  ASSERT_FALSE(label_text->mode->class_names.empty());
  EXPECT_EQ(label_text->mode->class_names[0], "ui/desktop-icon-label-text");

  auto wrapper_selected =
    Roo::Dict::get_property(icon->state, Roo::keyword("selected"));
  auto label_value = Roo::Dict::get_property(label->state, Roo::keyword("label"));
  auto label_selected = Roo::Dict::get_property(label->state, Roo::keyword("selected"));
  auto box_selected =
    Roo::Dict::get_property(label_box->state, Roo::keyword("selected"));
  ASSERT_NE(wrapper_selected, nullptr);
  ASSERT_NE(label_value, nullptr);
  EXPECT_EQ(label_value->to_string(), "\"Disk\"");
  EXPECT_EQ(wrapper_selected->to_string(), "true");
  if (label_selected)
  {
    EXPECT_NE(label_selected->to_string(), "true");
  }
  if (box_selected)
  {
    EXPECT_NE(box_selected->to_string(), "true");
  }
}

TEST_F(IconTest, desktop_icon_without_ids_is_not_selected_by_default)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/desktop-icon
                   :state {:item {:label "Loose"}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto icon = session.active_mode->children[0];
  ASSERT_NE(icon, nullptr);
  ASSERT_EQ(icon->children.size(), 2u);
  auto selected = Roo::Dict::get_property(icon->state, Roo::keyword("selected"));
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->to_string(), "false");
}

TEST_F(IconTest, desktop_icon_item_values_replace_previous_derived_values)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/desktop-icon
                   :state {:item {:label "Fresh"
                                  :image :items/fresh}
                           :label "Stale"
                           :image :items/stale}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto icon = session.active_mode->children[0];
  ASSERT_NE(icon, nullptr);

  auto label_value = Roo::Dict::get_property(icon->state, Roo::keyword("label"));
  auto image_value = Roo::Dict::get_property(icon->state, Roo::keyword("image"));
  ASSERT_NE(label_value, nullptr);
  ASSERT_NE(image_value, nullptr);
  EXPECT_EQ(label_value->to_string(), "\"Fresh\"");
  EXPECT_EQ(image_value->to_string(), ":items/fresh");
}

TEST_F(IconTest, make_desktop_icon_functions_construct_icon_and_preview)
{
  runtime.eval(R"(
    (pixils/defcomponent test/desktop-icon-image
      {:style {:width 20
               :height 10}})

    (pixils/defmode root-mode
      {:children [(pixils.ui.desktop-icon/make-desktop-icon
                   {:item {:id :disk
                           :label "Disk"
                           :image :items/disk}
                    :selected-id :disk
                    :icon {:mode 'test/desktop-icon-image
                           :state-keys [:item]}})
                  (pixils.ui.desktop-icon/make-desktop-icon-preview
                   {:item {:id :dragged
                           :label "Dragged"
                           :image :items/dragged}
                    :selected? true
                    :position {:x 12 :y 14}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto icon = session.active_mode->children[0];
  auto preview = session.active_mode->children[1];
  ASSERT_NE(icon, nullptr);
  ASSERT_NE(preview, nullptr);
  ASSERT_NE(icon->mode, nullptr);
  ASSERT_NE(preview->mode, nullptr);
  EXPECT_EQ(icon->mode->name, "ui/desktop-icon");
  EXPECT_EQ(preview->mode->name, "ui/desktop-icon-preview");
  EXPECT_EQ(preview->bounds.x, 12);
  EXPECT_EQ(preview->bounds.y, 14);

  ASSERT_EQ(icon->children.size(), 2u);
  auto image = icon->children[0];
  ASSERT_NE(image, nullptr);
  ASSERT_NE(image->mode, nullptr);
  EXPECT_EQ(image->mode->name, "test/desktop-icon-image");

  auto icon_item = Roo::Dict::get_property(icon->state, Roo::keyword("item"));
  auto image_item = Roo::Dict::get_property(image->state, Roo::keyword("item"));
  auto selected_id = Roo::Dict::get_property(icon->state, Roo::keyword("selected-id"));
  auto preview_selected =
    Roo::Dict::get_property(preview->state, Roo::keyword("selected"));
  ASSERT_NE(icon_item, nullptr);
  ASSERT_NE(image_item, nullptr);
  ASSERT_NE(selected_id, nullptr);
  ASSERT_NE(preview_selected, nullptr);
  EXPECT_EQ(selected_id->to_string(), ":disk");
  EXPECT_EQ(image_item->to_string(), icon_item->to_string());
  EXPECT_EQ(preview_selected->to_string(), "true");
}

TEST_F(IconTest, make_desktop_icon_can_be_wrapped_with_drag_behavior)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.drag/make-draggable
                   (pixils.ui.desktop-icon/make-desktop-icon
                    {:item {:id :disk
                            :label "Disk"}})
                   {:start-event :test/drag-start
                    :move-event :test/drag-move
                    :end-event :test/drag-end})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto icon = session.active_mode->children[0];
  ASSERT_NE(icon, nullptr);
  ASSERT_NE(icon->mode, nullptr);
  EXPECT_EQ(icon->mode->name, "ui/desktop-icon");
  ASSERT_TRUE(icon->mode->drag.has_value());
  ASSERT_NE(icon->mode->on_drag_start, nullptr);
  ASSERT_NE(icon->mode->on_drag, nullptr);
  ASSERT_NE(icon->mode->on_drag_end, nullptr);
  ASSERT_EQ(icon->children.size(), 2u);
}

TEST_F(IconTest, make_desktop_icon_preview_preserves_bound_state_with_label_options)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               {:drag-preview {:item {:id :disk
                                      :label "Disk"}
                               :position {:x 8 :y 9}}})
       :children [(pixils.ui.desktop-icon/make-desktop-icon-preview
                   {:state (pixils.ui/bind-state :drag-preview)
                    :label {:style {:height 12}}})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto preview = session.active_mode->children[0];
  ASSERT_NE(preview, nullptr);
  auto item = Roo::Dict::get_property(preview->state, Roo::keyword("item"));
  auto label = Roo::Dict::get_property(preview->state, Roo::keyword("label"));
  ASSERT_NE(item, nullptr);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->to_string(), "\"Disk\"");
  EXPECT_EQ(preview->bounds.x, 8);
  EXPECT_EQ(preview->bounds.y, 9);
}

TEST_F(IconTest, make_draggable_adds_drag_policy_to_arbitrary_child)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:start nil :move nil :end nil})
       :style {:width 100
               :height 40}
       :on {:test/drag-start (fn [state event ctx]
                               (assoc state :start (:payload event)))
            :test/drag-move (fn [state event ctx]
                              (assoc state :move (:payload event)))
            :test/drag-end (fn [state event ctx]
                             (assoc state :end true))}
       :children [(pixils.ui.drag/make-draggable
                   {:style {:width 30
                            :height 20}
                    :state {:item {:id :disk}}}
                   {:threshold 4
                    :start-event :test/drag-start
                    :move-event :test/drag-move
                    :end-event :test/drag-end})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->mode, nullptr);
  ASSERT_TRUE(child->mode->drag.has_value());
  ASSERT_NE(child->mode->on_drag_start, nullptr);
  ASSERT_NE(child->mode->on_drag, nullptr);
  ASSERT_NE(child->mode->on_drag_end, nullptr);
  ASSERT_NE(child->mode->on_drag_start->type, Roo::Value::Type::NIL);
  ASSERT_NE(child->mode->on_drag->type, Roo::Value::Type::NIL);
  ASSERT_NE(child->mode->on_drag_end->type, Roo::Value::Type::NIL);
  ASSERT_EQ(child->bounds.x, 0);
  ASSERT_EQ(child->bounds.y, 0);
  ASSERT_EQ(child->bounds.w, 30);
  ASSERT_EQ(child->bounds.h, 20);

  input().mouse_down({5, 5});
  update_cycle();

  input().mouse_move({8, 5});
  update_cycle();
  auto start =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("start"));
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->type, Roo::Value::Type::NIL);

  input().mouse_move({12, 5});
  update_cycle();
  input().mouse_move({15, 8});
  update_cycle();
  input().mouse_up({15, 8});
  update_cycle();
  update_cycle();

  start = Roo::Dict::get_property(session.active_mode->state, Roo::keyword("start"));
  auto move =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("move"));
  auto end = Roo::Dict::get_property(session.active_mode->state, Roo::keyword("end"));
  ASSERT_NE(start, nullptr);
  ASSERT_NE(move, nullptr);
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(start->to_string(),
            "{:item {:id :disk} :offset {:x 5 :y 5} :position {:x 7 :y 0}}");
  EXPECT_EQ(move->to_string(), "{:position {:x 15 :y 8}}");
  EXPECT_EQ(end->to_string(), "true");
}
