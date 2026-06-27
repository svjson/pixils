#include "composable_app_session_fixture.h"

#include "app_source_builder.h"
#include <pixils/asset/registry.h>
#include <pixils/font_registry.h>
#include <pixils/program.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sdl2_mock/mock_resources.h>
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

  void copy_path_into_root(const std::filesystem::path& source_path,
                           const std::filesystem::path& destination_path)
  {
    if (std::filesystem::is_directory(source_path))
    {
      std::filesystem::create_directories(destination_path);
      std::filesystem::copy(source_path,
                            destination_path,
                            std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing);
      return;
    }

    std::filesystem::create_directories(destination_path.parent_path());
    std::filesystem::copy_file(source_path,
                               destination_path,
                               std::filesystem::copy_options::overwrite_existing);
  }
} // namespace

ComposableAppSessionFixture::ComposableAppSessionFixture()
  : input_simulator(events)
{
  render_ctx.renderer = SDL_CreateRenderer(nullptr, 0, 0);
  render_ctx.buffer_texture = render_ctx.renderer->default_render_target;
  render_ctx.buffer_dim = {320, 200};
  hook_ctx = std::make_unique<Pixils::HookContext>(
    Pixils::HookContext{&events, &render_ctx, nullptr});
}

void ComposableAppSessionFixture::TearDown()
{
  pixils_session.reset();
  hook_ctx.reset();
  roo_runtime.reset();
  render_ctx.asset_registry.reset();
  render_ctx.font_registry.reset();

  if (!app_root.empty() && !should_keep_composed_app())
  {
    std::filesystem::remove_all(app_root);
  }

  SDLMock::reset_mocks();
}

void ComposableAppSessionFixture::load_app(
  const Pixils::Test::AppFixture::AppManifest& manifest,
  const std::string& main_namespace,
  const std::vector<std::string>& entry_files,
  const std::vector<RuntimeAssetCopy>& runtime_assets)
{
  pixils_session.reset();
  roo_runtime.reset();
  render_ctx.asset_registry.reset();
  render_ctx.font_registry.reset();

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
    std::cout << "[ComposableAppSessionFixture] app root: " << app_root << '\n';
  }

  stage_runtime_assets(runtime_assets);

  roo_runtime = Pixils::make_roo_runtime(
    render_ctx,
    main_namespace,
    [this](Pixils::RuntimeConfiguration* cfg)
    {
      cfg->load_path = {app_root.string()};
      cfg->asset_base_path = app_root.string();
    },
    entry_files);

  hook_ctx = std::make_unique<Pixils::HookContext>(
    Pixils::HookContext{&events, &render_ctx, nullptr});
  Pixils::Runtime::HookArguments hook_args{
    Pixils::Script::HookContextAdapter::make_ref(*hook_ctx)};
  hook_args.events = &events;

  pixils_session = std::make_unique<Pixils::Runtime::Session>(*roo_runtime,
                                                              *render_ctx.asset_registry,
                                                              render_ctx,
                                                              hook_args);
}

void ComposableAppSessionFixture::set_frame_size(const Pixils::Dimension& dim)
{
  render_ctx.buffer_dim = dim;
}

const Pixils::Dimension& ComposableAppSessionFixture::frame_size() const
{
  return render_ctx.buffer_dim;
}

InputSimulator& ComposableAppSessionFixture::input()
{
  return input_simulator;
}

void ComposableAppSessionFixture::update_cycle()
{
  session().update_mode();
  if (session().process_messages())
  {
    session().update_mode();
  }
  input_simulator.clear_transients();
}

void ComposableAppSessionFixture::render_cycle()
{
  session().render_mode();
  input_simulator.clear_transients();
}

void ComposableAppSessionFixture::frame_cycle()
{
  session().update_mode();
  if (session().process_messages())
  {
    session().update_mode();
  }
  session().render_mode();
  input_simulator.clear_transients();
}

Pixils::Program& ComposableAppSessionFixture::load_program()
{
  return Pixils::load_program(pixils(), session());
}

Roo::Runtime& ComposableAppSessionFixture::pixils()
{
  if (!roo_runtime)
  {
    throw std::runtime_error("ComposableAppSessionFixture runtime not initialized");
  }
  return *roo_runtime;
}

Pixils::Runtime::Session& ComposableAppSessionFixture::session()
{
  if (!pixils_session)
  {
    throw std::runtime_error("ComposableAppSessionFixture session not initialized");
  }
  return *pixils_session;
}

Roo::sptr_val ComposableAppSessionFixture::eval(const std::string& source)
{
  return pixils().eval(source);
}

SDL_Texture* ComposableAppSessionFixture::render_target()
{
  return render_ctx.renderer->render_target;
}

const std::filesystem::path& ComposableAppSessionFixture::app_root_dir() const
{
  return app_root;
}

std::filesystem::path ComposableAppSessionFixture::make_temp_app_root()
{
  return std::filesystem::temp_directory_path() /
         ("pixils-composable-session-app-" + std::to_string(unique_suffix()));
}

void ComposableAppSessionFixture::stage_runtime_assets(
  const std::vector<RuntimeAssetCopy>& runtime_assets)
{
  for (const auto& asset_copy : runtime_assets)
  {
    copy_path_into_root(asset_copy.source_path, app_root / asset_copy.dest_relative_path);
  }
}
