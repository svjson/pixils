#include "../fixture.h"

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

class TilemapEditorProjectSaveTest : public BaseFixture
{
};

TEST_F(TilemapEditorProjectSaveTest, save_dialog_result_writes_project_edn)
{
  const auto save_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-save-test.edn";
  std::error_code ec;
  std::filesystem::remove(save_path, ec);

  runtime.read_file("examples/tilemap-editor/data.lisple");
  runtime.read_file("examples/tilemap-editor/controls.lisple");

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
