#include <lisple-package/manifest.h>
#include <lisple-package/native_loader.h>

#include <gtest/gtest.h>

#include <lisple/io/dir_root_file_system.h>
#include <lisple/runtime.h>

namespace
{
  Lisple::Package::ResolveOptions test_package_resolve_options()
  {
    return Lisple::Package::ResolveOptions{
      .package_search_roots = {PIXILS_LISPLE_LOCAL_PACKAGE_ROOT,
                               PIXILS_LISPLE_INSTALLED_PACKAGE_ROOT},
    };
  }

  Lisple::Package::LoadPlan resolve_test_package(const std::string& package_dir)
  {
    Lisple::DirRootFileSystem manifest_fs("/");
    return Lisple::Package::resolve_load_plan(
      manifest_fs,
      package_dir,
      test_package_resolve_options());
  }
} // namespace

TEST(PixilsLisplePackageTest, loads_native_package_and_runs_lisple_proof_tests)
{
  auto plan = resolve_test_package(PIXILS_TEST_LISPLE_PACKAGE_DIR);

  auto fs = Lisple::Package::make_load_path_file_system(plan);
  Lisple::Package::LoadedNativePackages native_packages;
  Lisple::Runtime runtime(fs.get());
  native_packages = Lisple::Package::load_native_libraries(runtime, plan);

  runtime.eval(
    "(ns pixils.package-test-runner (:require proof.core pixils.package-test))");

  auto summary = runtime.eval("(result-summary (run))");

  EXPECT_EQ(summary->to_string(), "{:total 2 :passed 2 :failed 0}");
}

TEST(PixilsLisplePackageTest, pixils_runner_package_loads)
{
  auto plan = resolve_test_package(PIXILS_RUNNER_PACKAGE_DIR);

  auto fs = Lisple::Package::make_load_path_file_system(plan);
  Lisple::Package::LoadedNativePackages native_packages;
  Lisple::Runtime runtime(fs.get());
  native_packages = Lisple::Package::load_native_libraries(runtime, plan);

  runtime.eval("(ns pixils.runner-package-test (:require pixils.runner))");

  EXPECT_EQ(runtime.eval("pixils.runner/runner-package-loaded?")->to_string(), "true");
}

TEST(PixilsLisplePackageTest, pixils_test_package_loads)
{
  auto plan = resolve_test_package(PIXILS_TEST_PACKAGE_DIR);

  auto fs = Lisple::Package::make_load_path_file_system(plan);
  Lisple::Package::LoadedNativePackages native_packages;
  Lisple::Runtime runtime(fs.get());
  native_packages = Lisple::Package::load_native_libraries(runtime, plan);

  runtime.eval("(ns pixils.test-package-test (:require pixils.test))");

  EXPECT_EQ(runtime.eval("pixils.test/test-package-loaded?")->to_string(), "true");
}
