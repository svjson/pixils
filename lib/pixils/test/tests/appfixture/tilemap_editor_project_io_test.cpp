#include "tilemap_editor_test_support.h"

#include <gtest/gtest.h>

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
      :path )" +
               lisp_string(save_path.string()) + R"(
      :directory )" +
               lisp_string(save_path.parent_path().string()) + R"(
      :filename "saved-project.edn"})
  )");

  ASSERT_TRUE(std::filesystem::exists(save_path));
  const std::string contents = read_text_file(save_path);
  EXPECT_NE(contents.find(":format :pixils.tilemap-editor/project"), std::string::npos);
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
      :project-history-path )" +
                             lisp_string(history_path.string()) + R"(}
     {:type :confirm
      :mode :file-dialog/save
      :path )" + lisp_string(save_path.string()) +
                             R"(
      :directory )" + lisp_string(save_path.parent_path().string()) +
                             R"(
      :filename "saved-history-project.edn"})
  )");

  ASSERT_TRUE(std::filesystem::exists(history_path));
  const std::string state = result->to_string();
  EXPECT_NE(state.find(":recent-projects [{"), std::string::npos);
  EXPECT_NE(state.find(R"(:filename "saved-history-project.edn")"), std::string::npos);

  const std::string contents = read_text_file(history_path);
  EXPECT_NE(contents.find(":projects"), std::string::npos);
  EXPECT_NE(contents.find(save_path.string()), std::string::npos);
  EXPECT_NE(contents.find(R"(:filename "saved-history-project.edn")"), std::string::npos);

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
      :path )" + lisp_string(open_path.string()) +
                             R"(
      :directory )" + lisp_string(open_path.parent_path().string()) +
                             R"(
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
