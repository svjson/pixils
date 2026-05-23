#include <pixils/client.h>
#include <pixils/context.h>
#include <pixils/init_sdl.h>
#include <pixils/script.h>

#include <SDL2/SDL.h>
#include <filesystem>
#include <lisple-package/manifest.h>
#include <lisple-package/native_abi.h>
#include <lisple/exec.h>
#include <lisple/io/dir_root_file_system.h>
#include <lisple/namespace.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace
{
  struct LaunchTarget
  {
    std::filesystem::path asset_base_path;
    std::vector<std::string> load_path;
    std::vector<Lisple::NamespaceRoot> namespace_roots;
    std::vector<std::string> source_files;
    std::vector<std::string> entry_points;
  };

  std::string package_last_error;

  const std::vector<std::filesystem::path>& directory_entrypoint_candidates()
  {
    static const std::vector<std::filesystem::path> candidates{
      "core.lisple",
      "src/core.lisple",
      "main.lisple",
      "src/main.lisple",
      "game.lisple",
      "src/game.lisple",
    };
    return candidates;
  }

  std::optional<std::filesystem::path> find_directory_entrypoint(
    const std::filesystem::path& directory)
  {
    for (const auto& candidate : directory_entrypoint_candidates())
    {
      auto path = directory / candidate;
      if (std::filesystem::exists(path)) return std::filesystem::canonical(path);
    }
    return std::nullopt;
  }

  LaunchTarget script_launch_target(const std::filesystem::path& script_path)
  {
    auto canonical_path = std::filesystem::canonical(script_path);
    auto script_dir = canonical_path.parent_path();
    return LaunchTarget{
      .asset_base_path = script_dir,
      .load_path = {script_dir.string()},
      .namespace_roots = {},
      .source_files = {canonical_path.filename().string()},
      .entry_points = {},
    };
  }

  LaunchTarget package_launch_target(const std::filesystem::path& package_root)
  {
    Lisple::DirRootFileSystem manifest_fs("/");
    auto manifest =
      Lisple::Package::read_manifest(manifest_fs, (package_root / "package.edn").string());
    auto plan = Lisple::Package::resolve_load_plan(manifest_fs, package_root.string());
    auto asset_base_path = package_root;
    if (!manifest.load_roots.empty())
    {
      asset_base_path = package_root / manifest.load_roots.front();
    }

    LaunchTarget target{
      .asset_base_path = asset_base_path,
      .load_path =
        Lisple::Package::merge_load_paths(plan,
                                          {std::filesystem::current_path().string(), "/"}),
      .namespace_roots = plan.namespace_roots,
      .source_files = {},
      .entry_points = manifest.entry_points,
    };

    if (target.entry_points.empty())
    {
      auto script_path = find_directory_entrypoint(package_root);
      if (script_path.has_value())
      {
        auto script_dir = script_path->parent_path();
        target.load_path.insert(target.load_path.begin(), script_dir.string());
        target.source_files = {script_path->filename().string()};
      }
    }

    return target;
  }

  std::optional<LaunchTarget> resolve_launch_target(std::filesystem::path path)
  {
    if (!std::filesystem::exists(path)) return std::nullopt;

    path = std::filesystem::canonical(path);

    if (!std::filesystem::is_directory(path)) return script_launch_target(path);

    if (std::filesystem::exists(path / "package.edn"))
    {
      auto target = package_launch_target(path);
      if (!target.entry_points.empty() || !target.source_files.empty()) return target;
      return std::nullopt;
    }

    auto script_path = find_directory_entrypoint(path);
    if (!script_path.has_value()) return std::nullopt;

    return script_launch_target(*script_path);
  }

  std::string required_context_string(const Lisple::sptr_val& context,
                                      const std::string& key)
  {
    auto value = Lisple::Dict::get_property(context, Lisple::keyword(key));
    if (!value || value->type == Lisple::Value::Type::NIL)
    {
      throw Lisple::InvocationException("pixils.runner/run missing context key :" + key);
    }
    if (value->type != Lisple::Value::Type::STRING)
    {
      throw Lisple::TypeError("pixils.runner/run context key :" + key + " must be a string");
    }
    return value->str();
  }

  void run_target(const LaunchTarget& target)
  {
    auto opt_ctx = Pixils::init_sdl("Pixils");
    if (!opt_ctx.has_value())
    {
      SDL_Quit();
      throw Lisple::InvocationException("Failed to initialize SDL.");
    }

    try
    {
      Pixils::RenderContext ctx = std::move(*opt_ctx);

      Lisple::Runtime runtime = Pixils::init_lisple_runtime(
        ctx,
        "main",
        [&target](Pixils::RuntimeConfiguration* cfg)
        {
          cfg->load_path = target.load_path;
          cfg->namespace_roots = target.namespace_roots;
          cfg->asset_base_path = target.asset_base_path.string();
        },
        target.source_files);

      for (const auto& entry_point : target.entry_points)
      {
        runtime.eval("(ns pixils.package-entry (:require " + entry_point + "))",
                     "<package-entry>");
      }

      Pixils::Client client(runtime, ctx);
      client.run();
    }
    catch (...)
    {
      SDL_Quit();
      throw;
    }

    SDL_Quit();
  }

  int load_native_package(const LispleNativeHostV1* host);
  void unload_native_package();
  const char* last_error();

  namespace Function
  {
    FUNC(RunBangFunction, run);

    FUNC_IMPL(RunBangFunction,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&RunBangFunction::exec_run))));

    EXEC_BODY(RunBangFunction, exec_run)
    {
      auto package_root = required_context_string(args[0], "package-root");
      auto target = resolve_launch_target(package_root);
      if (!target.has_value())
      {
        throw Lisple::InvocationException("No Pixils launch target found for package '" +
                                          package_root + "'.");
      }

      run_target(*target);
      return Lisple::Constant::NIL;
    }
  } // namespace Function

  class RunnerNativeNamespace : public Lisple::Namespace
  {
   public:
    RunnerNativeNamespace()
      : Lisple::Namespace("pixils.runner.native")
    {
      values.emplace("run!", Function::RunBangFunction::make());
    }
  };

  int load_native_package(const LispleNativeHostV1* host)
  {
    try
    {
      auto ns = std::make_unique<RunnerNativeNamespace>();
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
    "pixils-runner-native",
    "0.1.0",
    LISPLE_NATIVE_CXX_ABI,
    load_native_package,
    unload_native_package,
    last_error,
  };
  return &package;
}
