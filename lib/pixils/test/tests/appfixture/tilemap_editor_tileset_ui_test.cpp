#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest, tileset_row_context_menu_opens_edit_dialog)
{
  const auto history_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-tileset-context-history.edn";
  const auto project_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-tileset-context-project.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);

  {
    std::ofstream out(project_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {}}
 :tilesets [{:id :loaded
             :label "Loaded"
             :tile-size 16
             :tiles [{:id :grass
                      :name "Grass"
                      :char "g"
                      :type :color
                      :color {:r 0 :g 128 :b 0}}]}]
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
                                  :filename "tileset-context-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_GE(tab_strip->children.size(), 2u);
  auto tilesets_tab = tab_strip->children[1];
  ASSERT_NE(tilesets_tab, nullptr);
  input().mouse_down({tilesets_tab->bounds.x + tilesets_tab->bounds.w / 2,
                      tilesets_tab->bounds.y + tilesets_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({tilesets_tab->bounds.x + tilesets_tab->bounds.w / 2,
                    tilesets_tab->bounds.y + tilesets_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  std::vector<std::shared_ptr<Pixils::Runtime::View>> rows;
  find_descendant_modes(session.active_mode, "tileset-definition-row", rows);
  ASSERT_EQ(rows.size(), 1u);
  auto row = rows[0];
  input().mouse_down({row->bounds.x + row->bounds.w / 2, row->bounds.y + row->bounds.h / 2},
                     SDL_BUTTON_RIGHT);
  update_cycle();
  input().mouse_up({row->bounds.x + row->bounds.w / 2, row->bounds.y + row->bounds.h / 2},
                   SDL_BUTTON_RIGHT);
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "ui/context-menu");

  std::vector<std::shared_ptr<Pixils::Runtime::View>> menu_items;
  find_descendant_modes(session.active_mode, "ui/popup-menu-item", menu_items);
  ASSERT_EQ(menu_items.size(), 1u);
  auto edit_item = menu_items[0];
  input().mouse_down({edit_item->bounds.x + edit_item->bounds.w / 2,
                      edit_item->bounds.y + edit_item->bounds.h / 2});
  update_cycle();
  input().mouse_up({edit_item->bounds.x + edit_item->bounds.w / 2,
                    edit_item->bounds.y + edit_item->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "tileset-edit-dialog-modal");
  const std::string dialog_state = session.active_mode->state->to_string();
  EXPECT_NE(dialog_state.find(R"(:tileset-id "loaded")"), std::string::npos);
  EXPECT_NE(dialog_state.find(R"(:tileset-label "Loaded")"), std::string::npos);
  EXPECT_NE(dialog_state.find(":tile-size 16"), std::string::npos);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}

TEST_F(TilemapEditorStartupTest, tileset_tile_grid_drag_selects_and_delete_prompts)
{
  const auto history_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-tile-delete-history.edn";
  const auto project_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-tile-delete-project.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);

  {
    std::ofstream out(project_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {}}
 :tilesets [{:id :terrain
             :label "Terrain"
             :tile-size 16
             :tiles [{:id :grass
                      :name "Grass"
                      :char "g"
                      :type :color
                      :color {:r 0 :g 128 :b 0}}
                     {:id :water
                      :name "Water"
                      :char "w"
                      :type :color
                      :color {:r 0 :g 0 :b 255}}
                     {:id :lava
                      :name "Lava"
                      :char "l"
                      :type :color
                      :color {:r 255 :g 64 :b 0}}]}]
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
                                  :filename "tile-delete-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  auto tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_GE(tab_strip->children.size(), 2u);
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
  auto tile_grid = tile_grids[0];
  const float first_x = tile_grid->bounds.x + 5;
  const float second_x = tile_grid->bounds.x + 21;
  const float tile_y = tile_grid->bounds.y + 5;

  input().mouse_down({first_x, tile_y});
  update_cycle();
  input().mouse_move({second_x, tile_y});
  update_cycle();
  input().mouse_up({second_x, tile_y});
  update_cycle();
  session.render_mode();

  EXPECT_NE(
    session.active_mode->state->to_string().find(":selected-tileset-tile-indices [0 1]"),
    std::string::npos);

  input().key_down(SDLK_DELETE);
  update_cycle();
  update_cycle();
  session.render_mode();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  EXPECT_NE(session.active_mode->state->to_string().find(":operation :delete-tiles"),
            std::string::npos);
  EXPECT_NE(session.active_mode->state->to_string().find(":tile-indices [0 1]"),
            std::string::npos);

  session.pop_mode(runtime.eval("{:choice :dialog/cancel}"));
  update_cycle();
  session.render_mode();

  tile_grids.clear();
  find_descendant_modes(session.active_mode, "tileset-tile-grid", tile_grids);
  ASSERT_EQ(tile_grids.size(), 1u);
  tile_grid = tile_grids[0];
  input().mouse_down({tile_grid->bounds.x + 21, tile_grid->bounds.y + 5}, SDL_BUTTON_RIGHT);
  update_cycle();
  input().mouse_up({tile_grid->bounds.x + 21, tile_grid->bounds.y + 5}, SDL_BUTTON_RIGHT);
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "ui/context-menu");

  std::vector<std::shared_ptr<Pixils::Runtime::View>> menu_items;
  find_descendant_modes(session.active_mode, "ui/popup-menu-item", menu_items);
  ASSERT_EQ(menu_items.size(), 1u);
  auto delete_item = menu_items[0];
  input().mouse_down({delete_item->bounds.x + delete_item->bounds.w / 2,
                      delete_item->bounds.y + delete_item->bounds.h / 2});
  update_cycle();
  input().mouse_up({delete_item->bounds.x + delete_item->bounds.w / 2,
                    delete_item->bounds.y + delete_item->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  EXPECT_NE(session.active_mode->state->to_string().find(":operation :delete-tiles"),
            std::string::npos);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}
