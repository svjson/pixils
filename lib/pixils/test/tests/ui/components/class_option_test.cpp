#include "../../runtime/session_fixture.h"

#include <algorithm>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <pixils/runtime/view.h>

using ClassOptionTest = SessionFixture;

namespace
{
  bool has_class(const std::shared_ptr<Pixils::Runtime::View>& view,
                 const std::string& class_name)
  {
    return view && view->mode &&
           std::find(view->mode->class_names.begin(),
                     view->mode->class_names.end(),
                     class_name) != view->mode->class_names.end();
  }
} // namespace

TEST_F(ClassOptionTest, public_make_functions_apply_class_to_outer_component)
{
  runtime.eval(R"(
    (def menu-definition
      {:items [{:label "File"}]})

    (pixils/defmode root-mode
      {:children [(pixils.ui.slider/make
                   {:class :class-test/slider})
                  (pixils.ui.checkbox/make
                   {:class :class-test/checkbox
                    :label "Check"})
                  (pixils.ui.option-box/make
                   {:class :class-test/option-box
                    :label "Option"})
                  (pixils.ui.option-box/make-group
                   {:class :class-test/option-box-group
                    :options [{:label "One" :value :one}]})
                  (pixils.ui.toggle-button/make
                   {:class :class-test/toggle-button
                    :label "Toggle"})
                  (pixils.ui.toggle-button/make-group
                   {:class :class-test/toggle-button-group
                    :buttons [{:label "One" :value :one}]})
                  (pixils.ui.combo-box/make
                   {:class :class-test/combo-box
                    :options [{:label "One" :value :one}]})
                  (pixils.ui.list-box/make
                   {:class :class-test/list-box
                    :options [{:label "One" :value :one}]})
                  (pixils.ui.tree-view/make
                   {:class :class-test/tree-view
                    :roots [{:id :root :label "Root"}]})
                  (pixils.ui.file-tree/make
                   {:class :class-test/file-tree
                    :roots [{:id :root :label "Root"}]})
                  (pixils.ui.number-input/make
                   {:class :class-test/number-input
                    :value 1})
                  (pixils.ui.tab-panel/make
                   {:class :class-test/tab-panel
                    :tabs [{:id :one
                            :title "One"
                            :children []}]})
                  (pixils.ui.split-pane/make
                   {:class :class-test/split-pane
                    :children [[] []]})
                  (pixils.ui.header-panel/make
                   {:class :class-test/header-panel
                    :title "Header"
                    :children []})
                  (pixils.ui.desktop-icon/make-desktop-icon
                   {:class :class-test/desktop-icon
                    :label "Icon"})
                  (pixils.ui.desktop-icon/make-desktop-icon-preview
                   {:class :class-test/desktop-icon-preview
                    :label "Preview"})
                  (pixils.ui.icon-container/make-grid
                   {:class :class-test/icon-grid
                    :grid {:cell-width 20
                           :cell-height 20
                           :columns 1}
                    :children []})
                  (pixils.ui.menu/make-menu
                   {:class :class-test/menu}
                   menu-definition
                   {})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 18u);
  EXPECT_TRUE(has_class(session.active_mode->children[0], "class-test/slider"));
  EXPECT_TRUE(has_class(session.active_mode->children[1], "class-test/checkbox"));
  EXPECT_TRUE(has_class(session.active_mode->children[2], "class-test/option-box"));
  EXPECT_TRUE(has_class(session.active_mode->children[3],
                        "class-test/option-box-group"));
  EXPECT_TRUE(has_class(session.active_mode->children[4], "class-test/toggle-button"));
  EXPECT_TRUE(has_class(session.active_mode->children[5],
                        "class-test/toggle-button-group"));
  EXPECT_TRUE(has_class(session.active_mode->children[6], "class-test/combo-box"));
  EXPECT_TRUE(has_class(session.active_mode->children[7], "class-test/list-box"));
  EXPECT_TRUE(has_class(session.active_mode->children[8], "class-test/tree-view"));
  EXPECT_TRUE(has_class(session.active_mode->children[9], "class-test/file-tree"));
  EXPECT_TRUE(has_class(session.active_mode->children[10],
                        "class-test/number-input"));
  EXPECT_TRUE(has_class(session.active_mode->children[11], "class-test/tab-panel"));
  EXPECT_TRUE(has_class(session.active_mode->children[12], "class-test/split-pane"));
  EXPECT_TRUE(has_class(session.active_mode->children[13],
                        "class-test/header-panel"));
  EXPECT_TRUE(has_class(session.active_mode->children[14],
                        "class-test/desktop-icon"));
  EXPECT_TRUE(has_class(session.active_mode->children[15],
                        "class-test/desktop-icon-preview"));
  EXPECT_TRUE(has_class(session.active_mode->children[16], "class-test/icon-grid"));
  EXPECT_TRUE(has_class(session.active_mode->children[17], "class-test/menu"));
}
