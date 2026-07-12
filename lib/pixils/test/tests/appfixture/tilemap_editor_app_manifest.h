#ifndef PIXILS__TEST__APPFIXTURE__TILEMAP_EDITOR_APP_MANIFEST_H
#define PIXILS__TEST__APPFIXTURE__TILEMAP_EDITOR_APP_MANIFEST_H

#include "app_manifest.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Pixils::Test::AppFixture::TilemapEditor
{
  namespace file_ids
  {
    inline constexpr std::string_view core = "tilemap-editor/core";
    inline constexpr std::string_view data = "tilemap-editor/data";
    inline constexpr std::string_view tilemap = "tilemap-editor/tilemap";
    inline constexpr std::string_view rendering = "tilemap-editor/rendering";
    inline constexpr std::string_view theme = "tilemap-editor/theme";
    inline constexpr std::string_view assets = "tilemap-editor/assets";
  } // namespace file_ids

  namespace unit_ids
  {
    inline constexpr std::string_view program = "tilemap-editor/program";
    inline constexpr std::string_view main_mode = "tilemap-editor/main-mode";
    inline constexpr std::string_view workspace_component = "tilemap-editor/workspace";
    inline constexpr std::string_view canvas_component = "tilemap-editor/canvas";
    inline constexpr std::string_view controls_component = "tilemap-editor/controls";
    inline constexpr std::string_view palette_component = "tilemap-editor/palette";
    inline constexpr std::string_view inspector_component = "tilemap-editor/inspector";
    inline constexpr std::string_view data = "tilemap-editor/data";
    inline constexpr std::string_view tilemap = "tilemap-editor/tilemap";
    inline constexpr std::string_view tile_renderer = "tilemap-editor/tile-renderer";
    inline constexpr std::string_view theme = "tilemap-editor/theme";
    inline constexpr std::string_view assets_bundle = "tilemap-editor/bundles/assets";
    inline constexpr std::string_view layered_program = "tilemap-editor/layered/program";
    inline constexpr std::string_view layered_main_mode = "tilemap-editor/layered/main-mode";
    inline constexpr std::string_view layered_workspace_component =
      "tilemap-editor/layered/workspace";
    inline constexpr std::string_view layered_canvas_component =
      "tilemap-editor/layered/canvas";
    inline constexpr std::string_view layered_controls_shared =
      "tilemap-editor/layered/controls/shared";
    inline constexpr std::string_view layered_toolbar_component =
      "tilemap-editor/layered/controls/toolbar";
    inline constexpr std::string_view layered_tileset_controls_component =
      "tilemap-editor/layered/controls/tileset-controls";
    inline constexpr std::string_view layered_layer_visibility_toggle_component =
      "tilemap-editor/layered/controls/layer-visibility-toggle";
    inline constexpr std::string_view layered_layer_row_component =
      "tilemap-editor/layered/controls/layer-row";
    inline constexpr std::string_view layered_layer_list_plain_component =
      "tilemap-editor/layered/controls/layer-list-plain";
    inline constexpr std::string_view layered_layer_list_content_component =
      "tilemap-editor/layered/controls/layer-list-content";
    inline constexpr std::string_view layered_layer_list_plain_overflow_component =
      "tilemap-editor/layered/controls/layer-list-plain-overflow";
    inline constexpr std::string_view layered_layer_list_content_overflow_component =
      "tilemap-editor/layered/controls/layer-list-content-overflow";
    inline constexpr std::string_view layered_layer_list_unclipped_component =
      "tilemap-editor/layered/controls/layer-list-unclipped";
    inline constexpr std::string_view layered_layer_list_clipped_component =
      "tilemap-editor/layered/controls/layer-list-clipped";
    inline constexpr std::string_view layered_layer_controls_component =
      "tilemap-editor/layered/controls/layer-controls";
    inline constexpr std::string_view layered_zoom_control_component =
      "tilemap-editor/layered/controls/zoom-control";
    inline constexpr std::string_view layered_inspector_component =
      "tilemap-editor/layered/inspector";
    inline constexpr std::string_view layered_data = "tilemap-editor/layered/data";
    inline constexpr std::string_view layered_tilemap = "tilemap-editor/layered/tilemap";
    inline constexpr std::string_view layered_tile_renderer =
      "tilemap-editor/layered/tile-renderer";
    inline constexpr std::string_view layered_theme = "tilemap-editor/layered/theme";
  } // namespace unit_ids

  AppManifest manifest();
  AppManifest layered_manifest();
  AppManifest layered_plain_layer_list_manifest();
  AppManifest layered_content_layer_list_manifest();
  AppManifest layered_plain_overflow_layer_list_manifest();
  AppManifest layered_content_overflow_layer_list_manifest();
  AppManifest layered_clipped_layer_list_manifest();
  AppManifest benchmark_808e9fd_manifest();
  AppManifest current_manifest();
  std::string main_namespace();
  std::vector<std::string> entry_files();
  std::vector<std::string> layered_entry_files();
  std::vector<std::string> benchmark_808e9fd_entry_files();
  std::filesystem::path spritesheet_asset_path();
  std::filesystem::path benchmark_808e9fd_spritesheet_asset_path();
  std::filesystem::path benchmark_808e9fd_example_map_path();
} // namespace Pixils::Test::AppFixture::TilemapEditor

#endif
