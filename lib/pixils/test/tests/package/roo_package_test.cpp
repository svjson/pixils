#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/script.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <roo-package/manifest.h>
#include <roo-package/native_loader.h>
#include <roo/io/dir_root_file_system.h>
#include <roo/runtime.h>

namespace
{
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

TEST(PixilsRooPackageTest, owned_native_namespaces_retain_the_package_render_context)
{
  auto namespaces =
    Pixils::make_roo_native_namespaces(std::make_unique<Pixils::RenderContext>());
  auto& render_context = namespaces.front()
                           ->lookup("render-context")
                           ->adapter<Pixils::Script::RenderContextAdapter>();

  std::unique_ptr<Pixils::RenderContext> owned_context;
  EXPECT_NO_THROW(owned_context = render_context.release_pointer());
  EXPECT_NE(owned_context, nullptr);
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
  Roo::Package::configure_runtime_namespace_roots(runtime, plan);
  native_packages = Roo::Package::load_native_libraries(runtime, plan);
  Roo::Package::load_autoloads(runtime, plan);

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
  Roo::Package::configure_runtime_namespace_roots(runtime, plan);
  native_packages = Roo::Package::load_native_libraries(runtime, plan);
  Roo::Package::load_autoloads(runtime, plan);

  runtime.eval("(ns pixils.test-package-test (:require pixils.test))");

  EXPECT_EQ(runtime.eval("(nil? (resolve 'pixils.test/make-app))")->to_string(), "false");
}
