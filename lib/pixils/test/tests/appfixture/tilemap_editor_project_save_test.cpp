#include "../fixture.h"
#include "../render_fixture.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

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
  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  update_cycle();

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
  EXPECT_NE(contents.find(R"(:file-name "assets/simples_pimples.png")"),
            std::string::npos);
  EXPECT_NE(contents.find(":tilesets"), std::string::npos);
  EXPECT_NE(contents.find(":tilemap"), std::string::npos);
  EXPECT_NE(contents.find(":layers"), std::string::npos);

  std::filesystem::remove(save_path, ec);
}

TEST_F(TilemapEditorProjectIoTest, default_project_starts_with_empty_layers)
{
  runtime.read_file("examples/tilemap-editor/src/model/data.lisple");

  auto result = runtime.eval(R"(
    (let [layers (tilemap-editor.model.data/make-layered-map)]
      {:layer-count (count layers)
       :first-tile (get-in layers [0 :tiles 0 0])
       :last-tile (get-in layers [3 :tiles 27 39])})
  )");

  const std::string state = result->to_string();
  EXPECT_NE(state.find(":layer-count 4"), std::string::npos);
  EXPECT_NE(state.find(":first-tile nil"), std::string::npos);
  EXPECT_NE(state.find(":last-tile nil"), std::string::npos);
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
  EXPECT_NE(state.find(":tilesets [{:id :colors"), std::string::npos);
  EXPECT_NE(state.find(":selected-layer-index 0"), std::string::npos);
  EXPECT_NE(state.find(":hidden-layer-indices []"), std::string::npos);
  EXPECT_NE(state.find(":last-project-open-result"), std::string::npos);

  std::filesystem::remove(open_path, ec);
}
