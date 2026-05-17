#include <lisple-package/manifest.h>
#include <lisple-package/native_loader.h>

#include <gtest/gtest.h>

#include <lisple/io/dir_root_file_system.h>
#include <lisple/runtime.h>

TEST(PixilsLisplePackageTest, loads_native_package_and_runs_lisple_proof_tests)
{
  Lisple::DirRootFileSystem manifest_fs("/");
  Lisple::Package::ResolveOptions options{
    .package_search_roots = {PIXILS_LISPLE_LOCAL_PACKAGE_ROOT,
                             PIXILS_LISPLE_INSTALLED_PACKAGE_ROOT},
  };
  auto plan = Lisple::Package::resolve_load_plan(
    manifest_fs,
    PIXILS_TEST_LISPLE_PACKAGE_DIR,
    options);

  auto fs = Lisple::Package::make_load_path_file_system(plan);
  Lisple::Package::LoadedNativePackages native_packages;
  Lisple::Runtime runtime(fs.get());
  native_packages = Lisple::Package::load_native_libraries(runtime, plan);

  runtime.eval(
    "(ns pixils.package-test-runner (:require proof.core pixils.package-test))");

  auto summary = runtime.eval("(result-summary (run))");

  EXPECT_EQ(summary->to_string(), "{:total 2 :passed 2 :failed 0}");
}
