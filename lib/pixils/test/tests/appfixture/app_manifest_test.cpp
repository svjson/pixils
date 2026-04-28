#include "app_manifest.h"

#include <gtest/gtest.h>

namespace AppFixture = Pixils::Test::AppFixture;

namespace
{
  AppFixture::SourceUnit inline_unit(const std::string& id,
                                     const std::vector<std::string>& require_entries,
                                     const std::vector<std::string>& body)
  {
    return AppFixture::SourceUnit::inline_unit(id, require_entries, body);
  }
} // namespace

TEST(AppManifestTest, materializes_named_files_from_catalog_and_ordered_unit_ids)
{
  AppFixture::AppManifest manifest(
    {inline_unit("constants", {"pixils"}, {"(def board-size 9)"}),
     inline_unit("board-view", {"[pixils.ui :as ui]"}, {"(defn click! [ctx] (ui/emit! (:view ctx) :clicked nil))"})},
    {AppFixture::ManifestFile{
      .id = "main",
      .disk_path = "pixils/test/app/main.lisple",
      .namespace_name = "pixils.test.app.main",
      .unit_ids = {"constants", "board-view"}}});

  auto files = manifest.materialize_files();

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].namespace_name, "pixils.test.app.main");
  ASSERT_EQ(files[0].units.size(), 2u);
  EXPECT_EQ(files[0].units[0].id, "constants");
  EXPECT_EQ(files[0].units[1].id, "board-view");
}

TEST(AppManifestTest, upsert_unit_replaces_existing_implementation_under_same_id)
{
  AppFixture::AppManifest manifest(
    {inline_unit("status-panel", {}, {"(def panel-version :old)"})},
    {AppFixture::ManifestFile{
      .id = "ui",
      .disk_path = "pixils/test/app/ui.lisple",
      .namespace_name = "pixils.test.app.ui",
      .unit_ids = {"status-panel"}}});

  manifest.upsert_unit(inline_unit("status-panel", {}, {"(def panel-version :new)"}));

  auto files = manifest.materialize_files();
  auto content = AppFixture::compose_file_content(files[0]);

  EXPECT_NE(content.find("(def panel-version :new)"), std::string::npos);
  EXPECT_EQ(content.find("(def panel-version :old)"), std::string::npos);
}

TEST(AppManifestTest, supports_append_insert_and_remove_operations_on_file_units)
{
  AppFixture::AppManifest manifest(
    {inline_unit("a", {}, {"(def a 1)"}),
     inline_unit("b", {}, {"(def b 2)"}),
     inline_unit("c", {}, {"(def c 3)"})},
    {AppFixture::ManifestFile{
      .id = "main",
      .disk_path = "pixils/test/app/main.lisple",
      .namespace_name = "pixils.test.app.main",
      .unit_ids = {"a"}}});

  manifest.append_unit_to_file("main", "c");
  manifest.insert_unit_after("main", "a", "b");
  manifest.remove_unit_from_file("main", "c");

  auto files = manifest.materialize_files();
  ASSERT_EQ(files[0].units.size(), 2u);
  EXPECT_EQ(files[0].units[0].id, "a");
  EXPECT_EQ(files[0].units[1].id, "b");
}

TEST(AppManifestTest, removing_unit_also_removes_it_from_all_file_memberships)
{
  AppFixture::AppManifest manifest(
    {inline_unit("shared", {}, {"(def shared true)"}),
     inline_unit("tail", {}, {"(def tail true)"})},
    {AppFixture::ManifestFile{
       .id = "one",
       .disk_path = "pixils/test/app/one.lisple",
       .namespace_name = "pixils.test.app.one",
       .unit_ids = {"shared", "tail"}},
     AppFixture::ManifestFile{
       .id = "two",
       .disk_path = "pixils/test/app/two.lisple",
       .namespace_name = "pixils.test.app.two",
       .unit_ids = {"shared"}}});

  manifest.remove_unit("shared");

  auto files = manifest.materialize_files();
  ASSERT_EQ(files.size(), 2u);
  EXPECT_EQ(files[0].units.size(), 1u);
  EXPECT_EQ(files[0].units[0].id, "tail");
  EXPECT_TRUE(files[1].units.empty());
}

TEST(AppManifestTest, materialize_throws_when_file_references_unknown_unit)
{
  AppFixture::AppManifest manifest(
    {},
    {AppFixture::ManifestFile{
      .id = "main",
      .disk_path = "pixils/test/app/main.lisple",
      .namespace_name = "pixils.test.app.main",
      .unit_ids = {"missing"}}});

  EXPECT_THROW(manifest.materialize_files(), std::runtime_error);
}
