#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest, terrain_rule_add_button_creates_visible_rule)
{
  const auto history_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-terrain-rule-history.edn";
  const auto project_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-terrain-rule-project.edn";
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
             :tiles [{:id :grass-fill
                      :name "Grass Fill"
                      :char "g"
                      :type :color
                      :color {:r 0 :g 255 :b 0}}
                     {:id :water-fill
                      :name "Water Fill"
                      :char "w"
                      :type :color
                      :color {:r 0 :g 0 :b 255}}]}]
 :terrain-sets [{:id :overworld
                 :label "Overworld"
                 :tileset :terrain
                 :terrains [{:id :grass
                             :label "Grass"
                             :tile :grass-fill}
                            {:id :water
                             :label "Water"
                             :tile :water-fill}]}]
 :rulesets []
 :layer-profiles [{:id :default
                   :label "Default"
                   :layers [{:id :scene/terrain
                             :label "Terrain Source"
                             :kind :terrain
                             :data-kind :terrain
                             :terrain-set :overworld}
                            {:id :scene/terrain-visuals
                             :label "Terrain Visuals"
                             :kind :tile
                             :data-kind :tile-ref
                             :tileset :terrain}]}]
 :tilemap {:width 2
           :height 2
           :tile-size 16
           :layer-profile :default
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
                                  :filename "terrain-rule-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  auto tab_strip = tab_panel->children[0];
  ASSERT_GE(tab_strip->children.size(), 5u);

  auto terrains_tab = tab_strip->children[4];
  ASSERT_NE(terrains_tab, nullptr);
  input().mouse_down({terrains_tab->bounds.x + terrains_tab->bounds.w / 2,
                      terrains_tab->bounds.y + terrains_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({terrains_tab->bounds.x + terrains_tab->bounds.w / 2,
                    terrains_tab->bounds.y + terrains_tab->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  auto terrain_detail_tab = find_descendant_mode_containing_state(session.active_mode,
                                                                  "ui/tab",
                                                                  ":label \"Terrain\"");
  ASSERT_NE(terrain_detail_tab, nullptr);
  input().mouse_down({terrain_detail_tab->bounds.x + terrain_detail_tab->bounds.w / 2,
                      terrain_detail_tab->bounds.y + terrain_detail_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({terrain_detail_tab->bounds.x + terrain_detail_tab->bounds.w / 2,
                    terrain_detail_tab->bounds.y + terrain_detail_tab->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  auto add_rule_button =
    find_descendant_mode_containing_state(session.active_mode, "ui/button", "Add rule");
  ASSERT_NE(add_rule_button, nullptr);
  input().mouse_down({add_rule_button->bounds.x + add_rule_button->bounds.w / 2,
                      add_rule_button->bounds.y + add_rule_button->bounds.h / 2});
  update_cycle();
  input().mouse_up({add_rule_button->bounds.x + add_rule_button->bounds.w / 2,
                    add_rule_button->bounds.y + add_rule_button->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  EXPECT_NE(session.active_mode->state->to_string().find(":overworld-rules"),
            std::string::npos);
  EXPECT_NE(session.active_mode->state->to_string().find(":selected-terrain-rule-id :rule"),
            std::string::npos);

  auto rule_list =
    find_descendant_mode_containing_state(session.active_mode, "terrain-rule-list", ":rule");
  EXPECT_NE(rule_list, nullptr);
  auto rule_editor = find_descendant_mode_containing_state(session.active_mode,
                                                           "terrain-rule-visual-editor",
                                                           ":rule");
  EXPECT_NE(rule_editor, nullptr);
  auto rule_row =
    find_descendant_mode_containing_state(session.active_mode, "terrain-rule-row", ":rule");
  EXPECT_NE(rule_row, nullptr);

  auto center_cell = find_descendant_mode_containing_state(session.active_mode,
                                                           "terrain-match-tile-cell",
                                                           ":center? true");
  ASSERT_NE(center_cell, nullptr);
  EXPECT_NE(center_cell->state->to_string().find(":grass-fill"), std::string::npos);

  auto north_cell = find_descendant_mode_containing_state(session.active_mode,
                                                          "terrain-match-tile-cell",
                                                          ":direction :n ");
  ASSERT_NE(north_cell, nullptr);
  auto click_north_cell = [&](int button)
  {
    input().mouse_down({north_cell->bounds.x + north_cell->bounds.w / 2,
                        north_cell->bounds.y + north_cell->bounds.h / 2},
                       button);
    update_cycle();
    input().mouse_up({north_cell->bounds.x + north_cell->bounds.w / 2,
                      north_cell->bounds.y + north_cell->bounds.h / 2},
                     button);
    update_cycle();
    update_cycle();
  };
  click_north_cell(SDL_BUTTON_RIGHT);
  EXPECT_NE(session.active_mode->state->to_string().find(":n :same"), std::string::npos);
  EXPECT_NE(session.active_mode->state->to_string().find(
              ":selected-terrain-rule-match-direction :n"),
            std::string::npos);

  click_north_cell(SDL_BUTTON_RIGHT);
  EXPECT_NE(session.active_mode->state->to_string().find(":n :not-same"), std::string::npos);

  click_north_cell(SDL_BUTTON_RIGHT);
  EXPECT_NE(session.active_mode->state->to_string().find(":n :other-terrain"),
            std::string::npos);

  click_north_cell(SDL_BUTTON_LEFT);
  EXPECT_NE(session.active_mode->state->to_string().find(":n :not-same"), std::string::npos);

  click_north_cell(SDL_BUTTON_RIGHT);
  click_north_cell(SDL_BUTTON_RIGHT);
  EXPECT_NE(session.active_mode->state->to_string().find(":n {:terrain :water}"),
            std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> stamp_cells;
  find_descendant_modes(session.active_mode, "terrain-stamp-grid-cell", stamp_cells);
  ASSERT_EQ(stamp_cells.size(), 9u);
  std::shared_ptr<Pixils::Runtime::View> stamp_cell;
  for (const auto& cell : stamp_cells)
  {
    const auto state_text = cell->state ? cell->state->to_string() : "";
    if (state_text.find(":stamp-x 2") != std::string::npos &&
        state_text.find(":stamp-y 1") != std::string::npos)
    {
      stamp_cell = cell;
      break;
    }
  }
  ASSERT_NE(stamp_cell, nullptr);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_pickers;
  find_descendant_modes(session.active_mode, "terrain-stamp-tile-picker", tile_pickers);
  ASSERT_EQ(tile_pickers.size(), 1u);
  auto tile_picker = tile_pickers[0];
  const InputSimulator::Coord water_tile_position{
    static_cast<int>(tile_picker->bounds.x + 84),
    static_cast<int>(tile_picker->bounds.y + 16)};
  const InputSimulator::Coord no_tile_position{static_cast<int>(tile_picker->bounds.x + 16),
                                               static_cast<int>(tile_picker->bounds.y + 16)};
  const InputSimulator::Coord stamp_cell_position{
    static_cast<int>(stamp_cell->bounds.x + stamp_cell->bounds.w / 2),
    static_cast<int>(stamp_cell->bounds.y + stamp_cell->bounds.h / 2)};
  input().mouse_down(water_tile_position);
  update_cycle();
  EXPECT_NE(
    session.active_mode->state->to_string().find(":terrain-rule-selected-tile :water-fill"),
    std::string::npos);
  input().mouse_move(stamp_cell_position);
  update_cycle();
  input().mouse_up(stamp_cell_position);
  update_cycle();
  EXPECT_NE(session.active_mode->state->to_string().find(
              ":tiles [[nil nil nil] [nil :grass-fill :water-fill] [nil nil nil]]"),
            std::string::npos);

  input().mouse_down(no_tile_position);
  update_cycle();
  input().mouse_move(stamp_cell_position);
  update_cycle();
  input().mouse_up(stamp_cell_position);
  update_cycle();
  EXPECT_NE(session.active_mode->state->to_string().find(
              ":tiles [[nil nil nil] [nil :grass-fill nil] [nil nil nil]]"),
            std::string::npos);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}

TEST_F(TilemapEditorStartupTest, loaded_terrain_rule_normalizes_legacy_sprite_sources)
{
  const auto history_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-terrain-rule-legacy-source-history.edn";
  const auto project_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-terrain-rule-legacy-source-project.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);

  {
    std::ofstream out(project_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {:project-assets
                       {:images {:spritesheet
                                 {:file-name "examples/tilemap-editor/assets/simples_pimples.png"
                                  :name "Spritesheet"}}}}}
 :tilesets [{:id :terrain
             :label "Terrain"
             :tile-size 16
             :tiles [{:id :grass-fill
                      :name "Grass Fill"
                      :char "g"
                      :type :sprite
                      :image :project-assets/spritesheet
                      :source {:x 0 :y 0}}]}]
 :terrain-sets [{:id :overworld
                 :label "Overworld"
                 :tileset :terrain
                 :terrains [{:id :grass
                             :label "Grass"
                             :tile :grass-fill}]}]
 :rulesets [{:id :overworld-rules
             :label "Overworld Rules"
             :kind :terrain-stamp
             :terrain-set :overworld
             :source-layer :scene/terrain
             :rules [{:id :rule
                      :label "Rule"
                      :terrain :grass
                      :match {:nw :ignore
                              :n :same
                              :ne :ignore
                              :w :ignore
                              :e :ignore
                              :sw :ignore
                              :s :ignore
                              :se :ignore}
                      :output {:anchor {:x 1 :y 1}
                               :layers [{:id :layer
                                         :data-kind :tile-ref
                                         :target-layer :scene/terrain-visuals
                                         :tileset :terrain
                                         :tiles [[nil nil nil]
                                                 [nil :grass-fill nil]
                                                 [nil nil nil]]}]}}]}]
 :layer-profiles [{:id :default
                   :label "Default"
                   :layers [{:id :scene/terrain
                             :label "Terrain Source"
                             :kind :terrain
                             :data-kind :terrain
                             :terrain-set :overworld}
                            {:id :scene/terrain-visuals
                             :label "Terrain Visuals"
                             :kind :tile
                             :data-kind :tile-ref
                             :tileset :terrain}]}]
 :tilemap {:width 2
           :height 2
           :tile-size 16
           :layer-profile :default
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
                                  :filename "terrain-rule-legacy-source-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  EXPECT_NE(session.active_mode->state->to_string().find(":source {:x 0 :y 0 :w 16 :h 16}"),
            std::string::npos);

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  auto tab_strip = tab_panel->children[0];
  ASSERT_GE(tab_strip->children.size(), 5u);

  auto terrains_tab = tab_strip->children[4];
  ASSERT_NE(terrains_tab, nullptr);
  input().mouse_down({terrains_tab->bounds.x + terrains_tab->bounds.w / 2,
                      terrains_tab->bounds.y + terrains_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({terrains_tab->bounds.x + terrains_tab->bounds.w / 2,
                    terrains_tab->bounds.y + terrains_tab->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  auto terrain_detail_tab = find_descendant_mode_containing_state(session.active_mode,
                                                                  "ui/tab",
                                                                  ":label \"Terrain\"");
  ASSERT_NE(terrain_detail_tab, nullptr);
  input().mouse_down({terrain_detail_tab->bounds.x + terrain_detail_tab->bounds.w / 2,
                      terrain_detail_tab->bounds.y + terrain_detail_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({terrain_detail_tab->bounds.x + terrain_detail_tab->bounds.w / 2,
                    terrain_detail_tab->bounds.y + terrain_detail_tab->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  auto rule_editor = find_descendant_mode_containing_state(session.active_mode,
                                                           "terrain-rule-visual-editor",
                                                           ":rule");
  EXPECT_NE(rule_editor, nullptr);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}

TEST_F(TilemapEditorStartupTest, painting_terrain_on_canvas_applies_terrain_stamp_rules)
{
  const auto history_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-terrain-rule-paint-history.edn";
  const auto project_path = std::filesystem::temp_directory_path() /
                            "pixils-tilemap-editor-terrain-rule-paint-project.edn";
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
             :tiles [{:id :grass-fill
                      :name "Grass Fill"
                      :type :color
                      :color {:r 0 :g 255 :b 0}}
                     {:id :water-fill
                      :name "Water Fill"
                      :type :color
                      :color {:r 0 :g 0 :b 255}}
                     {:id :shore-n
                      :name "Shore North"
                      :type :color
                      :color {:r 255 :g 255 :b 0}}]}]
 :terrain-sets [{:id :overworld
                 :label "Overworld"
                 :tileset :terrain
                 :terrains [{:id :grass
                             :label "Grass"
                             :tile :grass-fill}
                            {:id :water
                             :label "Water"
                             :tile :water-fill}]}]
 :rulesets [{:id :shore-rules
             :kind :terrain-stamp
             :terrain-set :overworld
             :source-layer nil
             :rules [{:id :grass-north-water
                      :terrain :grass
                      :match {:nw :ignore
                              :n {:terrain :water}
                              :ne :ignore
                              :w :ignore
                              :e :ignore
                              :sw :ignore
                              :s :ignore
                              :se :ignore}
                      :output {:anchor {:x 1 :y 1}
                               :layers [{:id :layer
                                         :target-layer nil
                                         :tileset :terrain
                                         :tiles [[nil :shore-n nil]
                                                 [nil :grass-fill nil]
                                                 [nil nil nil]]}]}}]}]
 :layer-profiles [{:id :default
                   :label "Default"
                   :layers [{:id :scene/terrain
                             :label "Terrain Source"
                             :kind :tile
                             :role :source
                             :data-kind :terrain
                             :terrain-set :overworld}]}]
 :tilemap {:width 32
           :height 32
           :tile-size 16
           :layer-profile :default
           :layers [{:id :scene/terrain
                     :label "Terrain Source"
                     :kind :tile
                     :role :source
                     :data-kind :terrain
                     :terrain-set :overworld
                     :tiles [[nil :water nil]]}]}})";
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
                                  :filename "terrain-rule-paint-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  std::vector<std::shared_ptr<Pixils::Runtime::View>> canvases;
  find_descendant_modes(session.active_mode, "map-canvas", canvases);
  ASSERT_EQ(canvases.size(), 1u);
  auto canvas = canvases[0];

  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("terrain-rule-application"),
                             Lisple::keyword("paint-baked"));
  update_cycle();
  update_cycle();

  input().mouse_down({canvas->bounds.x + 16 + 2, canvas->bounds.y + 16 + 2});
  update_cycle();
  input().mouse_up({canvas->bounds.x + 16 + 2, canvas->bounds.y + 16 + 2});
  update_cycle();
  update_cycle();

  const auto baked_state_text = session.active_mode->state->to_string();
  EXPECT_NE(baked_state_text.find(":terrain-stamp-replacements"), std::string::npos);
  EXPECT_NE(baked_state_text.find(":id :layer :label \":shore-rules\""),
            std::string::npos);
  EXPECT_NE(baked_state_text.find(":shore-n"), std::string::npos);
  EXPECT_NE(baked_state_text.find(":grass-fill"), std::string::npos);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}
