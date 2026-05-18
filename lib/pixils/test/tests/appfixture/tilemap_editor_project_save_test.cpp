#include "../fixture.h"
#include "../render_fixture.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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
} // namespace

class TilemapEditorProjectIoTest : public BaseFixture
{
};

class TilemapEditorStartupTest : public RenderFixture
{
};

TEST_F(TilemapEditorStartupTest, current_example_main_mode_updates_and_renders)
{
  runtime.read_file("examples/tilemap-editor/src/assets.lisple");
  runtime.read_file("examples/tilemap-editor/src/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/tilemap.lisple");
  runtime.read_file("examples/tilemap-editor/src/tile-renderer.lisple");
  runtime.read_file("examples/tilemap-editor/src/canvas.lisple");
  runtime.read_file("examples/tilemap-editor/src/inspector.lisple");
  runtime.read_file("examples/tilemap-editor/src/palette.lisple");
  runtime.read_file("examples/tilemap-editor/src/controls.lisple");
  runtime.read_file("examples/tilemap-editor/src/workspace.lisple");
  runtime.read_file("examples/tilemap-editor/src/theme.lisple");
  runtime.read_file("examples/tilemap-editor/src/root.lisple");

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

TEST_F(TilemapEditorProjectIoTest, save_dialog_result_writes_project_edn)
{
  const auto save_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-save-test.edn";
  std::error_code ec;
  std::filesystem::remove(save_path, ec);

  runtime.read_file("examples/tilemap-editor/src/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/controls.lisple");

  runtime.eval(R"(
    (tilemap-editor.controls/apply-project-file-dialog-result
     {:layers (tilemap-editor.data/make-layered-map)}
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
  EXPECT_NE(contents.find(":image :editor-assets/spritesheet"), std::string::npos);

  std::filesystem::remove(save_path, ec);
}

TEST_F(TilemapEditorProjectIoTest, default_project_starts_with_empty_layers)
{
  runtime.read_file("examples/tilemap-editor/src/data.lisple");

  auto result = runtime.eval(R"(
    (let [layers (tilemap-editor.data/make-layered-map)]
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

  runtime.read_file("examples/tilemap-editor/src/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/controls.lisple");

  auto result = runtime.eval(R"(
    (tilemap-editor.controls/apply-project-file-dialog-result
     {:layers (tilemap-editor.data/make-layered-map)
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
  EXPECT_NE(state.find(":selected-layer-index 0"), std::string::npos);
  EXPECT_NE(state.find(":hidden-layer-indices []"), std::string::npos);
  EXPECT_NE(state.find(":last-project-open-result"), std::string::npos);

  std::filesystem::remove(open_path, ec);
}
