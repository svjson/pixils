#include "composable_app_fixture.h"

#include "app_source_builder.h"

#include <chrono>
#include <stdexcept>

namespace
{
  long long unique_suffix()
  {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }
} // namespace

void ComposableAppFixture::TearDown()
{
  lisple_runtime.reset();

  if (!app_root.empty()) std::filesystem::remove_all(app_root);
}

void ComposableAppFixture::load_app(const Pixils::Test::AppFixture::AppManifest& manifest,
                                    const std::string& main_namespace,
                                    const std::vector<std::string>& entry_files)
{
  lisple_runtime.reset();

  if (!app_root.empty()) std::filesystem::remove_all(app_root);
  app_root = make_temp_app_root();
  std::filesystem::create_directories(app_root);

  for (const auto& file : manifest.materialize_files())
    Pixils::Test::AppFixture::write_composed_file(file, app_root);

  lisple_runtime = Pixils::make_lisple_runtime(
    render_ctx,
    main_namespace,
    [this](Pixils::RuntimeConfiguration* cfg)
    {
      cfg->load_path = {app_root.string()};
      cfg->asset_base_path = app_root.string();
    },
    entry_files);
}

Lisple::Runtime& ComposableAppFixture::pixils()
{
  if (!lisple_runtime)
    throw std::runtime_error("ComposableAppFixture runtime not initialized");
  return *lisple_runtime;
}

Lisple::sptr_rtval ComposableAppFixture::eval(const std::string& source)
{
  return pixils().eval(source);
}

std::filesystem::path ComposableAppFixture::make_temp_app_root()
{
  return std::filesystem::temp_directory_path() /
         ("pixils-composable-app-" + std::to_string(unique_suffix()));
}
