#include "../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

class ViewScaleGeometryTest : public RenderFixture
{
 protected:
  void push_and_render_root()
  {
    session.push_mode("root-mode", Roo::Constant::NIL);
    render_cycle();
  }
};

namespace
{
  Roo::sptr_val state_key(const std::shared_ptr<Pixils::Runtime::View>& view,
                          const std::string& key)
  {
    return Roo::Dict::get_property(view->state, Roo::keyword(key));
  }

  int map_int(const Roo::sptr_val& map, const std::string& key)
  {
    auto value = Roo::Dict::get_property(map, Roo::keyword(key));
    if (!value) return 0;
    return value->num().get_int();
  }
} // namespace

TEST_F(ViewScaleGeometryTest, scaled_child_keeps_logical_bounds_and_scales_external_bounds)
{
  runtime.eval(R"(
    (pixils/defmode scaled-child
      {:style {:width 100 :height 50 :scale 2}})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-child :id "scaled"}]})
  )");

  push_and_render_root();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);

  EXPECT_EQ(child->bounds.x, 0);
  EXPECT_EQ(child->bounds.y, 0);
  EXPECT_EQ(child->bounds.w, 100);
  EXPECT_EQ(child->bounds.h, 50);
  EXPECT_EQ(child->external_bounds.x, 0);
  EXPECT_EQ(child->external_bounds.y, 0);
  EXPECT_EQ(child->external_bounds.w, 200);
  EXPECT_EQ(child->external_bounds.h, 100);
  EXPECT_EQ(child->visual_bounds.x, 0);
  EXPECT_EQ(child->visual_bounds.y, 0);
  EXPECT_EQ(child->visual_bounds.w, 200);
  EXPECT_EQ(child->visual_bounds.h, 100);
  EXPECT_EQ(child->visual_scale, 2);
}

TEST_F(ViewScaleGeometryTest, flow_layout_positions_sibling_after_scaled_child_external_size)
{
  runtime.eval(R"(
    (pixils/defmode scaled-child
      {:style {:width 40 :height 20 :scale 2}})

    (pixils/defmode normal-child
      {:style {:width 20 :height 20}})

    (pixils/defmode root-mode
      {:style {:layout {:direction :row}}
       :children [{:mode 'scaled-child :id "scaled"}
                  {:mode 'normal-child :id "normal"}]})
  )");

  push_and_render_root();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto scaled = session.active_mode->children[0];
  auto normal = session.active_mode->children[1];
  ASSERT_NE(scaled, nullptr);
  ASSERT_NE(normal, nullptr);

  EXPECT_EQ(scaled->bounds.x, 0);
  EXPECT_EQ(scaled->bounds.w, 40);
  EXPECT_EQ(scaled->external_bounds.w, 80);
  EXPECT_EQ(normal->bounds.x, 80);
  EXPECT_EQ(normal->bounds.w, 20);
  EXPECT_EQ(normal->external_bounds.w, 20);
  EXPECT_EQ(scaled->visual_bounds.x, 0);
  EXPECT_EQ(scaled->visual_bounds.w, 80);
  EXPECT_EQ(scaled->visual_scale, 2);
  EXPECT_EQ(normal->visual_bounds.x, 80);
  EXPECT_EQ(normal->visual_bounds.w, 20);
  EXPECT_EQ(normal->visual_scale, 1);
}

TEST_F(ViewScaleGeometryTest, scaled_root_visual_bounds_match_buffer_space)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:style {:scale 2}
       :children [{:mode 'child-mode :id "child"}]})

    (pixils/defmode child-mode
      {:style {:width :fill :height :fill}})
  )");

  push_and_render_root();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->bounds.x, 0);
  EXPECT_EQ(session.active_mode->bounds.y, 0);
  EXPECT_EQ(session.active_mode->bounds.w, 160);
  EXPECT_EQ(session.active_mode->bounds.h, 100);
  EXPECT_EQ(session.active_mode->external_bounds.w, 320);
  EXPECT_EQ(session.active_mode->external_bounds.h, 200);
  EXPECT_EQ(session.active_mode->visual_bounds.x, 0);
  EXPECT_EQ(session.active_mode->visual_bounds.y, 0);
  EXPECT_EQ(session.active_mode->visual_bounds.w, 320);
  EXPECT_EQ(session.active_mode->visual_bounds.h, 200);
  EXPECT_EQ(session.active_mode->visual_scale, 2);

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->bounds.w, 160);
  EXPECT_EQ(child->bounds.h, 100);
  EXPECT_EQ(child->visual_bounds.w, 320);
  EXPECT_EQ(child->visual_bounds.h, 200);
  EXPECT_EQ(child->visual_scale, 2);
}

TEST_F(ViewScaleGeometryTest, scaled_parent_child_bounds_remain_logical_inside_parent)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:style {:position :absolute
               :left 10
               :top 5
               :width 20
               :height 10}})

    (pixils/defmode scaled-parent
      {:style {:position :absolute
               :left 20
               :top 30
               :width 100
               :height 60
               :scale 2}
       :children [{:mode 'child-mode :id "child"}]})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-parent :id "parent"}]})
  )");

  push_and_render_root();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto parent = session.active_mode->children[0];
  ASSERT_NE(parent, nullptr);
  ASSERT_EQ(parent->children.size(), 1u);
  auto child = parent->children[0];
  ASSERT_NE(child, nullptr);

  EXPECT_EQ(parent->bounds.x, 20);
  EXPECT_EQ(parent->bounds.y, 30);
  EXPECT_EQ(parent->bounds.w, 100);
  EXPECT_EQ(parent->bounds.h, 60);
  EXPECT_EQ(parent->external_bounds.x, 20);
  EXPECT_EQ(parent->external_bounds.y, 30);
  EXPECT_EQ(parent->external_bounds.w, 200);
  EXPECT_EQ(parent->external_bounds.h, 120);
  EXPECT_EQ(parent->visual_bounds.x, 20);
  EXPECT_EQ(parent->visual_bounds.y, 30);
  EXPECT_EQ(parent->visual_bounds.w, 200);
  EXPECT_EQ(parent->visual_bounds.h, 120);
  EXPECT_EQ(parent->visual_scale, 2);

  EXPECT_EQ(child->bounds.x, 30);
  EXPECT_EQ(child->bounds.y, 35);
  EXPECT_EQ(child->bounds.w, 20);
  EXPECT_EQ(child->bounds.h, 10);
  EXPECT_EQ(child->external_bounds.x, 30);
  EXPECT_EQ(child->external_bounds.y, 35);
  EXPECT_EQ(child->external_bounds.w, 20);
  EXPECT_EQ(child->external_bounds.h, 10);
  EXPECT_EQ(child->visual_bounds.x, 40);
  EXPECT_EQ(child->visual_bounds.y, 40);
  EXPECT_EQ(child->visual_bounds.w, 40);
  EXPECT_EQ(child->visual_bounds.h, 20);
  EXPECT_EQ(child->visual_scale, 2);
}

TEST_F(ViewScaleGeometryTest, nested_scaled_views_multiply_visual_scale)
{
  runtime.eval(R"(
    (pixils/defmode grandchild-mode
      {:style {:position :absolute
               :left 4
               :top 3
               :width 10
               :height 5
               :scale 3}})

    (pixils/defmode scaled-parent
      {:style {:position :absolute
               :left 20
               :top 30
               :width 100
               :height 60
               :scale 2}
       :children [{:mode 'grandchild-mode :id "grandchild"}]})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-parent :id "parent"}]})
  )");

  push_and_render_root();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto parent = session.active_mode->children[0];
  ASSERT_NE(parent, nullptr);
  ASSERT_EQ(parent->children.size(), 1u);
  auto grandchild = parent->children[0];
  ASSERT_NE(grandchild, nullptr);

  EXPECT_EQ(parent->visual_scale, 2);
  EXPECT_EQ(grandchild->visual_scale, 6);
  EXPECT_EQ(grandchild->bounds.x, 24);
  EXPECT_EQ(grandchild->bounds.y, 33);
  EXPECT_EQ(grandchild->bounds.w, 10);
  EXPECT_EQ(grandchild->bounds.h, 5);
  EXPECT_EQ(grandchild->external_bounds.x, 24);
  EXPECT_EQ(grandchild->external_bounds.y, 33);
  EXPECT_EQ(grandchild->external_bounds.w, 30);
  EXPECT_EQ(grandchild->external_bounds.h, 15);
  EXPECT_EQ(grandchild->visual_bounds.x, 28);
  EXPECT_EQ(grandchild->visual_bounds.y, 36);
  EXPECT_EQ(grandchild->visual_bounds.w, 60);
  EXPECT_EQ(grandchild->visual_bounds.h, 30);
}

TEST_F(ViewScaleGeometryTest, scaled_parent_with_offset_hit_tests_child_at_visual_position)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:style {:position :absolute
               :left 10
               :top 5
               :width 20
               :height 10}
       :on-mouse-down (fn [state event ctx]
                        {:x (:x (:position event))
                         :y (:y (:position event))})})

    (pixils/defmode scaled-parent
      {:style {:position :absolute
               :left 20
               :top 30
               :width 100
               :height 60
               :scale 2}
       :children [{:mode 'child-mode :id "child"}]})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-parent :id "parent"}]})
  )");

  push_and_render_root();

  input().mouse_down({50, 44});
  update_cycle();

  auto parent = session.active_mode->children[0];
  auto child = parent->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->state->to_string(), "{:x 5 :y 2}");
}

TEST_F(ViewScaleGeometryTest, scaled_child_receives_logical_local_mouse_position)
{
  runtime.eval(R"(
    (pixils/defmode scaled-child
      {:style {:width 100 :height 50 :scale 2}
       :on-mouse-down (fn [state event ctx]
                        {:x (:x (:position event))
                         :y (:y (:position event))})})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-child :id "child"}]})
  )");

  push_and_render_root();

  input().mouse_down({150, 20});
  update_cycle();

  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->state->to_string(), "{:x 75 :y 10}");
}

TEST_F(ViewScaleGeometryTest, scaled_clipped_parent_rejects_hits_outside_visual_clip)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:style {:position :absolute
               :left 60
               :top 10
               :width 20
               :height 20}
       :init (fn [state ctx] {:hit false})
       :on-mouse-down (fn [state event ctx]
                        (assoc state :hit true))})

    (pixils/defmode scaled-parent
      {:style {:width 50
               :height 50
               :scale 2
               :clip true}
       :children [{:mode 'child-mode :id "child"}]})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-parent :id "parent"}]})
  )");

  push_and_render_root();

  input().mouse_down({130, 30});
  update_cycle();

  auto parent = session.active_mode->children[0];
  auto child = parent->children[0];
  ASSERT_NE(child, nullptr);
  auto hit = state_key(child, "hit");
  ASSERT_NE(hit, nullptr);
  EXPECT_EQ(hit->to_string(), "false");
}

TEST_F(ViewScaleGeometryTest, view_adapter_bounds_and_external_bounds_keep_existing_meanings)
{
  runtime.eval(R"(
    (pixils/defmode scaled-child
      {:style {:width 100 :height 50 :scale 2}
       :on-mouse-down (fn [state event ctx]
                        {:bounds (:bounds (:view ctx))
                         :external-bounds (:external-bounds (:view ctx))
                         :visual-bounds (:visual-bounds (:view ctx))
                         :visual-scale (:visual-scale (:view ctx))})})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-child :id "child"}]})
  )");

  push_and_render_root();

  input().mouse_down({150, 20});
  update_cycle();

  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  auto bounds = state_key(child, "bounds");
  auto external_bounds = state_key(child, "external-bounds");
  auto visual_bounds = state_key(child, "visual-bounds");
  auto visual_scale = state_key(child, "visual-scale");
  ASSERT_NE(bounds, nullptr);
  ASSERT_NE(external_bounds, nullptr);
  ASSERT_NE(visual_bounds, nullptr);
  ASSERT_NE(visual_scale, nullptr);

  EXPECT_EQ(map_int(bounds, "x"), 0);
  EXPECT_EQ(map_int(bounds, "y"), 0);
  EXPECT_EQ(map_int(bounds, "w"), 100);
  EXPECT_EQ(map_int(bounds, "h"), 50);

  EXPECT_EQ(map_int(external_bounds, "x"), 0);
  EXPECT_EQ(map_int(external_bounds, "y"), 0);
  EXPECT_EQ(map_int(external_bounds, "w"), 200);
  EXPECT_EQ(map_int(external_bounds, "h"), 100);

  EXPECT_EQ(map_int(visual_bounds, "x"), 0);
  EXPECT_EQ(map_int(visual_bounds, "y"), 0);
  EXPECT_EQ(map_int(visual_bounds, "w"), 200);
  EXPECT_EQ(map_int(visual_bounds, "h"), 100);
  EXPECT_EQ(visual_scale->num().get_int(), 2);
}
