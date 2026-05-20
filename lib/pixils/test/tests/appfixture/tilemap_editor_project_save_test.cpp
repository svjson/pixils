#include "../fixture.h"
#include "../render_fixture.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace
{
  std::string lisp_string(const std::string& value)
  {
    std::string out = "\"";
    for (char c : value)
    {
      if (c == '\\' || c == '"')
      {
        out.push_back('\\');
      }
      out.push_back(c);
    }
    out.push_back('"');
    return out;
  }

  std::string read_text_file(const std::filesystem::path& path)
  {
    std::ifstream in(path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
  }

  void find_descendant_modes(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name,
    std::vector<std::shared_ptr<Pixils::Runtime::View>>& out)
  {
    if (!view) return;
    if (view->mode && view->mode->name == mode_name) out.push_back(view);

    for (const auto& child : view->children)
    {
      find_descendant_modes(child, mode_name, out);
    }
  }

  void read_tilemap_editor_sources(Lisple::Runtime& runtime)
  {
    runtime.read_file("examples/tilemap-editor/src/assets.lisple");
    runtime.read_file("examples/tilemap-editor/src/model/data.lisple");
    runtime.read_file("examples/tilemap-editor/src/model/tilemap.lisple");
    runtime.read_file("examples/tilemap-editor/src/io/project.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/renderer.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/canvas.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/inspector.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/palette.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/controls.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/layout.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/tileset/layout.lisple");
    runtime.read_file("examples/tilemap-editor/src/view/theme.lisple");
    runtime.read_file("examples/tilemap-editor/src/root.lisple");
  }
} // namespace

class TilemapEditorProjectIoTest : public BaseFixture
{
};

class TilemapEditorStartupTest : public RenderFixture
{
 protected:
  TilemapEditorStartupTest() { render_ctx.buffer_dim = {800, 600}; }
};

TEST_F(TilemapEditorStartupTest, current_example_main_mode_updates_and_renders)
{
  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "main-mode");

  ASSERT_GE(session.active_mode->children.size(), 2u);
  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  ASSERT_EQ(tab_panel->children.size(), 2u);
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

  auto body = tab_panel->children[1];
  ASSERT_NE(body, nullptr);
  ASSERT_EQ(body->children.size(), 1u);
  EXPECT_EQ(body->children[0]->mode->name, "tileset-definition-workspace");
}

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

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_swatches;
  find_descendant_modes(session.active_mode, "tile-swatch", tile_swatches);
  EXPECT_GT(tile_swatches.size(), 1u);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> combo_boxes;
  find_descendant_modes(session.active_mode, "ui/combo-box", combo_boxes);
  ASSERT_GE(combo_boxes.size(), 1u);
  EXPECT_NE(combo_boxes[0]->state->to_string().find("Background Colors"),
            std::string::npos);

  session.render_mode();
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
  EXPECT_NE(tileset_tab_combo_boxes[0]->state->to_string().find("Color"),
            std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_definition_panels;
  find_descendant_modes(session.active_mode, "tile-definition-panel", tile_definition_panels);
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

  input().mouse_down({map_tab->bounds.x + map_tab->bounds.w / 2,
                      map_tab->bounds.y + map_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({map_tab->bounds.x + map_tab->bounds.w / 2,
                    map_tab->bounds.y + map_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  layer_rows.clear();
  find_descendant_modes(session.active_mode, "layer-row", layer_rows);
  EXPECT_EQ(layer_rows.size(), 4u);

  tile_swatches.clear();
  find_descendant_modes(session.active_mode, "tile-swatch", tile_swatches);
  EXPECT_GT(tile_swatches.size(), 1u);

  std::filesystem::remove(history_path, ec);
}

TEST_F(TilemapEditorStartupTest, loads_sprite_project_after_empty_tileset_tab_with_missing_image)
{
  const auto history_path =
    std::filesystem::temp_directory_path() /
    "pixils-tilemap-editor-missing-image-history.edn";
  const auto project_path =
    std::filesystem::temp_directory_path() /
    "pixils-tilemap-editor-missing-image-project.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);

  {
    std::ofstream out(project_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {:project-assets
                       {:images {:missing
                                 {:file-name "does-not-exist.png"
                                  :name "Missing Spritesheet"}}}}}
 :tilesets [{:id :sprites
             :label "Sprites"
             :tile-size 16
             :tiles [{:id :sprite
                      :name "Sprite"
                      :char "s"
                      :type :sprite
                      :image :project-assets/missing
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
  session.render_mode();

  auto tab_panel = session.active_mode->children[1];
  auto tab_strip = tab_panel->children[0];
  auto tilesets_tab = tab_strip->children[1];
  input().mouse_down({tilesets_tab->bounds.x + tilesets_tab->bounds.w / 2,
                      tilesets_tab->bounds.y + tilesets_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({tilesets_tab->bounds.x + tilesets_tab->bounds.w / 2,
                    tilesets_tab->bounds.y + tilesets_tab->bounds.h / 2});
  update_cycle();
  session.render_mode();

  auto origin = Lisple::map({Lisple::keyword("view"),
                             Pixils::Script::ViewAdapter::make_ref(*session.active_mode),
                             Lisple::keyword("event"),
                             Lisple::keyword("project/file-dialog-result")});
  auto overrides = Lisple::map({Lisple::keyword("origin"), origin});
  session.push_mode("ui/tab-panel-empty", Lisple::Constant::NIL, overrides);
  session.pop_mode(runtime.eval(R"({:type :confirm
                                  :mode :file-dialog/open
                                  :path )" + lisp_string(project_path.string()) + R"(
                                  :directory )" +
                                 lisp_string(project_path.parent_path().string()) + R"(
                                  :filename "sprite-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  EXPECT_NE(session.active_mode->state->to_string().find(":workspace-tab :tilesets"),
            std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> sprite_previews;
  find_descendant_modes(session.active_mode, "sprite-map-preview", sprite_previews);
  ASSERT_EQ(sprite_previews.size(), 1u);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}

TEST_F(TilemapEditorStartupTest, loaded_project_with_resources_opens_resource_and_tileset_tabs)
{
  const auto history_path =
    std::filesystem::temp_directory_path() /
    "pixils-tilemap-editor-resource-history.edn";
  const auto project_path =
    std::filesystem::temp_directory_path() /
    "pixils-tilemap-editor-resource-project.edn";
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
                                  :path )" + lisp_string(project_path.string()) + R"(
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
  ASSERT_GE(tab_strip->children.size(), 3u);

  auto resources_tab = tab_strip->children[2];
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
  EXPECT_NE(bundle_rows[0]->state->to_string().find("project-assets"),
            std::string::npos);
  EXPECT_EQ(bundle_rows[0]->state->to_string().find("editor-assets"),
            std::string::npos);

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

TEST_F(TilemapEditorProjectIoTest, save_dialog_result_writes_project_edn)
{
  const auto save_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-save-test.edn";
  std::error_code ec;
  std::filesystem::remove(save_path, ec);

  runtime.read_file("examples/tilemap-editor/src/model/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/io/project.lisple");

  runtime.eval(R"(
    (tilemap-editor.io.project/apply-project-file-dialog-result
     {:layers (tilemap-editor.model.data/make-layered-map)}
     {:type :confirm
      :mode :file-dialog/save
      :path )" + lisp_string(save_path.string()) + R"(
      :directory )" + lisp_string(save_path.parent_path().string()) + R"(
      :filename "saved-project.edn"})
  )");

  ASSERT_TRUE(std::filesystem::exists(save_path));
  const std::string contents = read_text_file(save_path);
  EXPECT_NE(contents.find(":format :pixils.tilemap-editor/project"),
            std::string::npos);
  EXPECT_NE(contents.find(":version 1"), std::string::npos);
  EXPECT_NE(contents.find(":resources"), std::string::npos);
  EXPECT_EQ(contents.find("editor-assets"), std::string::npos);
  EXPECT_EQ(contents.find(":dynamic?"), std::string::npos);
  EXPECT_NE(contents.find(":tilesets"), std::string::npos);
  EXPECT_NE(contents.find(":tilemap"), std::string::npos);
  EXPECT_NE(contents.find(":layers"), std::string::npos);

  std::filesystem::remove(save_path, ec);
}

TEST_F(TilemapEditorProjectIoTest, save_dialog_result_updates_recent_projects)
{
  const auto save_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-save-history-test.edn";
  const auto history_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-history-test.edn";
  std::error_code ec;
  std::filesystem::remove(save_path, ec);
  std::filesystem::remove(history_path, ec);

  runtime.read_file("examples/tilemap-editor/src/model/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/io/project.lisple");

  auto result = runtime.eval(R"(
    (tilemap-editor.io.project/apply-project-file-dialog-result
     {:layers []
      :recent-projects []
      :project-history-path )" + lisp_string(history_path.string()) + R"(}
     {:type :confirm
      :mode :file-dialog/save
      :path )" + lisp_string(save_path.string()) + R"(
      :directory )" + lisp_string(save_path.parent_path().string()) + R"(
      :filename "saved-history-project.edn"})
  )");

  ASSERT_TRUE(std::filesystem::exists(history_path));
  const std::string state = result->to_string();
  EXPECT_NE(state.find(":recent-projects [{"), std::string::npos);
  EXPECT_NE(state.find(R"(:filename "saved-history-project.edn")"),
            std::string::npos);

  const std::string contents = read_text_file(history_path);
  EXPECT_NE(contents.find(":projects"), std::string::npos);
  EXPECT_NE(contents.find(save_path.string()), std::string::npos);
  EXPECT_NE(contents.find(R"(:filename "saved-history-project.edn")"),
            std::string::npos);

  std::filesystem::remove(save_path, ec);
  std::filesystem::remove(history_path, ec);
}

TEST_F(TilemapEditorProjectIoTest, default_project_starts_with_empty_layers)
{
  runtime.read_file("examples/tilemap-editor/src/model/data.lisple");

  auto result = runtime.eval(R"(
    (let [layers (tilemap-editor.model.data/make-layered-map)]
      {:layer-count (count layers)})
  )");

  const std::string state = result->to_string();
  EXPECT_NE(state.find(":layer-count 0"), std::string::npos);
}

TEST_F(TilemapEditorProjectIoTest, open_dialog_result_loads_project_edn_layers)
{
  const auto open_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-open-test.edn";
  std::error_code ec;
  std::filesystem::remove(open_path, ec);

  {
    std::ofstream out(open_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:images {:editor-assets/spritesheet {:file-name "assets/simples_pimples.png"}}}
 :tilesets []
 :tilemap {:width 2
           :height 2
           :tile-size 16
           :layers [{:id :scene/background
                     :label "Loaded Background"
                     :kind :tile
                     :tileset :colors
                     :order 0
                     :tiles [[:night :void] [:void :night]]}]}})";
  }

  runtime.read_file("examples/tilemap-editor/src/model/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/io/project.lisple");

  auto result = runtime.eval(R"(
    (tilemap-editor.io.project/apply-project-file-dialog-result
     {:layers (tilemap-editor.model.data/make-layered-map)
      :selected-layer-index 2
      :hidden-layer-indices [1 2]}
     {:type :confirm
      :mode :file-dialog/open
      :path )" + lisp_string(open_path.string()) + R"(
      :directory )" + lisp_string(open_path.parent_path().string()) + R"(
      :filename "loaded-project.edn"})
  )");

  const std::string state = result->to_string();
  EXPECT_NE(state.find(R"(:project-path ")" + open_path.string() + R"(")"),
            std::string::npos);
  EXPECT_NE(state.find(R"(:label "Loaded Background")"), std::string::npos);
  EXPECT_NE(state.find(":tilesets []"), std::string::npos);
  EXPECT_NE(state.find(":selected-layer-index 0"), std::string::npos);
  EXPECT_NE(state.find(":hidden-layer-indices []"), std::string::npos);
  EXPECT_NE(state.find(":last-project-open-result"), std::string::npos);

  std::filesystem::remove(open_path, ec);
}
