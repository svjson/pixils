
#include <pixils/client.h>
#include <pixils/context.h>
#include <pixils/init_sdl.h>
#include <pixils/script.h>

#include <SDL2/SDL.h>
#include <roo-package/manifest.h>
#include <roo/io/dir_root_file_system.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace
{
  struct LaunchTarget
  {
    std::filesystem::path asset_base_path;
    std::vector<std::string> load_path;
    std::vector<Roo::NamespaceRoot> namespace_roots;
    std::vector<std::string> source_files;
    std::vector<std::string> entry_points;
  };

  const std::vector<std::filesystem::path>& directory_entrypoint_candidates()
  {
    static const std::vector<std::filesystem::path> candidates{
      "core.roo",
      "src/core.roo",
      "main.roo",
      "src/main.roo",
      "game.roo",
      "src/game.roo",
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
    Roo::DirRootFileSystem manifest_fs("/");
    auto manifest = Roo::Package::read_manifest(
      manifest_fs,
      (package_root / "package.edn").string());
    auto plan = Roo::Package::resolve_load_plan(manifest_fs, package_root.string());
    auto asset_base_path = package_root;
    if (!manifest.load_roots.empty())
    {
      asset_base_path = package_root / manifest.load_roots.front();
    }

    LaunchTarget target{
      .asset_base_path = asset_base_path,
      .load_path = Roo::Package::merge_load_paths(
        plan,
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
} // namespace

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: pixils <script.roo|directory>" << std::endl;
    return 1;
  }

  std::optional<LaunchTarget> target;
  try
  {
    target = resolve_launch_target(argv[1]);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  if (!target.has_value())
  {
    std::cerr << "Not found: " << argv[1] << std::endl;
    return 1;
  }

  auto opt_ctx = Pixils::init_sdl("Pixils");
  if (!opt_ctx.has_value())
  {
    std::cerr << "Failed to initialize SDL." << std::endl;
    SDL_Quit();
    return 1;
  }

  try
  {
    Pixils::RenderContext ctx = std::move(*opt_ctx);

    Roo::Runtime runtime =
      Pixils::init_roo_runtime(ctx,
                                  "main",
                                  [&target](Pixils::RuntimeConfiguration* cfg)
                                  {
                                    cfg->load_path = target->load_path;
                                    cfg->namespace_roots = target->namespace_roots;
                                    cfg->asset_base_path =
                                      target->asset_base_path.string();
                                  },
                                  target->source_files);

    for (const auto& entry_point : target->entry_points)
    {
      runtime.eval("(ns pixils.package-entry (:require " + entry_point + "))",
                   "<package-entry>");
    }

    Pixils::Client client(runtime, ctx);
    client.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Quit();
  return 0;
}
