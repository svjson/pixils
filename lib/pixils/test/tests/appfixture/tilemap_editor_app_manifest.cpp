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

    manifest.upsert_unit(
      load_unit(std::string(unit_ids::program), "apps/tilemap-editor/program.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::main_mode),
                                   "apps/tilemap-editor/modes/main-mode/main-mode.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::workspace_component),
                                   "apps/tilemap-editor/components/workspace/"
                                   "workspace.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::canvas_component),
                                   "apps/tilemap-editor/components/canvas/canvas.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::controls_component),
                                   "apps/tilemap-editor/components/controls/"
                                   "controls.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::palette_component),
                                   "apps/tilemap-editor/components/palette/"
                                   "palette.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::inspector_component),
                                   "apps/tilemap-editor/components/inspector/"
                                   "inspector.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::data), "apps/tilemap-editor/data/data.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::tilemap),
                                   "apps/tilemap-editor/tilemap/tilemap.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::tile_renderer),
                                   "apps/tilemap-editor/rendering/tile-renderer.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::theme), "apps/tilemap-editor/themes/theme.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::assets_bundle),
                                   "apps/tilemap-editor/bundles/assets/assets.roo"));

    manifest.add_file(ManifestFile{.id = std::string(file_ids::core),
                                   .disk_path = "tilemap-editor/main.roo",
                                   .namespace_name = main_namespace(),
                                   .unit_ids = {std::string(unit_ids::program)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/root"s,
                                   .disk_path = "tilemap-editor/root.roo",
                                   .namespace_name = "tilemap-editor.root"s,
                                   .unit_ids = {std::string(unit_ids::main_mode)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::data),
                                   .disk_path = "tilemap-editor/data.roo",
                                   .namespace_name = "tilemap-editor.data"s,
                                   .unit_ids = {std::string(unit_ids::data)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::tilemap),
                                   .disk_path = "tilemap-editor/tilemap.roo",
                                   .namespace_name = "tilemap-editor.tilemap"s,
                                   .unit_ids = {std::string(unit_ids::tilemap)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::rendering),
                                   .disk_path = "tilemap-editor/tile-renderer.roo",
                                   .namespace_name = "tilemap-editor.tile-renderer"s,
                                   .unit_ids = {std::string(unit_ids::tile_renderer)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/canvas"s,
                                   .disk_path = "tilemap-editor/canvas.roo",
                                   .namespace_name = "tilemap-editor.canvas"s,
                                   .unit_ids = {std::string(unit_ids::canvas_component)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/controls"s,
                                   .disk_path = "tilemap-editor/controls.roo",
                                   .namespace_name = "tilemap-editor.controls"s,
                                   .unit_ids = {std::string(unit_ids::controls_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/inspector"s,
                   .disk_path = "tilemap-editor/inspector.roo",
                   .namespace_name = "tilemap-editor.inspector"s,
                   .unit_ids = {std::string(unit_ids::inspector_component)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/palette"s,
                                   .disk_path = "tilemap-editor/palette.roo",
                                   .namespace_name = "tilemap-editor.palette"s,
                                   .unit_ids = {std::string(unit_ids::palette_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/workspace"s,
                   .disk_path = "tilemap-editor/workspace.roo",
                   .namespace_name = "tilemap-editor.workspace"s,
                   .unit_ids = {std::string(unit_ids::workspace_component)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::theme),
                                   .disk_path = "tilemap-editor/theme.roo",
                                   .namespace_name = "tilemap-editor.theme"s,
                                   .unit_ids = {std::string(unit_ids::theme)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::assets),
                                   .disk_path = "tilemap-editor/assets.roo",
                                   .namespace_name = "tilemap-editor.assets"s,
                                   .unit_ids = {std::string(unit_ids::assets_bundle)}});

    return manifest;
  }

  AppManifest current_manifest()
  {
    AppManifest manifest;

    manifest.upsert_unit(load_unit(std::string(unit_ids::program),
                                   "apps/tilemap-editor/current/main.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::main_mode),
                                   "apps/tilemap-editor/current/root.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::workspace_component),
                                   "apps/tilemap-editor/current/workspace.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::canvas_component),
                                   "apps/tilemap-editor/current/canvas.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::controls_component),
                                   "apps/tilemap-editor/current/controls.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::palette_component),
                                   "apps/tilemap-editor/current/palette.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::inspector_component),
                                   "apps/tilemap-editor/current/inspector.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::data),
                                   "apps/tilemap-editor/current/data.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::tilemap),
                                   "apps/tilemap-editor/current/tilemap.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::tile_renderer),
                                   "apps/tilemap-editor/current/tile-renderer.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::theme),
                                   "apps/tilemap-editor/current/theme.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::assets_bundle),
                                   "apps/tilemap-editor/current/assets.roo"));

    manifest.add_file(ManifestFile{.id = std::string(file_ids::core),
                                   .disk_path = "tilemap-editor/main.roo",
                                   .namespace_name = main_namespace(),
                                   .unit_ids = {std::string(unit_ids::program)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/root"s,
                                   .disk_path = "tilemap-editor/root.roo",
                                   .namespace_name = "tilemap-editor.root"s,
                                   .unit_ids = {std::string(unit_ids::main_mode)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::data),
                                   .disk_path = "tilemap-editor/data.roo",
                                   .namespace_name = "tilemap-editor.data"s,
                                   .unit_ids = {std::string(unit_ids::data)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::tilemap),
                                   .disk_path = "tilemap-editor/tilemap.roo",
                                   .namespace_name = "tilemap-editor.tilemap"s,
                                   .unit_ids = {std::string(unit_ids::tilemap)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::rendering),
                                   .disk_path = "tilemap-editor/tile-renderer.roo",
                                   .namespace_name = "tilemap-editor.tile-renderer"s,
                                   .unit_ids = {std::string(unit_ids::tile_renderer)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/canvas"s,
                                   .disk_path = "tilemap-editor/canvas.roo",
                                   .namespace_name = "tilemap-editor.canvas"s,
                                   .unit_ids = {std::string(unit_ids::canvas_component)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/controls"s,
                                   .disk_path = "tilemap-editor/controls.roo",
                                   .namespace_name = "tilemap-editor.controls"s,
                                   .unit_ids = {std::string(unit_ids::controls_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/inspector"s,
                   .disk_path = "tilemap-editor/inspector.roo",
                   .namespace_name = "tilemap-editor.inspector"s,
                   .unit_ids = {std::string(unit_ids::inspector_component)}});

    manifest.add_file(ManifestFile{.id = "tilemap-editor/palette"s,
                                   .disk_path = "tilemap-editor/palette.roo",
                                   .namespace_name = "tilemap-editor.palette"s,
                                   .unit_ids = {std::string(unit_ids::palette_component)}});

    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/workspace"s,
                   .disk_path = "tilemap-editor/workspace.roo",
                   .namespace_name = "tilemap-editor.workspace"s,
                   .unit_ids = {std::string(unit_ids::workspace_component)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::theme),
                                   .disk_path = "tilemap-editor/theme.roo",
                                   .namespace_name = "tilemap-editor.theme"s,
                                   .unit_ids = {std::string(unit_ids::theme)}});

    manifest.add_file(ManifestFile{.id = std::string(file_ids::assets),
                                   .disk_path = "tilemap-editor/assets.roo",
                                   .namespace_name = "tilemap-editor.assets"s,
                                   .unit_ids = {std::string(unit_ids::assets_bundle)}});

    return manifest;
  }

  AppManifest benchmark_808e9fd_manifest()
  {
    AppManifest manifest;

    auto add_unit_file = [&](const std::string& unit_id,
                             const std::filesystem::path& source_path,
                             const std::string& file_id,
                             const std::filesystem::path& disk_path,
                             const std::string& namespace_name)
    {
      manifest.upsert_unit(load_unit(unit_id,
                                     std::filesystem::path(
                                       "apps/tilemap-editor/benchmark-808e9fd/src") /
                                       source_path));
      manifest.add_file(ManifestFile{.id = file_id,
                                     .disk_path = disk_path,
                                     .namespace_name = namespace_name,
                                     .unit_ids = {unit_id}});
    };

    add_unit_file("tilemap-editor/benchmark-808e9fd/core",
                  "core.roo",
                  "tilemap-editor/benchmark-808e9fd/core",
                  "tilemap-editor/main.roo",
                  main_namespace());
    add_unit_file("tilemap-editor/benchmark-808e9fd/assets",
                  "assets.roo",
                  "tilemap-editor/benchmark-808e9fd/assets",
                  "tilemap-editor/assets.roo",
                  "tilemap-editor.assets");
    add_unit_file("tilemap-editor/benchmark-808e9fd/root",
                  "root.roo",
                  "tilemap-editor/benchmark-808e9fd/root",
                  "tilemap-editor/root.roo",
                  "tilemap-editor.root");
    add_unit_file("tilemap-editor/benchmark-808e9fd/io/project",
                  "io/project.roo",
                  "tilemap-editor/benchmark-808e9fd/io/project",
                  "tilemap-editor/io/project.roo",
                  "tilemap-editor.io.project");
    add_unit_file("tilemap-editor/benchmark-808e9fd/model/data",
                  "model/data.roo",
                  "tilemap-editor/benchmark-808e9fd/model/data",
                  "tilemap-editor/model/data.roo",
                  "tilemap-editor.model.data");
    add_unit_file("tilemap-editor/benchmark-808e9fd/model/tilemap",
                  "model/tilemap.roo",
                  "tilemap-editor/benchmark-808e9fd/model/tilemap",
                  "tilemap-editor/model/tilemap.roo",
                  "tilemap-editor.model.tilemap");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/theme",
                  "view/theme.roo",
                  "tilemap-editor/benchmark-808e9fd/view/theme",
                  "tilemap-editor/view/theme.roo",
                  "tilemap-editor.view.theme");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/resources/model",
                  "view/resources/model.roo",
                  "tilemap-editor/benchmark-808e9fd/view/resources/model",
                  "tilemap-editor/view/resources/model.roo",
                  "tilemap-editor.view.resources.model");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/resources/dialogs",
                  "view/resources/dialogs.roo",
                  "tilemap-editor/benchmark-808e9fd/view/resources/dialogs",
                  "tilemap-editor/view/resources/dialogs.roo",
                  "tilemap-editor.view.resources.dialogs");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/resources/panels",
                  "view/resources/panels.roo",
                  "tilemap-editor/benchmark-808e9fd/view/resources/panels",
                  "tilemap-editor/view/resources/panels.roo",
                  "tilemap-editor.view.resources.panels");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/resources/layout",
                  "view/resources/layout.roo",
                  "tilemap-editor/benchmark-808e9fd/view/resources/layout",
                  "tilemap-editor/view/resources/layout.roo",
                  "tilemap-editor.view.resources.layout");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tilemap/renderer",
                  "view/tilemap/renderer.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tilemap/renderer",
                  "tilemap-editor/view/tilemap/renderer.roo",
                  "tilemap-editor.view.tilemap.renderer");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tilemap/canvas",
                  "view/tilemap/canvas.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tilemap/canvas",
                  "tilemap-editor/view/tilemap/canvas.roo",
                  "tilemap-editor.view.tilemap.canvas");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tilemap/inspector",
                  "view/tilemap/inspector.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tilemap/inspector",
                  "tilemap-editor/view/tilemap/inspector.roo",
                  "tilemap-editor.view.tilemap.inspector");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tilemap/palette",
                  "view/tilemap/palette.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tilemap/palette",
                  "tilemap-editor/view/tilemap/palette.roo",
                  "tilemap-editor.view.tilemap.palette");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tilemap/controls",
                  "view/tilemap/controls.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tilemap/controls",
                  "tilemap-editor/view/tilemap/controls.roo",
                  "tilemap-editor.view.tilemap.controls");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tilemap/layout",
                  "view/tilemap/layout.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tilemap/layout",
                  "tilemap-editor/view/tilemap/layout.roo",
                  "tilemap-editor.view.tilemap.layout");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tileset/model",
                  "view/tileset/model.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tileset/model",
                  "tilemap-editor/view/tileset/model.roo",
                  "tilemap-editor.view.tileset.model");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tileset/panels",
                  "view/tileset/panels.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tileset/panels",
                  "tilemap-editor/view/tileset/panels.roo",
                  "tilemap-editor.view.tileset.panels");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tileset/dialogs",
                  "view/tileset/dialogs.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tileset/dialogs",
                  "tilemap-editor/view/tileset/dialogs.roo",
                  "tilemap-editor.view.tileset.dialogs");
    add_unit_file("tilemap-editor/benchmark-808e9fd/view/tileset/layout",
                  "view/tileset/layout.roo",
                  "tilemap-editor/benchmark-808e9fd/view/tileset/layout",
                  "tilemap-editor/view/tileset/layout.roo",
                  "tilemap-editor.view.tileset.layout");

    return manifest;
  }

  static AppManifest build_layered_manifest(std::string_view layer_list_unit_id)
  {
    auto manifest = Pixils::Test::AppFixture::TilemapEditor::manifest();

    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_program),
                                   "apps/tilemap-editor/programs/"
                                   "layered-program.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_main_mode),
                                   "apps/tilemap-editor/modes/main-mode/"
                                   "layered-main-mode.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_workspace_component),
                                   "apps/tilemap-editor/components/workspace/"
                                   "layered-workspace.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_canvas_component),
                                   "apps/tilemap-editor/components/canvas/"
                                   "layered-canvas.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_controls_shared),
                                   "apps/tilemap-editor/components/controls/layered/"
                                   "shared-layer-controls.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_toolbar_component),
                                   "apps/tilemap-editor/components/controls/layered/"
                                   "toolbar.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::layered_tileset_controls_component),
                "apps/tilemap-editor/components/controls/layered/tileset-controls.roo"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::layered_layer_visibility_toggle_component),
      "apps/tilemap-editor/components/controls/layered/layer-visibility-toggle.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_layer_row_component),
                                   "apps/tilemap-editor/components/controls/layered/"
                                   "layer-row.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_layer_list_plain_component),
                                   "apps/tilemap-editor/components/controls/layered/"
                                   "layer-list-plain.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::layered_layer_list_content_component),
                "apps/tilemap-editor/components/controls/layered/"
                "layer-list-content.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::layered_layer_list_plain_overflow_component),
                "apps/tilemap-editor/components/controls/layered/"
                "layer-list-plain-overflow.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::layered_layer_list_content_overflow_component),
                "apps/tilemap-editor/components/controls/layered/"
                "layer-list-content-overflow.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::layered_layer_list_unclipped_component),
                "apps/tilemap-editor/components/controls/layered/"
                "layer-list-unclipped.roo"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::layered_layer_list_clipped_component),
                "apps/tilemap-editor/components/controls/layered/"
                "layer-list-clipped.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_layer_controls_component),
                                   "apps/tilemap-editor/components/controls/layered/"
                                   "layer-controls.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_zoom_control_component),
                                   "apps/tilemap-editor/components/controls/layered/"
                                   "zoom-control.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_inspector_component),
                                   "apps/tilemap-editor/components/inspector/"
                                   "layered-inspector.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_data),
                                   "apps/tilemap-editor/data/layered-data.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_tilemap),
                                   "apps/tilemap-editor/tilemap/layered-tilemap.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_tile_renderer),
                                   "apps/tilemap-editor/rendering/"
                                   "layered-tile-renderer.roo"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::layered_theme),
                                   "apps/tilemap-editor/themes/layered-theme.roo"));

    manifest.remove_file(std::string(file_ids::core));
    manifest.add_file(ManifestFile{.id = std::string(file_ids::core),
                                   .disk_path = "tilemap-editor/main.roo",
                                   .namespace_name = main_namespace(),
                                   .unit_ids = {std::string(unit_ids::layered_program)}});

    manifest.remove_file("tilemap-editor/root");
    manifest.add_file(ManifestFile{.id = "tilemap-editor/root"s,
                                   .disk_path = "tilemap-editor/root.roo",
                                   .namespace_name = "tilemap-editor.root"s,
                                   .unit_ids = {std::string(unit_ids::layered_main_mode)}});

    manifest.remove_file(std::string(file_ids::data));
    manifest.add_file(ManifestFile{.id = std::string(file_ids::data),
                                   .disk_path = "tilemap-editor/data.roo",
                                   .namespace_name = "tilemap-editor.data"s,
                                   .unit_ids = {std::string(unit_ids::layered_data)}});

    manifest.remove_file(std::string(file_ids::tilemap));
    manifest.add_file(ManifestFile{.id = std::string(file_ids::tilemap),
                                   .disk_path = "tilemap-editor/tilemap.roo",
                                   .namespace_name = "tilemap-editor.tilemap"s,
                                   .unit_ids = {std::string(unit_ids::layered_tilemap)}});

    manifest.remove_file(std::string(file_ids::rendering));
    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::rendering),
                   .disk_path = "tilemap-editor/tile-renderer.roo",
                   .namespace_name = "tilemap-editor.tile-renderer"s,
                   .unit_ids = {std::string(unit_ids::layered_tile_renderer)}});

    manifest.remove_file("tilemap-editor/canvas");
    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/canvas"s,
                   .disk_path = "tilemap-editor/canvas.roo",
                   .namespace_name = "tilemap-editor.canvas"s,
                   .unit_ids = {std::string(unit_ids::layered_canvas_component)}});

    manifest.remove_file("tilemap-editor/controls");
    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/controls"s,
                   .disk_path = "tilemap-editor/controls.roo",
                   .namespace_name = "tilemap-editor.controls"s,
                   .unit_ids = {
                     std::string(unit_ids::layered_controls_shared),
                     std::string(unit_ids::layered_toolbar_component),
                     std::string(unit_ids::layered_tileset_controls_component),
                     std::string(unit_ids::layered_layer_visibility_toggle_component),
                     std::string(unit_ids::layered_layer_row_component),
                     std::string(layer_list_unit_id),
                     std::string(unit_ids::layered_layer_controls_component),
                     std::string(unit_ids::layered_zoom_control_component),
                   }});

    manifest.remove_file("tilemap-editor/inspector");
    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/inspector"s,
                   .disk_path = "tilemap-editor/inspector.roo",
                   .namespace_name = "tilemap-editor.inspector"s,
                   .unit_ids = {std::string(unit_ids::layered_inspector_component)}});

    manifest.remove_file("tilemap-editor/workspace");
    manifest.add_file(
      ManifestFile{.id = "tilemap-editor/workspace"s,
                   .disk_path = "tilemap-editor/workspace.roo",
                   .namespace_name = "tilemap-editor.workspace"s,
                   .unit_ids = {std::string(unit_ids::layered_workspace_component)}});

    manifest.remove_file(std::string(file_ids::theme));
    manifest.add_file(ManifestFile{.id = std::string(file_ids::theme),
                                   .disk_path = "tilemap-editor/theme.roo",
                                   .namespace_name = "tilemap-editor.theme"s,
                                   .unit_ids = {std::string(unit_ids::layered_theme)}});

    return manifest;
  }

  AppManifest layered_manifest()
  {
    return build_layered_manifest(unit_ids::layered_layer_list_content_component);
  }

  AppManifest layered_plain_layer_list_manifest()
  {
    return build_layered_manifest(unit_ids::layered_layer_list_plain_component);
  }

  AppManifest layered_content_layer_list_manifest()
  {
    return build_layered_manifest(unit_ids::layered_layer_list_content_component);
  }

  AppManifest layered_plain_overflow_layer_list_manifest()
  {
    return build_layered_manifest(unit_ids::layered_layer_list_plain_overflow_component);
  }

  AppManifest layered_content_overflow_layer_list_manifest()
  {
    return build_layered_manifest(unit_ids::layered_layer_list_content_overflow_component);
  }

  AppManifest layered_clipped_layer_list_manifest()
  {
    return build_layered_manifest(unit_ids::layered_layer_list_clipped_component);
  }

  std::string main_namespace()
  {
    return "tilemap-editor.core";
  }

  std::vector<std::string> entry_files()
  {
    return {"tilemap-editor/main.roo"};
  }

  std::vector<std::string> layered_entry_files()
  {
    return entry_files();
  }

  std::vector<std::string> benchmark_808e9fd_entry_files()
  {
    return entry_files();
  }

  std::filesystem::path spritesheet_asset_path()
  {
    return assets_dir() / "apps" / "tilemap-editor" / "assets" / "simples_pimples.png";
  }

  std::filesystem::path benchmark_808e9fd_spritesheet_asset_path()
  {
    return assets_dir() / "apps" / "tilemap-editor" / "benchmark-808e9fd" /
           "assets" / "simples_pimples.png";
  }

  std::filesystem::path benchmark_808e9fd_example_map_path()
  {
    return assets_dir() / "apps" / "tilemap-editor" / "benchmark-808e9fd" /
           "example-maps" / "map1.edn";
  }
} // namespace Pixils::Test::AppFixture::TilemapEditor
