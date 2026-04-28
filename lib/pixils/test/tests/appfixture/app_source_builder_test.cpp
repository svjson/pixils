#include "app_source_builder.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace AppFixture = Pixils::Test::AppFixture;

namespace
{
  std::filesystem::path fixture_dir()
  {
    return std::filesystem::path(__FILE__).parent_path();
  }
} // namespace

TEST(AppSourceBuilderTest, parses_file_backed_unit_into_namespace_requires_and_body)
{
  auto unit = AppFixture::SourceUnit::from_file(
    "board-view",
    fixture_dir() / "assets" / "sample_unit.lisple");

  ASSERT_EQ(unit.require_entries.size(), 3u);
  EXPECT_EQ(unit.require_entries[0], "pixils");
  EXPECT_EQ(unit.require_entries[1], "[pixils.render :as r]");
  EXPECT_EQ(unit.require_entries[2], "[pixils.ui :as ui]");
  ASSERT_EQ(unit.body_forms.size(), 2u);
  EXPECT_EQ(unit.body_forms[0], "(def board-width 9)");
  EXPECT_EQ(unit.body_forms[1],
            "(defun board-click! [state ctx] (ui/emit! (:view ctx) :board/click {:x 1 :y 2}))");
}

TEST(AppSourceBuilderTest, composes_file_content_from_ordered_units_with_deduped_requires)
{
  AppFixture::ComposedFile file{
    .disk_path = "pixils/test/app/main.lisple",
    .namespace_name = "pixils.test.app.main",
    .units = {
      AppFixture::SourceUnit::inline_unit(
        "constants",
        {"pixils", "[pixils.ui :as ui]"},
        {"(def board-size 9)"}),
      AppFixture::SourceUnit::inline_unit(
        "handlers",
        {"pixils", "[pixils.render :as r]", "[pixils.ui :as ui]"},
        {"(defun render! [state ctx] (r/rect! {:x 0 :y 0 :w 1 :h 1} {:fill true}))",
         "(defn fire! [ctx] (ui/emit! (:view ctx) :board/click nil))"})}};

  std::string expected =
    "(ns pixils.test.app.main\n"
    "  (:require pixils [pixils.ui :as ui] [pixils.render :as r]))\n"
    "\n"
    "(def board-size 9)\n"
    "\n"
    "(defun render! [state ctx] (r/rect! {:x 0 :y 0 :w 1 :h 1} {:fill true}))\n"
    "\n"
    "(defn fire! [ctx] (ui/emit! (:view ctx) :board/click nil))\n";

  EXPECT_EQ(AppFixture::compose_file_content(file), expected);
}

TEST(AppSourceBuilderTest, ignores_unit_namespace_when_composing_file)
{
  auto unit = AppFixture::SourceUnit::from_source(
    "board-view",
    "(ns pixils.test.app.board (:require pixils)) (def board-width 9)");

  AppFixture::ComposedFile file{
    .disk_path = "pixils/test/app/main.lisple",
    .namespace_name = "pixils.test.app.main",
    .units = {unit}};

  EXPECT_EQ(AppFixture::compose_file_content(file),
            "(ns pixils.test.app.main\n"
            "  (:require pixils))\n"
            "\n"
            "(def board-width 9)\n");
}

TEST(AppSourceBuilderTest, writes_composed_file_to_disk)
{
  auto temp_root =
    std::filesystem::temp_directory_path() / "pixils-appfixture-test-output";
  std::filesystem::remove_all(temp_root);

  AppFixture::ComposedFile file{
    .disk_path = "pixils/test/app/config.lisple",
    .namespace_name = "pixils.test.app.config",
    .units = {AppFixture::SourceUnit::inline_unit("config", {}, {"(def mine-count 10)"})}};

  AppFixture::write_composed_file(file, temp_root);

  std::ifstream stream(temp_root / "pixils/test/app/config.lisple");
  ASSERT_TRUE(stream.is_open());

  std::string content((std::istreambuf_iterator<char>(stream)),
                      std::istreambuf_iterator<char>());

  EXPECT_EQ(content,
            "(ns pixils.test.app.config)\n"
            "\n"
            "(def mine-count 10)\n");

  std::filesystem::remove_all(temp_root);
}
