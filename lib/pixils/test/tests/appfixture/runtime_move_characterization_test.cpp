#include "app_manifest.h"
#include "app_source_builder.h"
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/script.h>

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <lisple/host.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>
#include <sdl2_mock/mock_resources.h>

namespace AppFixture = Pixils::Test::AppFixture;

namespace
{
  long long unique_suffix()
  {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }

  std::filesystem::path make_temp_app_root()
  {
    return std::filesystem::temp_directory_path() /
           ("pixils-runtime-move-test-" + std::to_string(unique_suffix()));
  }

  AppFixture::SourceUnit inline_unit(const std::string& id,
                                     const std::vector<std::string>& require_entries,
                                     const std::vector<std::string>& body)
  {
    return AppFixture::SourceUnit::inline_unit(id, require_entries, body);
  }

  AppFixture::AppManifest file_defined_function_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("test-fn-api", {}, {"(def test-fn (fn [state ctx] {:ticks 2}))"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.lisple",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"test-fn-api"}}});
  }

  AppFixture::AppManifest file_defined_mode_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("test-mode-api",
                   {"pixils"},
                   {"(pixils/defmode test-mode {:init (fn [state ctx] {:ticks 2})})"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.lisple",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"test-mode-api"}}});
  }

  void materialize_manifest(const AppFixture::AppManifest& manifest,
                            const std::filesystem::path& app_root)
  {
    std::filesystem::create_directories(app_root);

    for (const auto& file : manifest.materialize_files())
      AppFixture::write_composed_file(file, app_root);
  }

  Lisple::sptr_rtval invoke_test_fn(Lisple::Runtime& runtime)
  {
    Lisple::sptr_rtval_v args{Lisple::Constant::NIL, Lisple::Constant::NIL};
    return runtime.invoke("pixils.test.app.main/test-fn", args);
  }

  Lisple::sptr_rtval invoke_test_mode_init(Lisple::Runtime& runtime)
  {
    auto modes = runtime.lookup_value("pixils/modes");
    auto mode_val = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
    auto& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);

    Lisple::Context exec_ctx(runtime);
    Lisple::sptr_rtval_v args{Lisple::Constant::NIL, Lisple::Constant::NIL};
    return mode.init->exec().execute(exec_ctx, args);
  }

  Lisple::sptr_rtval invoke_test_mode_init_with_hook_context(
    Lisple::Runtime& runtime,
    Pixils::FrameEvents& events,
    Pixils::RenderContext& render_ctx)
  {
    auto modes = runtime.lookup_value("pixils/modes");
    auto mode_val = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
    auto& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);

    Pixils::HookContext hook_ctx{&events, &render_ctx, nullptr};
    Lisple::Context exec_ctx(runtime);
    Lisple::sptr_rtval_v args{Lisple::Constant::NIL,
                              Pixils::Script::HookContextAdapter::make_ref(hook_ctx)};
    return mode.init->exec().execute(exec_ctx, args);
  }
} // namespace

TEST(RuntimeMoveCharacterizationTest,
     directly_initialized_runtime_executes_file_defined_function)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_function_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  auto result = invoke_test_fn(runtime);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     moved_runtime_executes_preloaded_function_defined_in_file)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_function_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  auto moved_runtime = std::make_unique<Lisple::Runtime>(std::move(runtime));
  auto result = invoke_test_fn(*moved_runtime);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     moved_runtime_still_executes_function_defined_after_move)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_function_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  auto moved_runtime = std::make_unique<Lisple::Runtime>(std::move(runtime));
  moved_runtime->eval("(def moved-fn (fn [state ctx] {:ticks 3}))");

  Lisple::sptr_rtval_v args{Lisple::Constant::NIL, Lisple::Constant::NIL};
  auto result = moved_runtime->invoke("moved-fn", args);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 3);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     directly_initialized_runtime_executes_file_defined_mode_init_hook)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  auto result = invoke_test_mode_init(runtime);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest, moved_runtime_executes_file_defined_mode_init_hook)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  auto moved_runtime = std::make_unique<Lisple::Runtime>(std::move(runtime));
  auto result = invoke_test_mode_init(*moved_runtime);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     direct_file_defined_mode_init_invocation_executes_with_real_hook_context)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Pixils::FrameEvents events;
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  auto result = invoke_test_mode_init_with_hook_context(runtime, events, render_ctx);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     session_invoke_hook_directly_executes_file_defined_mode_init_hook)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Pixils::FrameEvents events;
  Pixils::HookContext hook_ctx{&events, &render_ctx, nullptr};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  Pixils::Runtime::HookArguments hook_args{
    Pixils::Script::HookContextAdapter::make_ref(hook_ctx)};
  hook_args.events = &events;

  Pixils::Runtime::Session session(runtime,
                                   *render_ctx.asset_registry,
                                   render_ctx,
                                   hook_args);

  auto modes = runtime.lookup_value("pixils/modes");
  auto mode_val = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
  auto& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);

  auto view = std::make_shared<Pixils::Runtime::View>();
  view->mode = &mode;
  view->state = Lisple::Constant::NIL;

  auto result =
    session.invoke_hook(view, mode.init, session.hook_args.init_args, Lisple::Constant::NIL);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     session_invoke_hook_executes_after_mode_stack_push_and_active_mode_setup)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  Pixils::FrameEvents events;
  Pixils::HookContext hook_ctx{&events, &render_ctx, nullptr};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  Pixils::Runtime::HookArguments hook_args{
    Pixils::Script::HookContextAdapter::make_ref(hook_ctx)};
  hook_args.events = &events;

  Pixils::Runtime::Session session(runtime,
                                   *render_ctx.asset_registry,
                                   render_ctx,
                                   hook_args);

  auto modes = runtime.lookup_value("pixils/modes");
  auto mode_val = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
  auto& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);

  session.mode_stack.push(mode_val, Lisple::Constant::NIL);
  session.active_mode = std::make_shared<Pixils::Runtime::View>();
  session.active_mode->mode = &mode;
  session.active_mode->state = Lisple::Constant::NIL;
  session.hook_args.update_state(Lisple::Constant::NIL);

  auto result = session.invoke_hook(session.active_mode,
                                    session.active_mode->mode->init,
                                    session.hook_args.init_args,
                                    session.active_mode->state);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     session_invoke_hook_with_fixture_like_render_context_executes)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  render_ctx.renderer = SDL_CreateRenderer(nullptr, 0, 0);
  render_ctx.buffer_texture = render_ctx.renderer->default_render_target;
  render_ctx.buffer_dim.w = 320;
  render_ctx.buffer_dim.h = 200;

  Pixils::FrameEvents events;
  Pixils::HookContext hook_ctx{&events, &render_ctx, nullptr};
  Lisple::Runtime runtime =
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"});

  Pixils::Runtime::HookArguments hook_args{
    Pixils::Script::HookContextAdapter::make_ref(hook_ctx)};
  hook_args.events = &events;

  Pixils::Runtime::Session session(runtime,
                                   *render_ctx.asset_registry,
                                   render_ctx,
                                   hook_args);

  auto modes = runtime.lookup_value("pixils/modes");
  auto mode_val = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
  auto& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);

  auto view = std::make_shared<Pixils::Runtime::View>();
  view->mode = &mode;
  view->state = Lisple::Constant::NIL;

  auto result =
    session.invoke_hook(view, mode.init, session.hook_args.init_args, Lisple::Constant::NIL);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}

TEST(RuntimeMoveCharacterizationTest,
     moved_runtime_session_invoke_hook_with_fixture_like_render_context_executes)
{
  auto app_root = make_temp_app_root();
  materialize_manifest(file_defined_mode_manifest(), app_root);

  Pixils::RenderContext render_ctx{};
  render_ctx.renderer = SDL_CreateRenderer(nullptr, 0, 0);
  render_ctx.buffer_texture = render_ctx.renderer->default_render_target;
  render_ctx.buffer_dim.w = 320;
  render_ctx.buffer_dim.h = 200;

  Pixils::FrameEvents events;
  Pixils::HookContext hook_ctx{&events, &render_ctx, nullptr};
  auto runtime = std::make_unique<Lisple::Runtime>(
    Pixils::init_lisple_runtime(render_ctx,
                                "pixils.test.app.main",
                                [&app_root](Pixils::RuntimeConfiguration* cfg)
                                {
                                  cfg->load_path = {app_root.string()};
                                  cfg->asset_base_path = app_root.string();
                                },
                                {"pixils/test/app/main.lisple"}));

  Pixils::Runtime::HookArguments hook_args{
    Pixils::Script::HookContextAdapter::make_ref(hook_ctx)};
  hook_args.events = &events;

  Pixils::Runtime::Session session(*runtime,
                                   *render_ctx.asset_registry,
                                   render_ctx,
                                   hook_args);

  auto modes = runtime->lookup_value("pixils/modes");
  auto mode_val = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol("test-mode"));
  auto& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);

  auto view = std::make_shared<Pixils::Runtime::View>();
  view->mode = &mode;
  view->state = Lisple::Constant::NIL;

  auto result =
    session.invoke_hook(view, mode.init, session.hook_args.init_args, Lisple::Constant::NIL);
  auto ticks = Lisple::Dict::get_property(result, Lisple::RTValue::keyword("ticks"));

  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);

  std::filesystem::remove_all(app_root);
}
