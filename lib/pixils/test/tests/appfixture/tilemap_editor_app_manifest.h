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
  } // namespace unit_ids

  AppManifest manifest();
  std::string main_namespace();
  std::vector<std::string> entry_files();
  std::filesystem::path spritesheet_asset_path();
} // namespace Pixils::Test::AppFixture::TilemapEditor

#endif
