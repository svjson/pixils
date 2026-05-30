#include "app_manifest.h"
#include "composable_app_session_fixture.h"
#include <pixils/program.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/view.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <sdl2_mock/mock_resources.h>

namespace AppFixture = Pixils::Test::AppFixture;

namespace
{
  std::filesystem::path fixture_dir()
  {
    return std::filesystem::path(__FILE__).parent_path();
  }

  AppFixture::SourceUnit inline_unit(const std::string& id,
                                     const std::vector<std::string>& require_entries,
                                     const std::vector<std::string>& body)
  {
    return AppFixture::SourceUnit::inline_unit(id, require_entries, body);
  }

  AppFixture::AppManifest simple_session_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("main-api", {}, {"(def answer 42)"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.roo",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"main-api"}}});
  }

  AppFixture::AppManifest program_boot_session_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit(
        "theme-and-program-api",
        {"pixils"},
        {"(pixils/deftheme app-theme {:styles {'root-mode {:background {:r 1 :g 2 :b 3}}}})",
         "(pixils/defmode root-mode {:init (fn [state ctx] {:ticks 2})})",
         "(pixils/defprogram app {:initial-mode 'root-mode :theme 'app-theme})"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.roo",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"theme-and-program-api"}}});
  }

  AppFixture::AppManifest program_without_initial_mode_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("program-api",
                   {"pixils"},
                   {"(pixils/defmode root-mode {:init (fn [state ctx] {:ticks 2})})",
                    "(pixils/defprogram app {})"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.roo",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"program-api"}}});
  }

  AppFixture::AppManifest file_defined_mode_session_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("test-mode-api",
                   {"pixils"},
                   {"(pixils/defmode test-mode {:init (fn [state ctx] {:ticks 2})})"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.roo",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"test-mode-api"}}});
  }

  AppFixture::AppManifest file_backed_mode_session_manifest()
  {
    return AppFixture::AppManifest(
      {AppFixture::SourceUnit::from_file(
        "test-mode-api",
        fixture_dir() / "assets" / "sample_mode_unit.roo")},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.roo",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"test-mode-api"}}});
  }

  AppFixture::AppManifest file_defined_counter_session_manifest()
  {
    AppFixture::AppManifest manifest;

    manifest.upsert_unit(AppFixture::SourceUnit::from_file("win311-theme",
                                                           fixture_dir() / "assets" /
                                                             "shared" / "ui" / "themes" /
                                                             "win311" / "win-theme.roo"));
    manifest.upsert_unit(
      AppFixture::SourceUnit::from_file("text-node-component",
                                        fixture_dir() / "assets" / "shared" / "ui" /
                                          "components" / "text-node" / "text-node.roo"));
    manifest.upsert_unit(
      AppFixture::SourceUnit::from_file("counter-bundle",
                                        fixture_dir() / "assets" / "apps" / "minesweeper" /
                                          "bundles" / "counter" / "counter.roo"));
    manifest.upsert_unit(AppFixture::SourceUnit::from_file(
      "status-panel-left-pad",
      fixture_dir() / "assets" / "apps" / "minesweeper" / "components" / "status-panel" /
        "left-pad.roo"));
    manifest.upsert_unit(AppFixture::SourceUnit::from_file(
      "counter-component",
      fixture_dir() / "assets" / "apps" / "minesweeper" / "components" / "status-panel" /
        "counter" / "counter.roo"));

    manifest.add_file(
      AppFixture::ManifestFile{.id = "shared-win311",
                               .disk_path = "pixils/test/app/shared/ui/themes/win311.roo",
                               .namespace_name = "pixils.test.app.shared.ui.themes.win311",
                               .unit_ids = {"win311-theme"}});
    manifest.add_file(AppFixture::ManifestFile{
      .id = "shared-text-node",
      .disk_path = "pixils/test/app/shared/ui/components/text-node.roo",
      .namespace_name = "pixils.test.app.shared.ui.components.text-node",
      .unit_ids = {"text-node-component"}});
    manifest.add_file(
      AppFixture::ManifestFile{.id = "main",
                               .disk_path = "pixils/test/app/minesweeper/core.roo",
                               .namespace_name = "pixils.test.app.minesweeper.core",
                               .unit_ids = {
                                 "counter-bundle",
                                 "status-panel-left-pad",
                                 "counter-component",
                               }});

    return manifest;
  }
} // namespace

class ComposableAppSessionFixtureTest : public ComposableAppSessionFixture
{
};

TEST_F(ComposableAppSessionFixtureTest,
       loads_composed_app_and_drives_full_session_frame_cycle)
{
  load_app(simple_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});
  EXPECT_EQ(eval("pixils.test.app.main/answer")->num().get_int(), 42);

  pixils().eval(R"(
    (pixils/defmode test-mode {
      :init   (fn [state ctx] {:ticks 2})
      :update (fn [state ctx] (assoc state :ticks (+ (:ticks state) 1)))
      :render (fn [state ctx]
                (pixils.render/rect!
                  {:x 0 :y 0}
                  {:x (:ticks state) :y (:ticks state)}
                  {:fill true}))
    })
  )");

  session().push_mode("test-mode", Roo::Constant::NIL);
  session().update_mode();
  session().render_mode();

  auto ticks =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 3);

  auto* target = render_target();
  ASSERT_NE(target, nullptr);
  ASSERT_EQ(target->render_ops.size(), 1u);
  EXPECT_EQ(target->render_ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(target->render_ops[0].rendered_rect.w, 3);
  EXPECT_EQ(target->render_ops[0].rendered_rect.h, 3);
}

TEST_F(ComposableAppSessionFixtureTest, loads_composed_app_and_pushes_mode_into_session)
{
  load_app(simple_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});
  EXPECT_EQ(eval("pixils.test.app.main/answer")->num().get_int(), 42);

  pixils().eval(R"(
    (pixils/defmode test-mode {
      :init (fn [state ctx] {:ticks 2})
    })
  )");

  ASSERT_NO_THROW(session().push_mode("test-mode", Roo::Constant::NIL));
  ASSERT_NE(session().active_mode, nullptr);

  auto ticks =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);
}

TEST_F(ComposableAppSessionFixtureTest, loads_composed_app_and_updates_mode_in_session)
{
  load_app(simple_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});
  EXPECT_EQ(eval("pixils.test.app.main/answer")->num().get_int(), 42);

  pixils().eval(R"(
    (pixils/defmode test-mode {
      :init   (fn [state ctx] {:ticks 2})
      :update (fn [state ctx] (assoc state :ticks (+ (:ticks state) 1)))
    })
  )");

  session().push_mode("test-mode", Roo::Constant::NIL);
  ASSERT_NO_THROW(session().update_mode());

  auto ticks =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 3);
}

TEST_F(ComposableAppSessionFixtureTest, stages_runtime_assets_into_generated_app_root)
{
  AppFixture::AppManifest manifest(
    {inline_unit("main-api", {}, {"(def answer 42)"})},
    {AppFixture::ManifestFile{.id = "main",
                              .disk_path = "pixils/test/app/main.roo",
                              .namespace_name = "pixils.test.app.main",
                              .unit_ids = {"main-api"}}});

  load_app(manifest,
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"},
           {{std::filesystem::path(__FILE__).parent_path() / "assets" / "sample_unit.roo",
             "assets/copied/sample_unit.roo"}});

  EXPECT_TRUE(std::filesystem::exists(app_root_dir() / "assets/copied/sample_unit.roo"));
}

TEST_F(ComposableAppSessionFixtureTest,
       pushes_file_defined_mode_from_generated_file_into_session)
{
  load_app(file_defined_mode_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  ASSERT_NO_THROW(session().push_mode("test-mode", Roo::Constant::NIL));
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "test-mode");

  auto ticks =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);
}

TEST_F(ComposableAppSessionFixtureTest,
       pushes_file_defined_mode_from_file_backed_unit_into_session)
{
  load_app(file_backed_mode_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  ASSERT_NO_THROW(session().push_mode("test-mode", Roo::Constant::NIL));
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "test-mode");

  auto ticks =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);
}

TEST_F(ComposableAppSessionFixtureTest,
       pushes_file_defined_counter_component_with_injected_state_into_session)
{
  load_app(file_defined_counter_session_manifest(),
           "pixils.test.app.minesweeper.core",
           {"pixils/test/app/shared/ui/themes/win311.roo",
            "pixils/test/app/shared/ui/components/text-node.roo",
            "pixils/test/app/minesweeper/core.roo"});

  ASSERT_NO_THROW(session().push_mode("counter", eval("{:value 123}")));
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "counter");

  auto value =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 123);
}

TEST_F(ComposableAppSessionFixtureTest,
       invokes_file_defined_mode_init_hook_directly_in_fixture_session)
{
  load_app(file_defined_mode_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  auto modes = pixils().lookup("pixils/modes");
  auto mode_val = Roo::Dict::get_property(modes, Roo::symbol("test-mode"));
  ASSERT_NE(mode_val, nullptr);

  auto& mode = Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  auto view = std::make_shared<Pixils::Runtime::View>();
  view->mode = &mode;
  view->state = Roo::Constant::NIL;

  Roo::sptr_val result;
  ASSERT_NO_THROW(result = Pixils::Runtime::invoke_hook(pixils(),
                                                        view,
                                                        mode.init,
                                                        session().hook_args.init_args));

  auto ticks = Roo::Dict::get_property(result, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);
}

TEST_F(ComposableAppSessionFixtureTest,
       invokes_file_defined_mode_init_hook_in_fresh_local_session_after_load_app)
{
  load_app(file_defined_mode_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  Pixils::HookContext local_hook_ctx{&events, &render_ctx, nullptr};
  Pixils::Runtime::HookArguments local_hook_args{
    Pixils::Script::HookContextAdapter::make_ref(local_hook_ctx)};
  local_hook_args.events = &events;

  Pixils::Runtime::Session local_session(pixils(),
                                         *render_ctx.asset_registry,
                                         render_ctx,
                                         local_hook_args);

  auto modes = pixils().lookup("pixils/modes");
  auto mode_val = Roo::Dict::get_property(modes, Roo::symbol("test-mode"));
  ASSERT_NE(mode_val, nullptr);

  auto& mode = Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  auto view = std::make_shared<Pixils::Runtime::View>();
  view->mode = &mode;
  view->state = Roo::Constant::NIL;

  Roo::sptr_val result;
  ASSERT_NO_THROW(result = Pixils::Runtime::invoke_hook(pixils(),
                                                        view,
                                                        mode.init,
                                                        local_session.hook_args.init_args));

  auto ticks = Roo::Dict::get_property(result, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);
}

TEST_F(ComposableAppSessionFixtureTest,
       load_program_pushes_declared_initial_mode_into_session)
{
  load_app(program_boot_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  Pixils::Program& program = load_program();

  EXPECT_EQ(program.initial_mode, "root-mode");
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "root-mode");

  auto ticks =
    Roo::Dict::get_property(session().active_mode->state, Roo::keyword("ticks"));
  ASSERT_NE(ticks, nullptr);
  EXPECT_EQ(ticks->num().get_int(), 2);
}

TEST_F(ComposableAppSessionFixtureTest,
       load_program_falls_back_to_first_mode_when_program_initial_mode_missing)
{
  load_app(program_without_initial_mode_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  Pixils::Program& program = load_program();

  EXPECT_FALSE(program.initial_mode.empty());
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, program.initial_mode);
}

TEST_F(ComposableAppSessionFixtureTest,
       load_program_creates_default_program_when_none_is_declared)
{
  load_app(file_defined_mode_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  Pixils::Program& program = load_program();

  EXPECT_FALSE(program.initial_mode.empty());
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, program.initial_mode);
}

TEST_F(ComposableAppSessionFixtureTest,
       load_program_applies_program_theme_override_to_root_view)
{
  load_app(program_boot_session_manifest(),
           "pixils.test.app.main",
           {"pixils/test/app/main.roo"});

  load_program();
  render_cycle();

  ASSERT_NE(session().active_mode, nullptr);
  const auto& style = session().active_mode->effective_style;
  ASSERT_TRUE(style.background.has_value());
  ASSERT_TRUE(style.background->color.has_value());
  EXPECT_EQ(*style.background->color, (Pixils::Color{1, 2, 3, 255}));
}
