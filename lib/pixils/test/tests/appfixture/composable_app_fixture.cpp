#include "composable_app_fixture.h"

#include "app_source_builder.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace
{
  long long unique_suffix()
  {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }

  bool env_flag_set(const char* name)
  {
    const char* value = std::getenv(name);
    if (!value) return false;

    const std::string_view flag(value);
    return !(flag.empty() || flag == "0" || flag == "false" || flag == "FALSE");
  }

  bool should_keep_composed_app()
  {
    return env_flag_set("PIXILS_KEEP_COMPOSED_APP");
  }

  bool should_log_composed_app_root()
  {
    return should_keep_composed_app() || env_flag_set("PIXILS_LOG_COMPOSED_APP_ROOT");
  }
} // namespace

void ComposableAppFixture::TearDown()
{
  lisple_runtime.reset();

  if (!app_root.empty() && !should_keep_composed_app())
    std::filesystem::remove_all(app_root);
}

void ComposableAppFixture::load_app(const Pixils::Test::AppFixture::AppManifest& manifest,
                                    const std::string& main_namespace,
                                    const std::vector<std::string>& entry_files)
{
  lisple_runtime.reset();

  if (!app_root.empty() && !should_keep_composed_app())
  {
    std::filesystem::remove_all(app_root);
  }
  app_root = make_temp_app_root();
  std::filesystem::create_directories(app_root);

  for (const auto& file : manifest.materialize_files())
  {
    Pixils::Test::AppFixture::write_composed_file(file, app_root);
  }

  if (should_log_composed_app_root())
  {
    std::cout << "[ComposableAppFixture] app root: " << app_root << '\n';
  }

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

Lisple::sptr_val ComposableAppFixture::eval(const std::string& source)
{
  return pixils().eval(source);
}

std::filesystem::path ComposableAppFixture::make_temp_app_root()
{
  return std::filesystem::temp_directory_path() /
         ("pixils-composable-app-" + std::to_string(unique_suffix()));
}
