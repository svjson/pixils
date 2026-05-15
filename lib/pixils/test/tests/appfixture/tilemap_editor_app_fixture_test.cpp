#include "composable_app_session_fixture.h"
#include "tilemap_editor_app_manifest.h"

#include <pixils/program.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>

namespace TilemapEditor = Pixils::Test::AppFixture::TilemapEditor;

class TilemapEditorAppFixtureTest : public ComposableAppSessionFixture
{
 protected:
  void load_tilemap_editor()
  {
    load_app(TilemapEditor::manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void layout_active_mode()
  {
    Pixils::UI::layout_view_tree(session().active_mode,
                                 {0, 0, frame_size().w, frame_size().h},
                                 pixils(),
                                 session().hook_args.render_args[1]);
  }

  void update_and_layout()
  {
    update_cycle();
    layout_active_mode();
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

  std::shared_ptr<Pixils::Runtime::View> find_descendant_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    for (const auto& child : view->children)
    {
      if (child && child->mode && child->mode->name == mode_name) return child;
      if (auto found = find_descendant_mode(child, mode_name)) return found;
    }
    return nullptr;
  }

  std::shared_ptr<Pixils::Runtime::View> find_body_containing_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/header-panel-body" &&
        find_descendant_mode(view, mode_name))
      return view;

    for (const auto& child : view->children)
    {
      if (auto found = find_body_containing_mode(child, mode_name)) return found;
    }
    return nullptr;
  }

  std::shared_ptr<Pixils::Runtime::View> find_header_panel_containing_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/header-panel" &&
        find_descendant_mode(view, mode_name))
      return view;

    for (const auto& child : view->children)
    {
      if (auto found = find_header_panel_containing_mode(child, mode_name)) return found;
    }

    return nullptr;
  }
};

TEST_F(TilemapEditorAppFixtureTest, loads_program_and_lays_out_initial_frame)
{
  set_frame_size({800, 500});
  load_tilemap_editor();

  Pixils::Program& program = load_program();

  EXPECT_EQ(program.get_name(), "tilemap-editor");
  EXPECT_EQ(program.initial_mode, "main-mode");
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "main-mode");

  ASSERT_NO_THROW(update_cycle());
  ASSERT_NO_THROW(layout_active_mode());
  EXPECT_GT(session().active_mode->bounds.w, 0);
  EXPECT_GT(session().active_mode->bounds.h, 0);
}

TEST_F(TilemapEditorAppFixtureTest, initial_layout_keeps_workspace_and_side_panel_visible)
{
  set_frame_size({800, 500});
  load_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  ASSERT_NE(session().active_mode, nullptr);
  ASSERT_EQ(session().active_mode->children.size(), 2u);
  auto workspace = session().active_mode->children[1];
  ASSERT_NE(workspace, nullptr);
  ASSERT_EQ(workspace->children.size(), 2u);

  auto map_area = workspace->children[0];
  auto side_panel = workspace->children[1];
  ASSERT_NE(map_area, nullptr);
  ASSERT_NE(side_panel, nullptr);

  EXPECT_GT(workspace->bounds.w, 0);
  EXPECT_GT(workspace->bounds.h, 0);
  EXPECT_GT(map_area->bounds.w, 0);
  EXPECT_GT(map_area->bounds.h, 0);
  EXPECT_GT(side_panel->bounds.w, 0);
  EXPECT_GT(side_panel->bounds.h, 0);
  EXPECT_GT(side_panel->bounds.x, map_area->bounds.x);
  EXPECT_LE(side_panel->bounds.x + side_panel->bounds.w,
            workspace->bounds.x + workspace->bounds.w);
}

TEST_F(TilemapEditorAppFixtureTest, toolbar_checkbox_is_laid_out_inside_toolbar)
{
  set_frame_size({800, 500});
  load_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  auto checkbox = find_first_mode(session().active_mode, "ui/checkbox");
  auto checkbox_box = find_first_mode(session().active_mode, "ui/checkbox-box");
  ASSERT_NE(checkbox, nullptr);
  ASSERT_NE(checkbox_box, nullptr);

  EXPECT_GT(checkbox->bounds.w, 0);
  EXPECT_GT(checkbox->bounds.h, 0);
  EXPECT_GT(checkbox_box->bounds.w, 0);
  EXPECT_GT(checkbox_box->bounds.h, 0);
  EXPECT_GE(checkbox->bounds.x, session().active_mode->bounds.x);
  EXPECT_GE(checkbox->bounds.y, session().active_mode->bounds.y);
  EXPECT_GT(checkbox_box->bounds.x, session().active_mode->bounds.x);
  EXPECT_GT(checkbox_box->bounds.y, session().active_mode->bounds.y);
}

TEST_F(TilemapEditorAppFixtureTest, tile_palette_scroll_pane_stays_inside_panel_body)
{
  set_frame_size({800, 500});
  load_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  auto panel_body = find_body_containing_mode(session().active_mode, "ui/scroll-pane");
  auto panel = find_header_panel_containing_mode(session().active_mode, "ui/scroll-pane");
  ASSERT_NE(panel, nullptr);
  ASSERT_NE(panel_body, nullptr);
  auto scroll_pane = find_descendant_mode(panel_body, "ui/scroll-pane");
  ASSERT_NE(scroll_pane, nullptr);

  EXPECT_LE(panel_body->bounds.x + panel_body->bounds.w, panel->bounds.x + panel->bounds.w);
  EXPECT_LE(panel_body->bounds.y + panel_body->bounds.h, panel->bounds.y + panel->bounds.h);
  EXPECT_GE(scroll_pane->bounds.x, panel_body->bounds.x);
  EXPECT_GE(scroll_pane->bounds.y, panel_body->bounds.y);
  EXPECT_LE(scroll_pane->bounds.x + scroll_pane->bounds.w,
            panel_body->bounds.x + panel_body->bounds.w);
  EXPECT_LE(scroll_pane->bounds.y + scroll_pane->bounds.h,
            panel_body->bounds.y + panel_body->bounds.h);
}
