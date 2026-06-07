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

TEST_F(IconTest, icon_container_grid_mode_positions_existing_children)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/icon-container
                   :style {:width 100
                           :height 100}
                   :state {:layout-mode :grid
                           :grid {:cell-width 20
                                  :cell-height 24
                                  :columns 2}}
                   :children [{:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :one}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :two}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :three}}}]}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto container = session.active_mode->children[0];
  ASSERT_NE(container, nullptr);
  ASSERT_EQ(container->children.size(), 3u);
  EXPECT_EQ(container->children[0]->bounds.x, 0);
  EXPECT_EQ(container->children[0]->bounds.y, 0);
  EXPECT_EQ(container->children[1]->bounds.x, 20);
  EXPECT_EQ(container->children[1]->bounds.y, 0);
  EXPECT_EQ(container->children[2]->bounds.x, 0);
  EXPECT_EQ(container->children[2]->bounds.y, 24);
}

TEST_F(IconTest, icon_container_snaps_drop_position_to_grid)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:drop nil})
       :style {:width 160
               :height 80
               :layout {:direction :row
                        :gap 10}}
       :on {:ui/icon-drop (fn [state event ctx]
                            (assoc state :drop (:payload event)))}
       :children [(pixils.ui.drag/make-draggable
                   {:style {:width 20 :height 20}
                    :state {:item {:id :disk}}}
                   {:threshold 1})
                  {:mode 'ui/icon-container
                   :style {:width 100
                           :height 80}
                   :state {:target :desktop
                           :snap-to-grid? true
                           :grid {:cell-width 20
                                  :cell-height 20}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_move({60, 31});
  update_cycle();
  input().mouse_up({60, 31});
  update_cycle();
  update_cycle();

  auto drop = Roo::Dict::get_property(session.active_mode->state, Roo::keyword("drop"));
  ASSERT_NE(drop, nullptr);
  EXPECT_EQ(drop->to_string(),
            "{:item {:id :disk} :target :desktop :position {:x 20 :y 20} "
            ":raw-position {:x 25 :y 26}}");
}

TEST_F(IconTest, icon_container_grid_reorder_emits_from_and_to_indexes)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:reorder nil})
       :style {:width 100
               :height 100
               :layout {:direction :row
                        :gap 10}}
       :on {:ui/icon-reorder (fn [state event ctx]
                               (assoc state :reorder (:payload event)))}
       :children [(pixils.ui.drag/make-draggable
                   {:style {:width 10 :height 10}
                    :state {:item {:id :one}}}
                   {:threshold 1})
                  {:mode 'ui/icon-container
                   :style {:width 100
                           :height 100}
                   :state {:target :desktop
                           :layout-mode :grid
                           :reorderable? true
                           :grid {:cell-width 20
                                  :cell-height 20
                                  :columns 2}}
                   :children [{:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :one}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :two}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :three}}}]}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({5, 5});
  update_cycle();
  input().mouse_move({45, 25});
  update_cycle();
  input().mouse_up({45, 25});
  update_cycle();
  update_cycle();

  auto reorder =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("reorder"));
  ASSERT_NE(reorder, nullptr);
  EXPECT_EQ(reorder->to_string(),
            "{:item {:id :one} :target :desktop :from-index 0 :to-index 2}");
}

TEST_F(IconTest, icon_container_keyboard_navigation_emits_select_and_activate)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:select nil :activate nil})
       :style {:width 100
               :height 100}
       :on {:ui/icon-select (fn [state event ctx]
                              (assoc state :select (:payload event)))
            :ui/icon-activate (fn [state event ctx]
                                (assoc state :activate (:payload event)))}
       :children [{:mode 'ui/icon-container
                   :style {:width 100
                           :height 100}
                   :state {:selected-id :one
                           :layout-mode :grid
                           :grid {:cell-width 20
                                  :cell-height 20
                                  :columns 2}}
                   :children [{:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :one}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :two}}}]}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  input().mouse_down({1, 1});
  update_cycle();
  input().mouse_up({1, 1});
  update_cycle();
  input().key_down(SDLK_RIGHT);
  update_cycle();
  input().key_up(SDLK_RIGHT);
  update_cycle();
  input().key_down(SDLK_RETURN);
  update_cycle();

  auto select =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("select"));
  auto activate =
    Roo::Dict::get_property(session.active_mode->state, Roo::keyword("activate"));
  ASSERT_NE(select, nullptr);
  ASSERT_NE(activate, nullptr);
  EXPECT_EQ(select->to_string(),
            "{:item {:id :two} :index 1 :id :two :input-source :keyboard}");
  EXPECT_EQ(activate->to_string(),
            "{:item {:id :two} :index 1 :id :two :input-source :keyboard}");
}

TEST_F(IconTest, icon_container_grid_content_size_uses_active_icon_count)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/icon-container
                   :style {:width 100
                           :height :auto}
                   :state {:layout-mode :grid
                           :icon-count 3
                           :grid {:cell-width 20
                                  :cell-height 24
                                  :columns 2}}
                   :children [{:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :one}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :two}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :three}}}
                              {:mode 'ui/icon
                               :style {:width 10 :height 10}
                               :state {:item {:id :empty-slot}}}]}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto container = session.active_mode->children[0];
  ASSERT_NE(container, nullptr);
  EXPECT_EQ(container->bounds.w, 100);
  EXPECT_EQ(container->bounds.h, 48);
  ASSERT_EQ(container->children.size(), 4u);
  EXPECT_EQ(container->children[0]->bounds.x, 0);
  EXPECT_EQ(container->children[0]->bounds.y, 0);
  EXPECT_EQ(container->children[1]->bounds.x, 20);
  EXPECT_EQ(container->children[1]->bounds.y, 0);
  EXPECT_EQ(container->children[2]->bounds.x, 0);
  EXPECT_EQ(container->children[2]->bounds.y, 24);
}

TEST_F(IconTest, make_grid_wraps_icon_container_in_auto_scroll_pane)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.icon-container/make-grid
                   {:style {:width 50 :height 40}
                    :state {:icon-count 3}
                    :grid {:cell-width 20
                           :cell-height 24
                           :columns 2}
                    :children [{:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item {:id :one}}}
                               {:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item {:id :two}}}
                               {:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item {:id :three}}}
                               {:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item {:id :empty-slot}}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(pane->mode, nullptr);
  EXPECT_EQ(pane->mode->name, "ui/scroll-pane");

  auto content_size =
    Roo::Dict::get_property(pane->state, Roo::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height = Roo::Dict::get_property(content_size, Roo::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 48);

  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_FALSE(row->children.empty());
  auto viewport = row->children[0];
  ASSERT_NE(viewport, nullptr);
  ASSERT_EQ(viewport->children.size(), 1u);
  auto content = viewport->children[0];
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 1u);
  auto container = content->children[0];
  ASSERT_NE(container, nullptr);
  ASSERT_NE(container->mode, nullptr);
  EXPECT_EQ(container->mode->name, "ui/icon-container");
  EXPECT_EQ(container->bounds.h, 48);
}

TEST_F(IconTest, make_grid_keeps_empty_grid_as_drop_surface)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [(pixils.ui.icon-container/make-grid
                   {:style {:width 50 :height 40}
                    :state {:icon-count 0}
                    :grid {:cell-width 20
                           :cell-height 24
                           :columns 2}
                    :children [{:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item {:id :empty-slot}}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto pane = session.active_mode->children[0];
  ASSERT_NE(pane, nullptr);
  auto content_size =
    Roo::Dict::get_property(pane->state, Roo::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height = Roo::Dict::get_property(content_size, Roo::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 24);
}

TEST_F(IconTest, make_grid_can_bind_scroll_content_state_from_owner)
{
  runtime.eval(R"(
    (pixils/defcomponent bound-grid-owner
      {:init (fn [state ctx]
               {:items [{:id :one} {:id :two} {:id :three}]})
       :children [(pixils.ui.icon-container/make-grid
                   {:style {:width 50 :height 40}
                    :bind-content-state? true
                    :state {:items (pixils.ui/bind-state :items)}
                    :grid {:cell-width 20
                           :cell-height 24
                           :columns 2}
                    :children [{:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item (pixils.ui/bind-state :items 0)}}
                               {:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item (pixils.ui/bind-state :items 1)}}
                               {:mode 'ui/icon
                                :style {:width 10 :height 10}
                                :state {:item (pixils.ui/bind-state :items 2)}}]})]})

    (pixils/defmode root-mode
      {:children [{:mode 'bound-grid-owner}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  auto owner = session.active_mode->children[0];
  ASSERT_NE(owner, nullptr);
  ASSERT_EQ(owner->children.size(), 1u);
  auto pane = owner->children[0];
  ASSERT_NE(pane, nullptr);
  auto content_size =
    Roo::Dict::get_property(pane->state, Roo::keyword("content-size"));
  ASSERT_NE(content_size, nullptr);
  auto content_height = Roo::Dict::get_property(content_size, Roo::keyword("h"));
  ASSERT_NE(content_height, nullptr);
  EXPECT_EQ(content_height->num().get_int(), 48);

  ASSERT_EQ(pane->children.size(), 1u);
  auto row = pane->children[0];
  ASSERT_NE(row, nullptr);
  ASSERT_FALSE(row->children.empty());
  auto viewport = row->children[0];
  ASSERT_NE(viewport, nullptr);
  ASSERT_EQ(viewport->children.size(), 1u);
  auto content = viewport->children[0];
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->children.size(), 1u);
  auto container = content->children[0];
  ASSERT_NE(container, nullptr);
  ASSERT_EQ(container->children.size(), 3u);

  auto first_item =
    Roo::Dict::get_property(container->children[0]->state, Roo::keyword("item"));
  ASSERT_NE(first_item, nullptr);
  EXPECT_EQ(first_item->to_string(), "{:id :one}");
}
