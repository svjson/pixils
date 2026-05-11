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
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::mine_layer_render),
      "apps/minesweeper/components/board/mine-layer/fixed-grid-mine-layer-render.lisple"));
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
                "apps/minesweeper/components/board/board-buttons/settings-sized-board-buttons.lisple"));
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
                "apps/minesweeper/components/menu/simple-menu-definition.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::custom_rendered_menu_item),
                "shared/ui/components/menu/menu-item/custom-rendered-menu-item.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::fixed_width_menu_bar_mode),
                "shared/ui/components/menu/menu-bar/fixed-width-menu-bar-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::popup_menu_mode),
                "shared/ui/components/menu/popup-menu/simple-popup-menu.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_face_new_game_state),
                "apps/minesweeper/game-logic/status-panel-face-new-game-state.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::game_rules),
                                   "apps/minesweeper/game-logic/game-rules.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::button_component),
                "shared/ui/components/button/rendered-button-component.lisple"));
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

  AppManifest pre_windowed_manifest()
  {
    AppManifest manifest;

    manifest.upsert_unit(
      load_unit(std::string(unit_ids::program), "apps/minesweeper/unthemed-program.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::main_mode),
                "apps/minesweeper/modes/main-mode/simple-main-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::window_mode),
                "apps/minesweeper/modes/window/bordered-window-mode.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::game_mode),
      "apps/minesweeper/components/game-layout/self-initializing-game-mode.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::status_bundle),
                                   "apps/minesweeper/bundles/status/status.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::counter_bundle),
                                   "apps/minesweeper/bundles/counter/counter.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_left_pad),
                "apps/minesweeper/components/status-panel/left-pad.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::counter_component),
                "apps/minesweeper/components/status-panel/counter/counter.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::status_panel_component),
      "apps/minesweeper/components/status-panel/fixed-height-status-panel.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::mine_layer_render),
      "apps/minesweeper/components/board/mine-layer/fixed-grid-mine-layer-render.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::mine_layer_mode),
      "apps/minesweeper/components/board/mine-layer/board-derived-mine-layer.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::board_button_component),
                                   "apps/minesweeper/components/board/board-button/"
                                   "delegated-pressed-board-button.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::board_button_row_component),
                                   "apps/minesweeper/components/board/board-button-row/"
                                   "static-board-button-row.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::board_buttons_component),
      "apps/minesweeper/components/board/board-buttons/static-board-buttons.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_mode),
                "apps/minesweeper/components/board/no-settings-board-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_definition),
                "apps/minesweeper/components/menu/simple-menu-definition.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::new_game_state),
                "apps/minesweeper/game-logic/positional-new-game-state.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::game_rules),
                                   "apps/minesweeper/game-logic/game-rules.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::button_component),
                "shared/ui/components/button/rendered-button-component.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::text_node_component),
                                   "shared/ui/components/text-node/text-node.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_item_component),
                "shared/ui/components/menu/menu-item/simple-menu-item.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_bar_mode),
                "shared/ui/components/menu/menu-bar/static-menu-bar-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::popup_menu_mode),
                "shared/ui/components/menu/popup-menu/simple-popup-menu.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::win311_theme),
                                   "shared/ui/themes/win311/primitives-win-theme.lisple"));

    manifest.add_file(ManifestFile{.id = std::string(file_ids::core),
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

  AppManifest simple_windowed_manifest()
  {
    auto manifest = pre_windowed_manifest();
    manifest.remove_unit(std::string(unit_ids::text_node_component));
    manifest.remove_file(std::string(file_ids::shared_text_node));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::program), "apps/minesweeper/program.lisple"));

    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_definition),
                "apps/minesweeper/components/menu/simple-windowed-menu-definition.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::main_mode),
                                   "apps/minesweeper/modes/main-mode/simple-windowed-main-mode.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::window_mode),
                                   "apps/minesweeper/modes/window/simple-windowed-window-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::status_panel_component),
                "apps/minesweeper/components/status-panel/beveled-status-panel.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::mine_layer_render),
                "apps/minesweeper/components/board/mine-layer/settings-grid-mine-layer-render.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::mine_layer_mode),
                "apps/minesweeper/components/board/mine-layer/settings-derived-mine-layer.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_button_component),
                "apps/minesweeper/components/board/board-button/flag-aware-board-button.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::board_button_row_component),
                                   "apps/minesweeper/components/board/board-button-row/"
                                   "board-button-row.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::board_buttons_component),
                "apps/minesweeper/components/board/board-buttons/settings-sized-board-buttons.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::board_mode),
                                   "apps/minesweeper/components/board/chord-board-mode.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::button_component),
                                   "shared/ui/components/button/child-rendered-button-component.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::button_inner_component),
                "shared/ui/components/button/button-inner/button-inner.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::menu_item_component),
                                   "shared/ui/components/menu/menu-item/simple-windowed-menu-item.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_option_indicator_component),
                "shared/ui/components/menu/menu-option-item/menu-option-indicator/"
                "menu-option-indicator.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_option_item_component),
                "shared/ui/components/menu/menu-option-item/menu-option-item.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_separator_component),
                "shared/ui/components/menu/menu-separator/menu-separator.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::menu_bar_mode),
                "shared/ui/components/menu/menu-bar/simple-windowed-menu-bar-mode.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::popup_menu_mode),
                "shared/ui/components/menu/popup-menu/simple-windowed-popup-menu.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::popup_menu_outer_mode),
                                   "shared/ui/components/menu/popup-menu/popup-menu-outer/"
                                   "popup-menu-outer.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::popup_menu_inner_mode),
                "shared/ui/components/menu/popup-menu/popup-menu-outer/popup-menu-inner/"
                "popup-menu-inner.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::window_component),
                                   "shared/ui/components/window/window-component.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::window_title_bar_component),
                "shared/ui/components/window/window-title-bar/window-title-bar.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::window_control_button_component),
                "shared/ui/components/window/window-title-bar/window-control-button/"
                "window-control-button.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::window_minimize_button_component),
                "shared/ui/components/window/window-title-bar/window-minimize-button/"
                "window-minimize-button.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::window_body_component),
                "shared/ui/components/window/window-body/window-body.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::win311_theme),
                                   "shared/ui/themes/win311/win-theme.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::new_game_state),
                                   "apps/minesweeper/game-logic/settings-map-new-game-state.lisple"));
    manifest.upsert_unit(load_unit(
      std::string(unit_ids::highscore_row_component),
      "apps/minesweeper/components/highscores-window/highscore-table/highscore-row/"
      "highscore-row.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::highscore_table_component),
                "apps/minesweeper/components/highscores-window/highscore-table/"
                "highscore-table.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::highscore_buttons_component),
                "apps/minesweeper/components/highscores-window/highscore-buttons/"
                "highscore-buttons.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::highscores_window_component),
                "apps/minesweeper/components/highscores-window/highscores-window.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::highscores_modal_mode),
                "apps/minesweeper/modes/highscores-modal/highscores-modal.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::game_mode),
                "apps/minesweeper/components/game-layout/settings-driven-game-mode.lisple"));

    manifest.remove_unit_from_file(std::string(file_ids::shared_button),
                                   std::string(unit_ids::button_component));
    manifest.remove_unit_from_file(std::string(file_ids::core),
                                   std::string(unit_ids::main_mode));
    manifest.remove_unit_from_file(std::string(file_ids::core),
                                   std::string(unit_ids::window_mode));
    manifest.remove_unit_from_file(std::string(file_ids::core),
                                   std::string(unit_ids::game_mode));
    manifest.append_unit_to_file(std::string(file_ids::shared_button),
                                 std::string(unit_ids::button_inner_component));
    manifest.append_unit_to_file(std::string(file_ids::shared_button),
                                 std::string(unit_ids::button_component));
    manifest.insert_unit_after(std::string(file_ids::shared_menu),
                               std::string(unit_ids::menu_item_component),
                               std::string(unit_ids::menu_option_indicator_component));
    manifest.insert_unit_after(std::string(file_ids::shared_menu),
                               std::string(unit_ids::menu_option_indicator_component),
                               std::string(unit_ids::menu_option_item_component));
    manifest.insert_unit_after(std::string(file_ids::shared_menu),
                               std::string(unit_ids::menu_option_item_component),
                               std::string(unit_ids::menu_separator_component));
    manifest.insert_unit_after(std::string(file_ids::shared_menu),
                               std::string(unit_ids::popup_menu_mode),
                               std::string(unit_ids::popup_menu_outer_mode));
    manifest.insert_unit_after(std::string(file_ids::shared_menu),
                               std::string(unit_ids::popup_menu_outer_mode),
                               std::string(unit_ids::popup_menu_inner_mode));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::highscore_row_component));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::highscore_table_component));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::highscore_buttons_component));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::highscores_window_component));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::highscores_modal_mode));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::game_mode));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::window_mode));
    manifest.append_unit_to_file(std::string(file_ids::core),
                                 std::string(unit_ids::main_mode));

    manifest.add_file(
      ManifestFile{.id = std::string(file_ids::shared_window),
                   .disk_path = "pixils/test/app/shared/ui/components/window.lisple",
                   .namespace_name = "pixils.test.app.shared.ui.components.window"s,
                   .unit_ids = {
                     std::string(unit_ids::window_control_button_component),
                     std::string(unit_ids::window_minimize_button_component),
                     std::string(unit_ids::window_body_component),
                     std::string(unit_ids::window_title_bar_component),
                     std::string(unit_ids::window_component),
                   }});

    return manifest;
  }

  AppManifest keyboard_menu_windowed_manifest()
  {
    auto manifest = simple_windowed_manifest();

    manifest.upsert_unit(load_unit(std::string(unit_ids::menu_definition),
                                   "apps/minesweeper/components/menu/"
                                   "keyboard-menu-windowed-menu-definition.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::window_mode),
                                   "apps/minesweeper/modes/window/"
                                   "keyboard-menu-windowed-window-mode.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::menu_item_component),
                                   "shared/ui/components/menu/menu-item/"
                                   "keyboard-menu-windowed-menu-item.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::menu_option_item_component),
                                   "shared/ui/components/menu/menu-option-item/"
                                   "keyboard-menu-windowed-menu-option-item.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::menu_bar_mode),
                                   "shared/ui/components/menu/menu-bar/"
                                   "keyboard-menu-windowed-menu-bar-mode.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::popup_menu_mode),
                                   "shared/ui/components/menu/popup-menu/"
                                   "keyboard-menu-windowed-popup-menu.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::popup_menu_inner_mode),
                                   "shared/ui/components/menu/popup-menu/popup-menu-outer/"
                                   "popup-menu-inner/"
                                   "keyboard-menu-windowed-popup-menu-inner.lisple"));
    manifest.upsert_unit(
      load_unit(std::string(unit_ids::highscore_buttons_component),
                "apps/minesweeper/components/highscores-window/highscore-buttons/"
                "keyboard-menu-windowed-highscore-buttons.lisple"));
    manifest.upsert_unit(load_unit(std::string(unit_ids::win311_theme),
                                   "shared/ui/themes/win311/"
                                   "keyboard-menu-windowed-win-theme.lisple"));

    return manifest;
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

  std::vector<std::string> pre_windowed_entry_files()
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

  std::vector<std::string> simple_windowed_entry_files()
  {
    return {
      "pixils/test/app/shared/ui/themes/win311.lisple",
      "pixils/test/app/shared/ui/components/button.lisple",
      "pixils/test/app/minesweeper/menu-definition.lisple",
      "pixils/test/app/shared/ui/components/menu.lisple",
      "pixils/test/app/minesweeper/game-logic.lisple",
      "pixils/test/app/shared/ui/components/window.lisple",
      "pixils/test/app/minesweeper/core.lisple",
    };
  }

  std::vector<std::string> keyboard_menu_windowed_entry_files()
  {
    return {
      "pixils/test/app/shared/ui/themes/win311.lisple",
      "pixils/test/app/shared/ui/components/button.lisple",
      "pixils/test/app/minesweeper/menu-definition.lisple",
      "pixils/test/app/shared/ui/components/menu.lisple",
      "pixils/test/app/minesweeper/game-logic.lisple",
      "pixils/test/app/shared/ui/components/window.lisple",
      "pixils/test/app/minesweeper/core.lisple",
    };
  }
} // namespace Pixils::Test::AppFixture::Minesweeper
