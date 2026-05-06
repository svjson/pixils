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

    SourceUnit load_unit(const std::string& unit_id,
                         const std::filesystem::path& relative_path)
    {
      return SourceUnit::from_file(unit_id, assets_dir() / relative_path);
    }
  } // namespace

  AppManifest implicit_fill_manifest()
  {
    AppManifest manifest;

    manifest.upsert_unit(load_unit(std::string(unit_ids::program),
                                   "apps/minesweeper/implicit-fill-program.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::implicit_fill_window_mode),
                "apps/minesweeper/modes/window/implicit-fill-window-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::implicit_fill_game_layout),
                "apps/minesweeper/components/game-layout/implicit-fill-game-layout.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::mine_layer_render),
                "apps/minesweeper/components/board/mine-layer/mine-layer-render.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::render_only_mine_layer),
      "apps/minesweeper/components/board/mine-layer/render-only-mine-layer.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::inhibit_only_board_button_component),
      "apps/minesweeper/components/board/board-button/inhibit-only-board-button.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::board_button_row_component),
                                   "apps/minesweeper/components/board/board-button-row/"
                                   "board-button-row.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_buttons_component),
                "apps/minesweeper/components/board/board-buttons/board-buttons.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::overlay_button_board_mode),
                "apps/minesweeper/components/board/overlay-button-board-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_left_pad),
                "apps/minesweeper/components/status-panel/left-pad.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::fixed_size_rendered_counter_component),
                "apps/minesweeper/components/status-panel/counter/"
                "fixed-size-rendered-counter.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::fixed_size_counter_status_panel),
                                   "apps/minesweeper/components/status-panel/"
                                   "fixed-size-counter-status-panel.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::counter_font),
      "apps/minesweeper/components/status-panel/counter-font/counter-font.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_definition),
                "apps/minesweeper/components/menu/menu-definition.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::custom_rendered_menu_item),
                "shared/ui/components/menu/menu-item/custom-rendered-menu-item.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::fixed_width_menu_bar_mode),
                "shared/ui/components/menu/menu-bar/fixed-width-menu-bar-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::popup_menu_mode),
                "shared/ui/components/menu/popup-menu/popup-menu.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_face_new_game_state),
                "apps/minesweeper/game-logic/status-panel-face-new-game-state.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::game_rules),
                                   "apps/minesweeper/game-logic/game-rules.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::button_component),
                                   "shared/ui/components/button/button-component.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::win311_theme),
                                   "shared/ui/themes/win311/win-theme.lisple"));

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::core),
                   .disk_path = "pixils/test/app/minesweeper/core.lisple",
                   .namespace_name = main_namespace(),
                   .unit_ids = {
                     std::string(unit_ids::program),
                     std::string(unit_ids::implicit_fill_window_mode),
                     std::string(unit_ids::implicit_fill_game_layout),
                     std::string(unit_ids::mine_layer_render),
                     std::string(unit_ids::render_only_mine_layer),
                     std::string(unit_ids::inhibit_only_board_button_component),
                     std::string(unit_ids::board_button_row_component),
                     std::string(unit_ids::board_buttons_component),
                     std::string(unit_ids::overlay_button_board_mode),
                     std::string(unit_ids::status_panel_left_pad),
                     std::string(unit_ids::fixed_size_rendered_counter_component),
                     std::string(unit_ids::fixed_size_counter_status_panel),
                     std::string(unit_ids::counter_font),
                     std::string(unit_ids::custom_rendered_menu_item),
                     std::string(unit_ids::fixed_width_menu_bar_mode),
                     std::string(unit_ids::popup_menu_mode),
                   }});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::game_logic),
                   .disk_path = "pixils/test/app/minesweeper/game-logic.lisple",
                   .namespace_name = "pixils.test.app.minesweeper.game-logic"s,
                   .unit_ids = {
                     std::string(unit_ids::status_panel_face_new_game_state),
                     std::string(unit_ids::game_rules),
                   }});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::menu_definition),
                   .disk_path = "pixils/test/app/minesweeper/menu-definition.lisple",
                   .namespace_name = "pixils.test.app.minesweeper.menu-definition"s,
                   .unit_ids = {std::string(unit_ids::menu_definition)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_button),
                   .disk_path = "pixils/test/app/shared/ui/components/button.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.components.button"s,
                   .unit_ids = {std::string(unit_ids::button_component)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_win311),
                   .disk_path = "pixils/test/app/shared/ui/themes/win311.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.themes.win311"s,
                   .unit_ids = {std::string(unit_ids::win311_theme)}});

    return manifest;
  }

  AppManifest canonical_manifest()
  {
    AppManifest manifest;

    manifest.upsert_unit(
      load_unit(std::string(unit_ids::program), "apps/minesweeper/program.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::main_mode),
                "apps/minesweeper/modes/main-mode/main-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::window_mode),
                "apps/minesweeper/modes/window/window-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::game_mode),
                "apps/minesweeper/components/game-layout/game-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_bundle),
                "apps/minesweeper/bundles/status/status.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::counter_bundle),
                "apps/minesweeper/bundles/counter/counter.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_left_pad),
                "apps/minesweeper/components/status-panel/left-pad.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::counter_component),
                "apps/minesweeper/components/status-panel/counter/counter.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_component),
                "apps/minesweeper/components/status-panel/status-panel.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::mine_layer_render),
                "apps/minesweeper/components/board/mine-layer/mine-layer-render.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::mine_layer_mode),
                "apps/minesweeper/components/board/mine-layer/mine-layer.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_button_component),
                "apps/minesweeper/components/board/board-button/board-button.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::board_button_row_component),
                                   "apps/minesweeper/components/board/board-button-row/"
                                   "board-button-row.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_buttons_component),
                "apps/minesweeper/components/board/board-buttons/board-buttons.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_mode),
                "apps/minesweeper/components/board/board-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_definition),
                "apps/minesweeper/components/menu/menu-definition.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::new_game_state),
                "apps/minesweeper/game-logic/new-game-state.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::game_rules),
                                   "apps/minesweeper/game-logic/game-rules.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::button_component),
                                   "shared/ui/components/button/button-component.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::text_node_component),
                "shared/ui/components/text-node/text-node.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_item_component),
                "shared/ui/components/menu/menu-item/menu-item.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_bar_mode),
                "shared/ui/components/menu/menu-bar/menu-bar-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::popup_menu_mode),
                "shared/ui/components/menu/popup-menu/popup-menu.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::win311_theme),
                                   "shared/ui/themes/win311/win-theme.lisple"));

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::core),
                   .disk_path = "pixils/test/app/minesweeper/core.lisple",
                   .namespace_name = main_namespace(),
                   .unit_ids = {
                     std::string(unit_ids::program),
                     std::string(unit_ids::main_mode),
                     std::string(unit_ids::window_mode),
                     std::string(unit_ids::game_mode),
                     std::string(unit_ids::status_bundle),
                     std::string(unit_ids::counter_bundle),
                     std::string(unit_ids::status_panel_left_pad),
                     std::string(unit_ids::counter_component),
                     std::string(unit_ids::status_panel_component),
                     std::string(unit_ids::mine_layer_render),
                     std::string(unit_ids::mine_layer_mode),
                     std::string(unit_ids::board_button_component),
                     std::string(unit_ids::board_button_row_component),
                     std::string(unit_ids::board_buttons_component),
                     std::string(unit_ids::board_mode),
                   }});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::game_logic),
                   .disk_path = "pixils/test/app/minesweeper/game-logic.lisple",
                   .namespace_name = "pixils.test.app.minesweeper.game-logic"s,
                   .unit_ids = {
                     std::string(unit_ids::new_game_state),
                     std::string(unit_ids::game_rules),
                   }});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::menu_definition),
                   .disk_path = "pixils/test/app/minesweeper/menu-definition.lisple",
                   .namespace_name = "pixils.test.app.minesweeper.menu-definition"s,
                   .unit_ids = {std::string(unit_ids::menu_definition)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_button),
                   .disk_path = "pixils/test/app/shared/ui/components/button.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.components.button"s,
                   .unit_ids = {std::string(unit_ids::button_component)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_text_node),
                   .disk_path = "pixils/test/app/shared/ui/components/text-node.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.components.text-node"s,
                   .unit_ids = {std::string(unit_ids::text_node_component)}});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_menu),
                   .disk_path = "pixils/test/app/shared/ui/components/menu.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.components.menu"s,
                   .unit_ids = {
                     std::string(unit_ids::menu_item_component),
                     std::string(unit_ids::menu_bar_mode),
                     std::string(unit_ids::popup_menu_mode),
                   }});

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_win311),
                   .disk_path = "pixils/test/app/shared/ui/themes/win311.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.themes.win311"s,
                   .unit_ids = {std::string(unit_ids::win311_theme)}});

    return manifest;
  }

  AppManifest default_manifest()
  {
    return canonical_manifest();
  }

  std::string main_namespace()
  {
    return "pixils.test.app.minesweeper.core";
  }

  std::vector<std::string> implicit_fill_entry_files()
  {
    return {
      "pixils/test/app/shared/ui/themes/win311.lisple",
      "pixils/test/app/shared/ui/components/button.lisple",
      "pixils/test/app/minesweeper/menu-definition.lisple",
      "pixils/test/app/minesweeper/game-logic.lisple",
      "pixils/test/app/minesweeper/core.lisple",
    };
  }

  std::vector<std::string> canonical_entry_files()
  {
    return {
      "pixils/test/app/shared/ui/themes/win311.lisple",
      "pixils/test/app/shared/ui/components/button.lisple",
      "pixils/test/app/shared/ui/components/text-node.lisple",
      "pixils/test/app/minesweeper/menu-definition.lisple",
      "pixils/test/app/shared/ui/components/menu.lisple",
      "pixils/test/app/minesweeper/game-logic.lisple",
      "pixils/test/app/minesweeper/core.lisple",
    };
  }

  std::vector<std::string> entry_files()
  {
    return canonical_entry_files();
  }
} // namespace Pixils::Test::AppFixture::Minesweeper
