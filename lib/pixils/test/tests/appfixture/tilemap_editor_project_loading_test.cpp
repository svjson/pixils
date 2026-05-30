#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest, loaded_project_populates_existing_side_panel_controls)
{
  const auto history_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-startup-history.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  SDLMock::prepared_surfaces["./../assets/simples_pimples.png"] = {800, 1280};

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
                                  :path "examples/tilemap-editor/example-maps/map1.edn"
                                  :directory "examples/tilemap-editor/example-maps"
                                  :filename "map1.edn"})"));

  update_cycle();
  update_cycle();

  auto resource_width = runtime.eval("(pixils.image/width :project-assets/simples-pimples)");
  auto resource_height =
    runtime.eval("(pixils.image/height :project-assets/simples-pimples)");
  ASSERT_NE(resource_width, nullptr);
  ASSERT_NE(resource_height, nullptr);
  ASSERT_EQ(resource_width->type, Lisple::Value::Type::NUMBER);
  ASSERT_EQ(resource_height->type, Lisple::Value::Type::NUMBER);
  EXPECT_EQ(resource_width->num().get_int(), 800);
  EXPECT_EQ(resource_height->num().get_int(), 1280);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> layer_rows;
  find_descendant_modes(session.active_mode, "layer-row", layer_rows);
  EXPECT_EQ(layer_rows.size(), 4u);

  session.render_mode();

  auto tile_select_grid = find_descendant_mode_containing_state(session.active_mode,
                                                                "tile-select-grid",
                                                                "Night Sky");
  ASSERT_NE(tile_select_grid, nullptr);

  input().mouse_down({tile_select_grid->bounds.x + 21, tile_select_grid->bounds.y + 5});
  update_cycle();
  input().mouse_up({tile_select_grid->bounds.x + 21, tile_select_grid->bounds.y + 5});
  update_cycle();
  EXPECT_NE(session.active_mode->state->to_string().find(":selected-tile :cave-depth"),
            std::string::npos);
  tile_select_grid = find_descendant_mode_containing_state(session.active_mode,
                                                           "tile-select-grid",
                                                           "Cave Depth");
  ASSERT_NE(tile_select_grid, nullptr);
  EXPECT_NE(tile_select_grid->state->to_string().find(":selected-indices [1]"),
            std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> combo_boxes;
  find_descendant_modes(session.active_mode, "ui/combo-box", combo_boxes);
  ASSERT_GE(combo_boxes.size(), 1u);
  auto has_background_colors_combo = false;
  for (const auto& combo_box : combo_boxes)
  {
    has_background_colors_combo =
      has_background_colors_combo ||
      combo_box->state->to_string().find("Background Colors") != std::string::npos;
  }
  EXPECT_TRUE(has_background_colors_combo);

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_GE(tab_panel->children.size(), 1u);
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

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tileset_tab_list_boxes;
  find_descendant_modes(session.active_mode, "ui/list-box", tileset_tab_list_boxes);
  ASSERT_GE(tileset_tab_list_boxes.size(), 1u);
  EXPECT_NE(tileset_tab_list_boxes[0]->state->to_string().find("Background Colors"),
            std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tileset_tab_combo_boxes;
  find_descendant_modes(session.active_mode, "ui/combo-box", tileset_tab_combo_boxes);
  ASSERT_EQ(tileset_tab_combo_boxes.size(), 1u);
  EXPECT_NE(tileset_tab_combo_boxes[0]->state->to_string().find("Color"), std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_definition_panels;
  find_descendant_modes(session.active_mode,
                        "tile-definition-panel",
                        tile_definition_panels);
  EXPECT_EQ(tile_definition_panels.size(), 1u);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_grids;
  find_descendant_modes(session.active_mode, "tileset-tile-grid", tile_grids);
  ASSERT_EQ(tile_grids.size(), 1u);
  auto tile_grid = tile_grids[0];
  input().mouse_down({tile_grid->bounds.x + 5, tile_grid->bounds.y + 5});
  update_cycle();
  input().mouse_up({tile_grid->bounds.x + 5, tile_grid->bounds.y + 5});
  update_cycle();
  input().key_down(SDLK_RIGHT);
  update_cycle();
  EXPECT_NE(session.active_mode->state->to_string().find(":selected-tileset-tile-index 1"),
            std::string::npos);

  tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_GE(tab_panel->children.size(), 1u);
  tab_strip = tab_panel->children[0];
  ASSERT_NE(tab_strip, nullptr);
  ASSERT_GE(tab_strip->children.size(), 2u);
  auto map_tab = tab_strip->children[0];
  ASSERT_NE(map_tab, nullptr);

  input().mouse_down(
    {map_tab->bounds.x + map_tab->bounds.w / 2, map_tab->bounds.y + map_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up(
    {map_tab->bounds.x + map_tab->bounds.w / 2, map_tab->bounds.y + map_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  layer_rows.clear();
  find_descendant_modes(session.active_mode, "layer-row", layer_rows);
  EXPECT_EQ(layer_rows.size(), 4u);

  tile_select_grid = find_descendant_mode_containing_state(session.active_mode,
                                                           "tile-select-grid",
                                                           "Night Sky");
  ASSERT_NE(tile_select_grid, nullptr);

  std::filesystem::remove(history_path, ec);
}

TEST_F(TilemapEditorStartupTest, direct_project_load_keeps_existing_palette_interactive)
{
  const auto history_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-direct-load-history.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  SDLMock::prepared_surfaces["./../assets/simples_pimples.png"] = {800, 1280};

  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  update_cycle();
  session.render_mode();
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("project-history-path"),
                             Lisple::string(history_path.string()));
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("recent-projects"),
                             Lisple::vector({}));

  auto payload = runtime.eval(R"({:type :confirm
                                  :mode :file-dialog/open
                                  :path "examples/tilemap-editor/example-maps/map1.edn"
                                  :directory "examples/tilemap-editor/example-maps"
                                  :filename "map1.edn"})");
  Lisple::sptr_val_v args{session.active_mode->state, payload};
  session.active_mode->state =
    runtime.invoke("tilemap-editor.io.project/apply-project-file-dialog-result", args);
  update_cycle();
  update_cycle();
  session.render_mode();

  auto tile_select_grid = find_descendant_mode_containing_state(session.active_mode,
                                                                "tile-select-grid",
                                                                "Night Sky");
  ASSERT_NE(tile_select_grid, nullptr);

  input().mouse_down({tile_select_grid->bounds.x + 21, tile_select_grid->bounds.y + 5});
  update_cycle();
  input().mouse_up({tile_select_grid->bounds.x + 21, tile_select_grid->bounds.y + 5});
  update_cycle();

  EXPECT_NE(session.active_mode->state->to_string().find(":selected-tile :cave-depth"),
            std::string::npos);
  tile_select_grid = find_descendant_mode_containing_state(session.active_mode,
                                                           "tile-select-grid",
                                                           "Cave Depth");
  ASSERT_NE(tile_select_grid, nullptr);
  EXPECT_NE(tile_select_grid->state->to_string().find(":selected-indices [1]"),
            std::string::npos);

  std::filesystem::remove(history_path, ec);
}
