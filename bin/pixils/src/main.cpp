
#include <pixils/client.h>
#include <pixils/context.h>
#include <pixils/init_sdl.h>
#include <pixils/script.h>

#include <SDL3/SDL.h>
#include <roo-package/manifest.h>
#include <roo-package/native_loader.h>
#include <roo/form.h>
#include <roo/io/dir_root_file_system.h>
#include <roo/reader.h>

#include <algorithm>
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
    std::optional<Roo::Package::LoadPlan> package_plan;
  };

  Roo::Package::LoadPlan pixils_host_load_plan(Roo::Package::LoadPlan plan)
  {
    std::erase_if(plan.native_libraries,
                  [](const Roo::Package::NativeLibrary& library)
                  {
                    return library.name == "pixils-native" ||
                           library.name == "pixils-runner-native";
                  });
    return plan;
  }

  std::optional<std::string> configured_asset_base(
    const Roo::Package::Manifest& manifest)
  {
    auto config_it = manifest.config.find("pixils");
    if (config_it == manifest.config.end()) return std::nullopt;

    Roo::Reader reader;
    auto forms = reader.read_sexps(config_it->second);
    if (forms.size() != 1 || forms[0]->get_type() != Roo::Form::MAP)
    {
      throw Roo::RooException("Invalid pixils package config: expected map, got " +
                              config_it->second);
    }

    auto& children = forms[0]->get_children();
    if (children.size() % 2 != 0)
    {
      throw Roo::RooException("Invalid pixils package config: uneven map form.");
    }

    for (size_t i = 0; i < children.size(); i += 2)
    {
      if (children[i]->get_type() == Roo::Form::KEYWORD &&
          children[i]->as<Roo::AST::Keyword>().get_identifier() == "asset-base")
      {
        if (children[i + 1]->get_type() != Roo::Form::STRING)
        {
          throw Roo::RooException(
            "Invalid pixils package config: :asset-base must be a string.");
        }
        return children[i + 1]->as<Roo::AST::String>().value;
      }
    }

    return std::nullopt;
  }

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
      .package_plan = std::nullopt,
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
    if (auto asset_base = configured_asset_base(manifest))
    {
      asset_base_path = std::filesystem::canonical(package_root / *asset_base);
    }
    else if (!manifest.load_roots.empty())
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
      .package_plan = pixils_host_load_plan(plan),
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

    Roo::Package::LoadedNativePackages native_packages;
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
                                  {});

    if (target->package_plan.has_value())
    {
      native_packages =
        Roo::Package::load_native_libraries(runtime, *target->package_plan);
      Roo::Package::load_autoloads(runtime, *target->package_plan);
    }

    for (const auto& source_file : target->source_files)
    {
      runtime.read_file(source_file);
    }

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
