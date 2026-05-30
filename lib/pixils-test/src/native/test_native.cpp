#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/keyboard.h>
#include <pixils/runtime/hook_arguments.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/script.h>

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <cctype>
#include <filesystem>
#include <lisple-package/manifest.h>
#include <lisple-package/native_abi.h>
#include <lisple/exception.h>
#include <lisple/exec.h>
#include <lisple/host/object.h>
#include <lisple/io/dir_root_file_system.h>
#include <lisple/namespace.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
  struct AppTarget
  {
    std::filesystem::path root;
    std::filesystem::path asset_base_path;
    std::vector<std::string> load_path;
    std::vector<Lisple::NamespaceRoot> namespace_roots;
    std::vector<std::string> entry_points;
    int buffer_width = 800;
    int buffer_height = 600;
  };

  class TestApp
  {
   public:
    Pixils::RenderContext render_ctx{};
    Pixils::FrameEvents events;
    Pixils::HookContext hook_ctx;
    std::unique_ptr<Lisple::Runtime> runtime;
    Pixils::Runtime::HookArguments hook_args;
    std::unique_ptr<Pixils::Runtime::Session> session;

    explicit TestApp(const AppTarget& target)
      : hook_ctx{&events, &render_ctx}
      , runtime(Pixils::make_lisple_runtime(render_ctx,
                                            "pixils.test.runtime.fixture",
                                            [&target](Pixils::RuntimeConfiguration* cfg)
                                            {
                                              cfg->load_path = target.load_path;
                                              cfg->namespace_roots = target.namespace_roots;
                                              cfg->asset_base_path =
                                                target.asset_base_path.string();
                                            },
                                            {}))
      , hook_args{Pixils::Script::HookContextAdapter::make_ref(hook_ctx)}
    {
      render_ctx.buffer_dim = {target.buffer_width, target.buffer_height};

      if (!render_ctx.asset_registry)
      {
        render_ctx.asset_registry =
          std::make_unique<Pixils::Asset::Registry>(render_ctx,
                                                    target.asset_base_path.string());
      }
      if (!render_ctx.font_registry)
      {
        render_ctx.font_registry = std::make_unique<Pixils::FontRegistry>();
      }

      session = std::make_unique<Pixils::Runtime::Session>(*runtime,
                                                           *render_ctx.asset_registry,
                                                           render_ctx,
                                                           hook_args);
      session->hook_args.events = &events;

      for (const auto& entry_point : target.entry_points)
      {
        runtime->eval(
          "(ns pixils.test.runtime.fixture-entry (:require " + entry_point + "))",
          "<pixils-test-runtime-entry>");
      }
    }

    void clear_transients()
    {
      events.mouse_button_down = Lisple::Constant::NIL;
      events.mouse_button_up = Lisple::Constant::NIL;
      events.mouse_button_down_clicks = 1;
      events.mouse_button_up_clicks = 1;
      events.mouse_moved = false;
      events.key_down = Lisple::Constant::NIL;
      events.key_up = Lisple::Constant::NIL;
    }

    void update()
    {
      session->update_mode();
      session->process_messages();
      clear_transients();
    }

    void render()
    {
      session->render_mode();
      clear_transients();
    }

    void frame()
    {
      session->update_mode();
      session->process_messages();
      session->render_mode();
      clear_transients();
    }
  };

  std::string package_last_error;

  namespace HostType
  {
    HOST_TYPE(TEST_APP, "HPixilsTestApp");
  }

  NATIVE_ADAPTER(TestAppAdapter, TestApp);
  NATIVE_ADAPTER_IMPL(TestAppAdapter, TestApp, &HostType::TEST_APP);

  std::string required_string(const Lisple::sptr_val& value, const std::string& label)
  {
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      throw Lisple::InvocationException(label + " is required");
    }
    if (value->type != Lisple::Value::Type::STRING)
    {
      throw Lisple::TypeError(label + " must be a string");
    }
    return value->str();
  }

  std::string value_name(const Lisple::sptr_val& value, const std::string& label)
  {
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      throw Lisple::InvocationException(label + " is required");
    }
    if (value->type == Lisple::Value::Type::STRING ||
        value->type == Lisple::Value::Type::SYMBOL)
    {
      return value->str();
    }
    if (value->type == Lisple::Value::Type::KEYWORD)
    {
      return value->str();
    }
    throw Lisple::TypeError(label + " must be a string, symbol, or keyword");
  }

  std::string mode_name(const Lisple::sptr_val& value)
  {
    std::string name = value_name(value, "mode");
    if (!name.empty() && name[0] == ':') return name.substr(1);
    return name;
  }

  std::optional<std::string> optional_string_property(const Lisple::sptr_val& map,
                                                      const std::string& key)
  {
    auto value = Lisple::Dict::get_property(map, Lisple::keyword(key));
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      return std::nullopt;
    }
    return required_string(value, ":" + key);
  }

  std::vector<std::string> string_vector_property(const Lisple::sptr_val& map,
                                                  const std::string& key)
  {
    auto value = Lisple::Dict::get_property(map, Lisple::keyword(key));
    std::vector<std::string> result;
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      return result;
    }
    if (value->type != Lisple::Value::Type::VECTOR)
    {
      throw Lisple::TypeError(":" + key + " must be a vector");
    }
    for (const auto& entry : value->elements())
    {
      result.push_back(value_name(entry, ":" + key + " entry"));
    }
    return result;
  }

  int optional_int_property(const Lisple::sptr_val& map,
                            const std::string& key,
                            int fallback)
  {
    auto value = Lisple::Dict::get_property(map, Lisple::keyword(key));
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      return fallback;
    }
    if (value->type != Lisple::Value::Type::NUMBER)
    {
      throw Lisple::TypeError(":" + key + " must be a number");
    }
    return value->num().get_int();
  }

  int required_int(const Lisple::sptr_val& value, const std::string& label)
  {
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      throw Lisple::InvocationException(label + " is required");
    }
    if (value->type != Lisple::Value::Type::NUMBER)
    {
      throw Lisple::TypeError(label + " must be a number");
    }
    return value->num().get_int();
  }

  std::string normalize_input_name(const Lisple::sptr_val& value, const std::string& label)
  {
    auto name = value_name(value, label);
    if (!name.empty() && name[0] == ':') name = name.substr(1);
    auto slash = name.find('/');
    if (slash != std::string::npos) name = name.substr(slash + 1);
    return name;
  }

  Uint8 mouse_button_value(const Lisple::sptr_val& value)
  {
    auto name = normalize_input_name(value, "button");
    if (name == "left") return SDL_BUTTON_LEFT;
    if (name == "right") return SDL_BUTTON_RIGHT;
    if (name == "middle") return SDL_BUTTON_MIDDLE;
    throw Lisple::InvocationException("unsupported mouse button: " + name);
  }

  SDL_Keycode keycode_value(const Lisple::sptr_val& value)
  {
    auto name = normalize_input_name(value, "key");
    auto it = Pixils::Keyboard::SYMBOL_TO_KEYCODE.find(name);
    if (it != Pixils::Keyboard::SYMBOL_TO_KEYCODE.end()) return it->second;

    if (name.size() == 1)
    {
      std::string upper;
      upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(name[0]))));
      it = Pixils::Keyboard::SYMBOL_TO_KEYCODE.find(upper);
      if (it != Pixils::Keyboard::SYMBOL_TO_KEYCODE.end()) return it->second;
    }

    throw Lisple::InvocationException("unsupported key: " + name);
  }

  AppTarget package_target(const Lisple::sptr_val& opts)
  {
    auto package_root_value =
      Lisple::Dict::get_property(opts, Lisple::keyword("package-root"));
    auto root =
      std::filesystem::canonical(required_string(package_root_value, ":package-root"));

    Lisple::DirRootFileSystem manifest_fs("/");
    auto manifest =
      Lisple::Package::read_manifest(manifest_fs, (root / "package.edn").string());
    auto plan = Lisple::Package::resolve_load_plan(manifest_fs, root.string());

    auto asset_base_path = root;
    if (auto explicit_asset_base = optional_string_property(opts, "asset-base-path"))
    {
      asset_base_path = std::filesystem::canonical(*explicit_asset_base);
    }
    else if (!manifest.load_roots.empty())
    {
      asset_base_path = root / manifest.load_roots.front();
    }

    auto entry_points = string_vector_property(opts, "entry-points");
    if (entry_points.empty())
    {
      entry_points = manifest.entry_points;
    }

    return AppTarget{
      .root = root,
      .asset_base_path = asset_base_path,
      .load_path =
        Lisple::Package::merge_load_paths(plan,
                                          {std::filesystem::current_path().string(), "/"}),
      .namespace_roots = plan.namespace_roots,
      .entry_points = entry_points,
      .buffer_width = optional_int_property(opts, "buffer-width", 800),
      .buffer_height = optional_int_property(opts, "buffer-height", 600),
    };
  }

  TestApp& app_from(const Lisple::sptr_val& value)
  {
    return Lisple::obj<TestApp>(*value);
  }

  Pixils::Runtime::View& view_from(const Lisple::sptr_val& value)
  {
    return Lisple::obj<Pixils::Runtime::View>(*value);
  }

  std::shared_ptr<Pixils::Runtime::View> find_descendant(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& wanted_mode,
    const std::optional<std::string>& state_text)
  {
    if (!view) return nullptr;

    const bool mode_matches = view->mode && view->mode->name == wanted_mode;
    const bool state_matches =
      !state_text.has_value() ||
      (view->state && view->state->to_string().find(*state_text) != std::string::npos);
    if (mode_matches && state_matches)
    {
      return view;
    }

    for (const auto& child : view->children)
    {
      auto found = find_descendant(child, wanted_mode, state_text);
      if (found) return found;
    }
    return nullptr;
  }

  int view_center_x(const Pixils::Runtime::View& view)
  {
    return static_cast<int>(view.bounds.x + view.bounds.w / 2);
  }

  int view_center_y(const Pixils::Runtime::View& view)
  {
    return static_cast<int>(view.bounds.y + view.bounds.h / 2);
  }

  void mouse_move(TestApp& app, int x, int y)
  {
    app.events.do_mouse_motion(x, y);
  }

  void mouse_down(TestApp& app, int x, int y, Uint8 button)
  {
    mouse_move(app, x, y);
    SDL_MouseButtonEvent event{};
    event.button = button;
    event.clicks = 1;
    app.events.do_mouse_button_down(event);
  }

  void mouse_up(TestApp& app, int x, int y, Uint8 button)
  {
    mouse_move(app, x, y);
    SDL_MouseButtonEvent event{};
    event.button = button;
    event.clicks = 1;
    app.events.do_mouse_button_up(event);
  }

  void key_down(TestApp& app, SDL_Keycode key)
  {
    SDL_KeyboardEvent event{};
    event.keysym.sym = key;
    app.events.do_key_down(event);
  }

  void key_up(TestApp& app, SDL_Keycode key)
  {
    SDL_KeyboardEvent event{};
    event.keysym.sym = key;
    app.events.do_key_up(event);
  }

  namespace Function
  {
    FUNC(MakeAppFunction, make_app);
    FUNC(EvalBangFunction, eval_bang);
    FUNC(PushModeBangFunction, push_mode);
    FUNC(PopModeBangFunction, pop_mode);
    FUNC(UpdateBangFunction, update_bang);
    FUNC(RenderBangFunction, render_bang);
    FUNC(FrameBangFunction, frame_bang);
    FUNC(ActiveModeFunction, active_mode);
    FUNC(ActiveModeNameFunction, active_mode_name);
    FUNC(ActiveStateFunction, active_state);
    FUNC(SetActiveStateBangFunction, set_active_state);
    FUNC(ViewModeNameFunction, view_mode_name);
    FUNC(ViewStateFunction, view_state);
    FUNC(FindViewFunction, find_view);
    FUNC(ClickBangFunction, click_bang);
    FUNC(MouseDownAtBangFunction, mouse_down_at);
    FUNC(MouseUpAtBangFunction, mouse_up_at);
    FUNC(MouseClickAtBangFunction, mouse_click_at);
    FUNC(MouseDownInBangFunction, mouse_down_in);
    FUNC(MouseUpInBangFunction, mouse_up_in);
    FUNC(MouseClickInBangFunction, mouse_click_in);
    FUNC(KeyDownBangFunction, key_down);
    FUNC(KeyUpBangFunction, key_up);

    FUNC_IMPL(MakeAppFunction,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeAppFunction::exec_make_app))));
    EXEC_BODY(MakeAppFunction, exec_make_app)
    {
      return TestAppAdapter::make_unique(package_target(args[0]));
    }

    FUNC_IMPL(EvalBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::STRING)),
                   EXEC_DISPATCH(&EvalBangFunction::exec_eval_bang))));
    EXEC_BODY(EvalBangFunction, exec_eval_bang)
    {
      return app_from(args[0]).runtime->eval(args[1]->str());
    }

    FUNC_IMPL(
      PushModeBangFunction,
      MULTI_SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::ANY)),
                 EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode)),
                (FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::ANY), (&Lisple::Type::ANY)),
                 EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode)),
                (FN_ARGS((&HostType::TEST_APP),
                         (&Lisple::Type::ANY),
                         (&Lisple::Type::ANY),
                         (&Lisple::Type::ANY)),
                 EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode))));
    EXEC_BODY(PushModeBangFunction, exec_push_mode)
    {
      auto& app = app_from(args[0]);
      auto state = args.size() > 2 ? args[2] : Lisple::Constant::NIL;
      auto overrides = args.size() > 3 ? args[3] : Lisple::Constant::NIL;
      app.session->push_mode(mode_name(args[1]), state, overrides);
      return args[0];
    }

    FUNC_IMPL(PopModeBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::ANY)),
                   EXEC_DISPATCH(&PopModeBangFunction::exec_pop_mode))));
    EXEC_BODY(PopModeBangFunction, exec_pop_mode)
    {
      app_from(args[0]).session->pop_mode(args[1]);
      return args[0];
    }

    FUNC_IMPL(UpdateBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP)),
                   EXEC_DISPATCH(&UpdateBangFunction::exec_update_bang))));
    EXEC_BODY(UpdateBangFunction, exec_update_bang)
    {
      app_from(args[0]).update();
      return args[0];
    }

    FUNC_IMPL(RenderBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP)),
                   EXEC_DISPATCH(&RenderBangFunction::exec_render_bang))));
    EXEC_BODY(RenderBangFunction, exec_render_bang)
    {
      app_from(args[0]).render();
      return args[0];
    }

    FUNC_IMPL(FrameBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP)),
                   EXEC_DISPATCH(&FrameBangFunction::exec_frame_bang))));
    EXEC_BODY(FrameBangFunction, exec_frame_bang)
    {
      app_from(args[0]).frame();
      return args[0];
    }

    FUNC_IMPL(ActiveModeFunction,
              SIG((FN_ARGS((&HostType::TEST_APP)),
                   EXEC_DISPATCH(&ActiveModeFunction::exec_active_mode))));
    EXEC_BODY(ActiveModeFunction, exec_active_mode)
    {
      auto& app = app_from(args[0]);
      if (!app.session->active_mode) return Lisple::Constant::NIL;
      return Pixils::Script::ViewAdapter::make_ref(*app.session->active_mode);
    }

    FUNC_IMPL(ActiveModeNameFunction,
              SIG((FN_ARGS((&HostType::TEST_APP)),
                   EXEC_DISPATCH(&ActiveModeNameFunction::exec_active_mode_name))));
    EXEC_BODY(ActiveModeNameFunction, exec_active_mode_name)
    {
      auto& app = app_from(args[0]);
      if (!app.session->active_mode || !app.session->active_mode->mode)
      {
        return Lisple::Constant::NIL;
      }
      return Lisple::string(app.session->active_mode->mode->name);
    }

    FUNC_IMPL(ActiveStateFunction,
              SIG((FN_ARGS((&HostType::TEST_APP)),
                   EXEC_DISPATCH(&ActiveStateFunction::exec_active_state))));
    EXEC_BODY(ActiveStateFunction, exec_active_state)
    {
      auto& app = app_from(args[0]);
      if (!app.session->active_mode) return Lisple::Constant::NIL;
      return app.session->active_mode->state;
    }

    FUNC_IMPL(SetActiveStateBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::ANY)),
                   EXEC_DISPATCH(&SetActiveStateBangFunction::exec_set_active_state))));
    EXEC_BODY(SetActiveStateBangFunction, exec_set_active_state)
    {
      auto& app = app_from(args[0]);
      if (!app.session->active_mode)
      {
        throw Lisple::InvocationException(
          "pixils.test.runtime/set-active-state! has no active mode");
      }
      app.session->active_mode->state = args[1];
      return args[0];
    }

    FUNC_IMPL(ViewModeNameFunction,
              SIG((FN_ARGS((&Pixils::Script::HostType::VIEW)),
                   EXEC_DISPATCH(&ViewModeNameFunction::exec_view_mode_name))));
    EXEC_BODY(ViewModeNameFunction, exec_view_mode_name)
    {
      auto& view = view_from(args[0]);
      if (!view.mode) return Lisple::Constant::NIL;
      return Lisple::string(view.mode->name);
    }

    FUNC_IMPL(ViewStateFunction,
              SIG((FN_ARGS((&Pixils::Script::HostType::VIEW)),
                   EXEC_DISPATCH(&ViewStateFunction::exec_view_state))));
    EXEC_BODY(ViewStateFunction, exec_view_state)
    {
      return view_from(args[0]).state;
    }

    FUNC_IMPL(FindViewFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::STRING)),
                         EXEC_DISPATCH(&FindViewFunction::exec_find_view)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::STRING),
                                 (&Lisple::Type::STRING)),
                         EXEC_DISPATCH(&FindViewFunction::exec_find_view))));
    EXEC_BODY(FindViewFunction, exec_find_view)
    {
      auto& app = app_from(args[0]);
      const auto state_text =
        args.size() > 2 ? std::optional<std::string>(args[2]->str()) : std::nullopt;
      auto found = find_descendant(app.session->active_mode, args[1]->str(), state_text);
      if (!found) return Lisple::Constant::NIL;
      return Pixils::Script::ViewAdapter::make_ref(*found);
    }

    FUNC_IMPL(ClickBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP), (&Pixils::Script::HostType::VIEW)),
                   EXEC_DISPATCH(&ClickBangFunction::exec_click_bang))));
    EXEC_BODY(ClickBangFunction, exec_click_bang)
    {
      auto& app = app_from(args[0]);
      auto& view = view_from(args[1]);
      const int x = view_center_x(view);
      const int y = view_center_y(view);
      mouse_down(app, x, y, SDL_BUTTON_LEFT);
      app.update();
      mouse_up(app, x, y, SDL_BUTTON_LEFT);
      app.update();
      return args[0];
    }

    FUNC_IMPL(MouseDownAtBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MouseDownAtBangFunction::exec_mouse_down_at)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&MouseDownAtBangFunction::exec_mouse_down_at))));
    EXEC_BODY(MouseDownAtBangFunction, exec_mouse_down_at)
    {
      auto& app = app_from(args[0]);
      auto button = args.size() > 3 ? mouse_button_value(args[3]) : SDL_BUTTON_LEFT;
      mouse_down(app, required_int(args[1], "x"), required_int(args[2], "y"), button);
      return args[0];
    }

    FUNC_IMPL(MouseUpAtBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MouseUpAtBangFunction::exec_mouse_up_at)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&MouseUpAtBangFunction::exec_mouse_up_at))));
    EXEC_BODY(MouseUpAtBangFunction, exec_mouse_up_at)
    {
      auto& app = app_from(args[0]);
      auto button = args.size() > 3 ? mouse_button_value(args[3]) : SDL_BUTTON_LEFT;
      mouse_up(app, required_int(args[1], "x"), required_int(args[2], "y"), button);
      return args[0];
    }

    FUNC_IMPL(MouseClickAtBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MouseClickAtBangFunction::exec_mouse_click_at)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&MouseClickAtBangFunction::exec_mouse_click_at))));
    EXEC_BODY(MouseClickAtBangFunction, exec_mouse_click_at)
    {
      auto& app = app_from(args[0]);
      const int x = required_int(args[1], "x");
      const int y = required_int(args[2], "y");
      auto button = args.size() > 3 ? mouse_button_value(args[3]) : SDL_BUTTON_LEFT;
      mouse_down(app, x, y, button);
      app.update();
      mouse_up(app, x, y, button);
      app.update();
      return args[0];
    }

    FUNC_IMPL(MouseDownInBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP),
                                 (&Pixils::Script::HostType::VIEW),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MouseDownInBangFunction::exec_mouse_down_in)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Pixils::Script::HostType::VIEW),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&MouseDownInBangFunction::exec_mouse_down_in))));
    EXEC_BODY(MouseDownInBangFunction, exec_mouse_down_in)
    {
      auto& app = app_from(args[0]);
      auto& view = view_from(args[1]);
      auto button = args.size() > 4 ? mouse_button_value(args[4]) : SDL_BUTTON_LEFT;
      mouse_down(app,
                 static_cast<int>(view.bounds.x) + required_int(args[2], "x"),
                 static_cast<int>(view.bounds.y) + required_int(args[3], "y"),
                 button);
      return args[0];
    }

    FUNC_IMPL(MouseUpInBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP),
                                 (&Pixils::Script::HostType::VIEW),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MouseUpInBangFunction::exec_mouse_up_in)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Pixils::Script::HostType::VIEW),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&MouseUpInBangFunction::exec_mouse_up_in))));
    EXEC_BODY(MouseUpInBangFunction, exec_mouse_up_in)
    {
      auto& app = app_from(args[0]);
      auto& view = view_from(args[1]);
      auto button = args.size() > 4 ? mouse_button_value(args[4]) : SDL_BUTTON_LEFT;
      mouse_up(app,
               static_cast<int>(view.bounds.x) + required_int(args[2], "x"),
               static_cast<int>(view.bounds.y) + required_int(args[3], "y"),
               button);
      return args[0];
    }

    FUNC_IMPL(MouseClickInBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::TEST_APP),
                                 (&Pixils::Script::HostType::VIEW),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MouseClickInBangFunction::exec_mouse_click_in)),
                        (FN_ARGS((&HostType::TEST_APP),
                                 (&Pixils::Script::HostType::VIEW),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&MouseClickInBangFunction::exec_mouse_click_in))));
    EXEC_BODY(MouseClickInBangFunction, exec_mouse_click_in)
    {
      auto& app = app_from(args[0]);
      auto& view = view_from(args[1]);
      const int x = static_cast<int>(view.bounds.x) + required_int(args[2], "x");
      const int y = static_cast<int>(view.bounds.y) + required_int(args[3], "y");
      auto button = args.size() > 4 ? mouse_button_value(args[4]) : SDL_BUTTON_LEFT;
      mouse_down(app, x, y, button);
      app.update();
      mouse_up(app, x, y, button);
      app.update();
      return args[0];
    }

    FUNC_IMPL(KeyDownBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::ANY)),
                   EXEC_DISPATCH(&KeyDownBangFunction::exec_key_down))));
    EXEC_BODY(KeyDownBangFunction, exec_key_down)
    {
      key_down(app_from(args[0]), keycode_value(args[1]));
      return args[0];
    }

    FUNC_IMPL(KeyUpBangFunction,
              SIG((FN_ARGS((&HostType::TEST_APP), (&Lisple::Type::ANY)),
                   EXEC_DISPATCH(&KeyUpBangFunction::exec_key_up))));
    EXEC_BODY(KeyUpBangFunction, exec_key_up)
    {
      key_up(app_from(args[0]), keycode_value(args[1]));
      return args[0];
    }
  } // namespace Function

  class RuntimeNativeNamespace : public Lisple::Namespace
  {
   public:
    RuntimeNativeNamespace()
      : Lisple::Namespace("pixils.test.runtime.native")
    {
      values.emplace("make-app", Function::MakeAppFunction::make());
      values.emplace("eval!", Function::EvalBangFunction::make());
      values.emplace("push-mode!", Function::PushModeBangFunction::make());
      values.emplace("pop-mode!", Function::PopModeBangFunction::make());
      values.emplace("update!", Function::UpdateBangFunction::make());
      values.emplace("render!", Function::RenderBangFunction::make());
      values.emplace("frame!", Function::FrameBangFunction::make());
      values.emplace("active-mode", Function::ActiveModeFunction::make());
      values.emplace("active-mode-name", Function::ActiveModeNameFunction::make());
      values.emplace("active-state", Function::ActiveStateFunction::make());
      values.emplace("set-active-state!", Function::SetActiveStateBangFunction::make());
      values.emplace("view-mode-name", Function::ViewModeNameFunction::make());
      values.emplace("view-state", Function::ViewStateFunction::make());
      values.emplace("find-view", Function::FindViewFunction::make());
      values.emplace("click!", Function::ClickBangFunction::make());
      values.emplace("mouse-down-at!", Function::MouseDownAtBangFunction::make());
      values.emplace("mouse-up-at!", Function::MouseUpAtBangFunction::make());
      values.emplace("mouse-click-at!", Function::MouseClickAtBangFunction::make());
      values.emplace("mouse-down-in!", Function::MouseDownInBangFunction::make());
      values.emplace("mouse-up-in!", Function::MouseUpInBangFunction::make());
      values.emplace("mouse-click-in!", Function::MouseClickInBangFunction::make());
      values.emplace("key-down!", Function::KeyDownBangFunction::make());
      values.emplace("key-up!", Function::KeyUpBangFunction::make());
    }
  };

  int load_native_package(const LispleNativeHostV1* host)
  {
    try
    {
      auto ns = std::make_unique<RuntimeNativeNamespace>();
      ns->set_origin(Lisple::Namespace::Origin::native());
      if (host->register_namespace(host->user, ns.release()) != 0)
      {
        return 1;
      }
      return 0;
    }
    catch (const std::exception& e)
    {
      package_last_error = e.what();
      return 1;
    }
  }

  void unload_native_package()
  {
    package_last_error.clear();
  }

  const char* last_error()
  {
    return package_last_error.c_str();
  }
} // namespace

extern "C" LISPLE_NATIVE_EXPORT const LispleNativePackageV1* lisple_native_package_v1()
{
  static const LispleNativePackageV1 package{
    LISPLE_NATIVE_ABI_VERSION,
    sizeof(LispleNativePackageV1),
    "pixils-test-native",
    "0.1.0",
    LISPLE_NATIVE_CXX_ABI,
    load_native_package,
    unload_native_package,
    last_error,
  };
  return &package;
}
