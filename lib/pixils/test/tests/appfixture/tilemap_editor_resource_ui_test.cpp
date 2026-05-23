#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest,
       loaded_project_with_resources_opens_resource_and_tileset_tabs)
{
  const auto history_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-resource-history.edn";
  const auto project_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-resource-project.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);

  {
    std::ofstream out(project_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {:editor-assets
                       {:images {:spritesheet
                                 {:file-name "assets/simples_pimples.png"}}}
                       :project-assets
                       {:images {:spritesheet
                                 {:file-name "examples/tilemap-editor/assets/simples_pimples.png"
                                  :name "Spritesheet"}}}}}
 :tilesets [{:id :loaded
             :label "Loaded"
             :tile-size 16
             :tiles [{:id :sprite
                      :name "Sprite"
                      :char "s"
                      :type :sprite
                      :image :project-assets/spritesheet
                      :source {:x 0 :y 0 :w 16 :h 16}}]}]
 :tilemap {:width 2
           :height 2
           :tile-size 16
           :layers []}})";
  }

  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  update_cycle();
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("project-history-path"),
                             Lisple::string(history_path.string()));
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("recent-projects"),
                             Lisple::vector({}));

  auto origin = Lisple::map({Lisple::keyword("view"),
                             Pixils::Script::ViewAdapter::make_ref(*session.active_mode),
                             Lisple::keyword("event"),
                             Lisple::keyword("project/file-dialog-result")});
  auto overrides = Lisple::map({Lisple::keyword("origin"), origin});
  session.push_mode("ui/tab-panel-empty", Lisple::Constant::NIL, overrides);
  session.pop_mode(runtime.eval(R"({:type :confirm
                                  :mode :file-dialog/open
                                  :path )" +
                                lisp_string(project_path.string()) + R"(
                                  :directory )" +
                                lisp_string(project_path.parent_path().string()) + R"(
                                  :filename "resource-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_GE(tab_panel->children.size(), 1u);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_GE(tab_strip->children.size(), 4u);

  auto resources_tab = tab_strip->children[3];
  ASSERT_NE(resources_tab, nullptr);
  input().mouse_down({resources_tab->bounds.x + resources_tab->bounds.w / 2,
                      resources_tab->bounds.y + resources_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({resources_tab->bounds.x + resources_tab->bounds.w / 2,
                    resources_tab->bounds.y + resources_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  std::vector<std::shared_ptr<Pixils::Runtime::View>> resource_lists;
  find_descendant_modes(session.active_mode, "resource-bundle-list", resource_lists);
  ASSERT_EQ(resource_lists.size(), 1u);
  std::vector<std::shared_ptr<Pixils::Runtime::View>> bundle_rows;
  find_descendant_modes(session.active_mode, "resource-bundle-row", bundle_rows);
  ASSERT_EQ(bundle_rows.size(), 1u);
  EXPECT_NE(bundle_rows[0]->state->to_string().find("project-assets"), std::string::npos);
  EXPECT_EQ(bundle_rows[0]->state->to_string().find("editor-assets"), std::string::npos);

  tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  tab_strip = tab_panel->children[0];
  auto tilesets_tab = tab_strip->children[1];
  ASSERT_NE(tilesets_tab, nullptr);
  input().mouse_down({tilesets_tab->bounds.x + tilesets_tab->bounds.w / 2,
                      tilesets_tab->bounds.y + tilesets_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({tilesets_tab->bounds.x + tilesets_tab->bounds.w / 2,
                    tilesets_tab->bounds.y + tilesets_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_grids;
  find_descendant_modes(session.active_mode, "tileset-tile-grid", tile_grids);
  ASSERT_EQ(tile_grids.size(), 1u);
  EXPECT_NE(tile_grids[0]->state->to_string().find(":project-assets/spritesheet"),
            std::string::npos);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}
