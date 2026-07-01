#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <roo-package/manifest.h>
#include <roo-package/native_loader.h>
#include <roo/io/dir_root_file_system.h>
#include <roo/runtime.h>

namespace
{
  class ScopedCurrentPath
  {
   public:
    explicit ScopedCurrentPath(const std::filesystem::path& path)
      : original_path(std::filesystem::current_path())
    {
      std::filesystem::current_path(path);
    }

    ~ScopedCurrentPath() { std::filesystem::current_path(original_path); }

   private:
    std::filesystem::path original_path;
  };

  Roo::Package::ResolveOptions test_package_resolve_options()
  {
    return Roo::Package::ResolveOptions{
      .package_search_roots = {PIXILS_ROO_LOCAL_PACKAGE_ROOT,
                               PIXILS_ROO_INSTALLED_PACKAGE_ROOT},
    };
  }

  Roo::Package::LoadPlan resolve_test_package(const std::string& package_dir)
  {
    Roo::DirRootFileSystem manifest_fs("/");
    return Roo::Package::resolve_load_plan(manifest_fs,
                                           package_dir,
                                           test_package_resolve_options());
  }
} // namespace

TEST(PixilsRooPackageTest, loads_native_package_and_runs_roo_proof_tests)
{
  auto plan = resolve_test_package(PIXILS_TEST_ROO_PACKAGE_DIR);
  ScopedCurrentPath test_package_cwd(PIXILS_TEST_ROO_PACKAGE_DIR);

  auto fs = Roo::Package::make_load_path_file_system(plan);
  Roo::Package::LoadedNativePackages native_packages;
  Roo::Runtime runtime(fs.get());
  native_packages = Roo::Package::load_native_libraries(runtime, plan);

  runtime.eval("(ns pixils.package-test-runner "
               "(:require proof.core "
               "pixils.package-test "
               "pixils.ui.window-test "
               "pixils.ui.scroll-pane-test "
               "pixils.ui.group-box-test))");

  auto summary = runtime.eval("(result-summary (run))");

  EXPECT_EQ(summary->to_string(), "{:total 32 :passed 32 :failed 0}");
}

TEST(PixilsRooPackageTest, pixils_runner_package_loads)
{
  auto plan = resolve_test_package(PIXILS_RUNNER_PACKAGE_DIR);

  auto runner_package = std::find_if(plan.packages.begin(),
                                     plan.packages.end(),
                                     [](const Roo::Package::PackageInfo& package)
                                     { return package.name == "pixils-runner"; });
  ASSERT_NE(runner_package, plan.packages.end());
  ASSERT_TRUE(runner_package->tools.count("run"));
  EXPECT_EQ(runner_package->tools.at("run"), "pixils.runner/run");

  auto fs = Roo::Package::make_load_path_file_system(plan);
  Roo::Package::LoadedNativePackages native_packages;
  Roo::Runtime runtime(fs.get());
  native_packages = Roo::Package::load_native_libraries(runtime, plan);

  runtime.eval("(ns pixils.runner-package-test (:require pixils.runner))");

  EXPECT_EQ(runtime.eval("pixils.runner/runner-package-loaded?")->to_string(), "true");
  EXPECT_EQ(runtime.eval("(nil? (resolve 'pixils.runner/run))")->to_string(), "false");
  EXPECT_EQ(runtime.eval("(nil? (resolve 'pixils.runner.native/run!))")->to_string(),
            "false");
}

TEST(PixilsRooPackageTest, pixils_test_package_loads)
{
  auto plan = resolve_test_package(PIXILS_TEST_PACKAGE_DIR);

  auto fs = Roo::Package::make_load_path_file_system(plan);
  Roo::Package::LoadedNativePackages native_packages;
  Roo::Runtime runtime(fs.get());
  native_packages = Roo::Package::load_native_libraries(runtime, plan);

  runtime.eval("(ns pixils.test-package-test (:require pixils.test))");

  EXPECT_EQ(runtime.eval("pixils.test/test-package-loaded?")->to_string(), "true");
}

TEST(PixilsRooPackageTest, pixils_test_package_runs_proof_tests)
{
  auto plan = resolve_test_package(PIXILS_TEST_PACKAGE_DIR);

  auto fs = Roo::Package::make_load_path_file_system(plan);
  Roo::Package::LoadedNativePackages native_packages;
  Roo::Runtime runtime(fs.get());
  native_packages = Roo::Package::load_native_libraries(runtime, plan);

  runtime.eval("(ns pixils.test-package-proof-runner "
               "(:require proof.core pixils.test.package-test))");

  auto summary = runtime.eval("(result-summary (run))");

  EXPECT_EQ(summary->to_string(), "{:total 5 :passed 5 :failed 0}");
}
