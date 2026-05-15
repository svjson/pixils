#include "tilemap_editor_app_manifest.h"

#include <filesystem>

namespace Pixils::Test::AppFixture::TilemapEditor
{
  namespace
  {
    using namespace std::string_literals;

    std::filesystem::path appfixture_dir()
    {
      return std::filesystem::path(__FILE__).parent_path();
    }

    std::filesystem::path assets_dir()
    {
      return appfixture_dir() / "assets";
    }

    SourceUnit load_unit(const std::string& unit_id,
                         const std::filesystem::path& relative_path)
    {
      return SourceUnit::from_file(unit_id, assets_dir() / relative_path);
    }
  } // namespace

  AppManifest manifest()
  {
    AppManifest manifest;

    manifest.upsert_unit(load_unit(std::string(unit_ids::program),
                                   "apps/tilemap-editor/program.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::main_mode),
                                   "apps/tilemap-editor/modes/main-mode/main-mode.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::workspace_component),
                                   "apps/tilemap-editor/components/workspace/"
                                   "workspace.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::canvas_component),
                                   "apps/tilemap-editor/components/canvas/canvas.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::controls_component),
                                   "apps/tilemap-editor/components/controls/"
                                   "controls.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::palette_component),
                                   "apps/tilemap-editor/components/palette/"
                                   "palette.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::inspector_component),
                                   "apps/tilemap-editor/components/inspector/"
                                   "inspector.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::data),
                                   "apps/tilemap-editor/data/data.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::tilemap),
                                   "apps/tilemap-editor/tilemap/tilemap.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::tile_renderer),
                                   "apps/tilemap-editor/rendering/tile-renderer.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::theme),
                                   "apps/tilemap-editor/themes/theme.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::assets_bundle),
                                   "apps/tilemap-editor/bundles/assets/assets.lisple"));

    manifest.add_file(ManifestFile{.id = std::string(file_ids::core),
                                   .disk_path = "tilemap-editor/main.lisple",
                                   .namespace_name = main_namespace(),
                                   .unit_ids = {std::string(unit_ids::program)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/root"s,
                                   .disk_path = "tilemap-editor/root.lisple",
                                   .namespace_name = "tilemap-editor.root"s,
                                   .unit_ids = {std::string(unit_ids::main_mode)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::data),
                   .disk_path = "tilemap-editor/data.lisple",
                   .namespace_name = "tilemap-editor.data"s,
                   .unit_ids = {std::string(unit_ids::data)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::tilemap),
                   .disk_path = "tilemap-editor/tilemap.lisple",
                   .namespace_name = "tilemap-editor.tilemap"s,
                   .unit_ids = {std::string(unit_ids::tilemap)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::rendering),
                   .disk_path = "tilemap-editor/tile-renderer.lisple",
                   .namespace_name = "tilemap-editor.tile-renderer"s,
                   .unit_ids = {std::string(unit_ids::tile_renderer)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/canvas"s,
                   .disk_path = "tilemap-editor/canvas.lisple",
                   .namespace_name = "tilemap-editor.canvas"s,
                   .unit_ids = {std::string(unit_ids::canvas_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/controls"s,
                   .disk_path = "tilemap-editor/controls.lisple",
                   .namespace_name = "tilemap-editor.controls"s,
                   .unit_ids = {std::string(unit_ids::controls_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/inspector"s,
                   .disk_path = "tilemap-editor/inspector.lisple",
                   .namespace_name = "tilemap-editor.inspector"s,
                   .unit_ids = {std::string(unit_ids::inspector_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/palette"s,
                   .disk_path = "tilemap-editor/palette.lisple",
                   .namespace_name = "tilemap-editor.palette"s,
                   .unit_ids = {std::string(unit_ids::palette_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/workspace"s,
                   .disk_path = "tilemap-editor/workspace.lisple",
                   .namespace_name = "tilemap-editor.workspace"s,
                   .unit_ids = {std::string(unit_ids::workspace_component)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::theme),
                   .disk_path = "tilemap-editor/theme.lisple",
                   .namespace_name = "tilemap-editor.theme"s,
                   .unit_ids = {std::string(unit_ids::theme)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::assets),
                   .disk_path = "tilemap-editor/assets.lisple",
                   .namespace_name = "tilemap-editor.assets"s,
                   .unit_ids = {std::string(unit_ids::assets_bundle)}});

    return manifest;
  }

  std::string main_namespace()
  {
    return "tilemap-editor.core";
  }

  std::vector<std::string> entry_files()
  {
    return {"tilemap-editor/main.lisple"};
  }

  std::filesystem::path spritesheet_asset_path()
  {
    return assets_dir() / "apps" / "tilemap-editor" / "assets" /
           "simples_pimples.png";
  }
} // namespace Pixils::Test::AppFixture::TilemapEditor
