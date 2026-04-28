#include "app_manifest.h"
#include "app_source_builder.h"
#include "composable_app_fixture.h"
#include <pixils/runtime/mode.h>

#include <gtest/gtest.h>
#include <lisple/host.h>
#include <lisple/runtime/dict.h>

namespace AppFixture = Pixils::Test::AppFixture;

namespace
{
  AppFixture::SourceUnit inline_unit(const std::string& id,
                                     const std::vector<std::string>& require_entries,
                                     const std::vector<std::string>& body)
  {
    return AppFixture::SourceUnit::inline_unit(id, require_entries, body);
  }

  AppFixture::AppManifest file_defined_mode_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("test-mode-api",
                   {"pixils"},
                   {"(pixils/defmode test-mode {:init (fn [state ctx] {:ticks 2}) "
                    ":update (fn [state ctx] (assoc state :ticks (+ (:ticks state) 1))) "
                    ":children [{:mode 'child-mode}]})",
                    "(pixils/defmode child-mode {})"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.lisple",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"test-mode-api"}}});
  }
} // namespace

class ComposableAppFixtureTest : public ComposableAppFixture
{
};

TEST_F(ComposableAppFixtureTest, loads_composed_app_with_cross_file_require_resolution)
{
  AppFixture::AppManifest manifest(
    {inline_unit("helpers-api", {}, {"(defun helper-answer [] 42)"}),
     inline_unit("main-api",
                 {"[pixils.test.app.helpers :as helpers]"},
                 {"(defun answer [] (helpers/helper-answer))"})},
    {AppFixture::ManifestFile{.id = "helpers",
                              .disk_path = "pixils/test/app/helpers.lisple",
                              .namespace_name = "pixils.test.app.helpers",
                              .unit_ids = {"helpers-api"}},
     AppFixture::ManifestFile{.id = "main",
                              .disk_path = "pixils/test/app/main.lisple",
                              .namespace_name = "pixils.test.app.main",
                              .unit_ids = {"main-api"}}});

  load_app(manifest, "pixils.test.app.main", {"pixils/test/app/main.lisple"});

  EXPECT_EQ(eval("(pixils.test.app.main/answer)")->num().get_int(), 42);
}

TEST_F(ComposableAppFixtureTest,
       loads_app_after_unit_override_without_changing_manifest_file_shape)
{
  AppFixture::AppManifest manifest(
    {inline_unit("helpers-api", {}, {"(defun helper-answer [] 10)"}),
     inline_unit("main-api",
                 {"[pixils.test.app.helpers :as helpers]"},
                 {"(defun answer [] (helpers/helper-answer))"})},
    {AppFixture::ManifestFile{.id = "helpers",
                              .disk_path = "pixils/test/app/helpers.lisple",
                              .namespace_name = "pixils.test.app.helpers",
                              .unit_ids = {"helpers-api"}},
     AppFixture::ManifestFile{.id = "main",
                              .disk_path = "pixils/test/app/main.lisple",
                              .namespace_name = "pixils.test.app.main",
                              .unit_ids = {"main-api"}}});

  manifest.upsert_unit(inline_unit("helpers-api", {}, {"(defun helper-answer [] 99)"}));

  load_app(manifest, "pixils.test.app.main", {"pixils/test/app/main.lisple"});

  EXPECT_EQ(eval("(pixils.test.app.main/answer)")->num().get_int(), 99);
}

TEST_F(ComposableAppFixtureTest, materializes_expected_source_for_file_defined_mode_unit)
{
  auto files = file_defined_mode_manifest().materialize_files();

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(
    AppFixture::compose_file_content(files[0]),
    "(ns pixils.test.app.main\n"
    "  (:require pixils))\n"
    "\n"
    "(pixils/defmode test-mode {:init (fn [state ctx] {:ticks 2}) :update (fn [state ctx] "
    "(assoc state :ticks (+ (:ticks state) 1))) :children [{:mode 'child-mode}]})\n"
    "\n"
    "(pixils/defmode child-mode {})\n");
}

TEST_F(ComposableAppFixtureTest, loads_file_defined_mode_into_pixils_mode_registry)
{
  load_app(file_defined_mode_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.lisple"});

  auto modes = pixils().lookup_value("pixils/modes");
  ASSERT_NE(modes, nullptr);

  auto test_mode_val =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
  ASSERT_NE(test_mode_val, nullptr);

  const auto& test_mode = Lisple::obj<Pixils::Runtime::Mode>(*test_mode_val);
  EXPECT_EQ(test_mode.name, "test-mode");
  EXPECT_EQ(test_mode.init->type, Lisple::RTValue::Type::FUNCTION);
  EXPECT_EQ(test_mode.update->type, Lisple::RTValue::Type::FUNCTION);
  ASSERT_EQ(test_mode.children.size(), 1u);
  EXPECT_EQ(test_mode.children[0].mode_name, "child-mode");
  EXPECT_EQ(test_mode.children[0].id, "child-mode-0");

  auto child_mode_val =
    Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("child-mode"));
  ASSERT_NE(child_mode_val, nullptr);
  const auto& child_mode = Lisple::obj<Pixils::Runtime::Mode>(*child_mode_val);
  EXPECT_EQ(child_mode.name, "child-mode");
}
