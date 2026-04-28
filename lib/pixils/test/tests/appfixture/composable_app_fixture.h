#ifndef PIXILS__TEST__APPFIXTURE__COMPOSABLE_APP_FIXTURE_H
#define PIXILS__TEST__APPFIXTURE__COMPOSABLE_APP_FIXTURE_H

#include "app_manifest.h"
#include <pixils/context.h>
#include <pixils/script.h>

#include <filesystem>
#include <gtest/gtest.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>
#include <memory>
#include <string>
#include <vector>

class ComposableAppFixture : public ::testing::Test
{
 protected:
  Pixils::RenderContext render_ctx{};
  std::filesystem::path app_root;
  std::unique_ptr<Lisple::Runtime> lisple_runtime;

  void TearDown() override;

  void load_app(const Pixils::Test::AppFixture::AppManifest& manifest,
                const std::string& main_namespace,
                const std::vector<std::string>& entry_files);

  Lisple::Runtime& pixils();
  Lisple::sptr_rtval eval(const std::string& source);

 private:
  static std::filesystem::path make_temp_app_root();
};

#endif
