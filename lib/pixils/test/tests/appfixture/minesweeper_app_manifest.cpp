#include "minesweeper_app_manifest.h"

#include <filesystem>

namespace Pixils::Test::AppFixture::Minesweeper
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

    SourceUnit load_unit(const std::string& unit_id, const std::filesystem::path& relative_path)
    {
      return SourceUnit::from_file(unit_id, assets_dir() / relative_path);
    }
  } // namespace

  AppManifest default_manifest()
  {
    AppManifest manifest;

    manifest.upsert_unit(load_unit(
      std::string(unit_ids::program),
      "apps/minesweeper/program.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::window_mode),
      "apps/minesweeper/modes/window-mode.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::game_layout),
      "apps/minesweeper/components/game-layout.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::mine_layer),
      "apps/minesweeper/components/board/mine-layer.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::board_buttons),
      "apps/minesweeper/components/board/board-buttons.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::board_mode),
      "apps/minesweeper/components/board/board-mode.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::status_panel),
      "apps/minesweeper/components/status-panel/status-panel.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::game_rules),
      "apps/minesweeper/game-logic/game-rules.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::button_component),
      "shared/ui/components/button/button-component.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::menu_components),
      "shared/ui/components/menu/menu-components.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::win311_theme),
      "shared/ui/themes/win311/win-theme.lisple"));

    manifest.add_file(ManifestFile{
      .id = std::string(file_ids::core),
      .disk_path = "pixils/test/app/minesweeper/core.lisple",
      .namespace_name = main_namespace(),
      .unit_ids = {
        std::string(unit_ids::program),
        std::string(unit_ids::window_mode),
        std::string(unit_ids::game_layout),
        std::string(unit_ids::mine_layer),
        std::string(unit_ids::board_buttons),
        std::string(unit_ids::board_mode),
        std::string(unit_ids::status_panel),
      }});

    manifest.add_file(ManifestFile{
      .id = std::string(file_ids::game_logic),
      .disk_path = "pixils/test/app/minesweeper/game-logic.lisple",
      .namespace_name = "pixils.test.app.minesweeper.game-logic"s,
      .unit_ids = {std::string(unit_ids::game_rules)}});

    manifest.add_file(ManifestFile{
      .id = std::string(file_ids::shared_button),
      .disk_path = "pixils/test/app/shared/ui/components/button.lisple",
      .namespace_name = "pixils.test.app.shared.ui.components.button"s,
      .unit_ids = {std::string(unit_ids::button_component)}});

    manifest.add_file(ManifestFile{
      .id = std::string(file_ids::shared_menu),
      .disk_path = "pixils/test/app/shared/ui/components/menu.lisple",
      .namespace_name = "pixils.test.app.shared.ui.components.menu"s,
      .unit_ids = {std::string(unit_ids::menu_components)}});

    manifest.add_file(ManifestFile{
      .id = std::string(file_ids::shared_win311),
      .disk_path = "pixils/test/app/shared/ui/themes/win311.lisple",
      .namespace_name = "pixils.test.app.shared.ui.themes.win311"s,
      .unit_ids = {std::string(unit_ids::win311_theme)}});

    return manifest;
  }

  std::string main_namespace()
  {
    return "pixils.test.app.minesweeper.core";
  }

  std::vector<std::string> entry_files()
  {
    return {
      "pixils/test/app/shared/ui/themes/win311.lisple",
      "pixils/test/app/shared/ui/components/button.lisple",
      "pixils/test/app/shared/ui/components/menu.lisple",
      "pixils/test/app/minesweeper/game-logic.lisple",
      "pixils/test/app/minesweeper/core.lisple",
    };
  }
} // namespace Pixils::Test::AppFixture::Minesweeper
