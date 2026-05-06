#ifndef PIXILS__TEST__APPFIXTURE__MINESWEEPER_APP_MANIFEST_H
#define PIXILS__TEST__APPFIXTURE__MINESWEEPER_APP_MANIFEST_H

#include "app_manifest.h"

#include <string>
#include <string_view>
#include <vector>

namespace Pixils::Test::AppFixture::Minesweeper
{
  namespace file_ids
  {
    inline constexpr std::string_view core = "minesweeper/core";
    inline constexpr std::string_view game_logic = "minesweeper/game-logic";
    inline constexpr std::string_view menu_definition = "minesweeper/menu-definition";
    inline constexpr std::string_view shared_button = "shared/ui/components/button";
    inline constexpr std::string_view shared_text_node = "shared/ui/components/text-node";
    inline constexpr std::string_view shared_menu = "shared/ui/components/menu";
    inline constexpr std::string_view shared_win311 = "shared/ui/themes/win311";
  } // namespace file_ids

  namespace unit_ids
  {
    inline constexpr std::string_view program = "minesweeper/program";
    inline constexpr std::string_view main_mode = "minesweeper/main-mode";
    inline constexpr std::string_view window_mode = "minesweeper/window-mode";
    inline constexpr std::string_view game_mode = "minesweeper/game-mode";
    inline constexpr std::string_view status_bundle = "minesweeper/bundles/status";
    inline constexpr std::string_view counter_bundle = "minesweeper/bundles/counter";
    inline constexpr std::string_view implicit_fill_window_mode =
      "minesweeper/implicit-fill/window-mode";
    inline constexpr std::string_view implicit_fill_game_layout =
      "minesweeper/implicit-fill/game-layout";
    inline constexpr std::string_view mine_layer_render =
      "minesweeper/board/mine-layer/mine-layer-render";
    inline constexpr std::string_view mine_layer_mode = "minesweeper/board/mine-layer";
    inline constexpr std::string_view board_button_component =
      "minesweeper/board/board-button/board-button";
    inline constexpr std::string_view render_only_mine_layer =
      "minesweeper/board/render-only-mine-layer";
    inline constexpr std::string_view inhibit_only_board_button_component =
      "minesweeper/board/board-button/inhibit-only-board-button";
    inline constexpr std::string_view board_button_row_component =
      "minesweeper/board/board-button-row/board-button-row";
    inline constexpr std::string_view board_buttons_component =
      "minesweeper/board/board-buttons/board-buttons";
    inline constexpr std::string_view board_mode = "minesweeper/board/board-mode";
    inline constexpr std::string_view overlay_button_board_mode =
      "minesweeper/board/overlay-button-board-mode";
    inline constexpr std::string_view status_panel_left_pad =
      "minesweeper/status-panel/left-pad";
    inline constexpr std::string_view counter_component =
      "minesweeper/status-panel/counter/counter";
    inline constexpr std::string_view fixed_size_rendered_counter_component =
      "minesweeper/status-panel/counter/fixed-size-rendered-counter";
    inline constexpr std::string_view counter_font =
      "minesweeper/status-panel/counter-font/counter-font";
    inline constexpr std::string_view status_panel_component =
      "minesweeper/status-panel/status-panel";
    inline constexpr std::string_view fixed_size_counter_status_panel =
      "minesweeper/status-panel/fixed-size-counter-status-panel";
    inline constexpr std::string_view menu_definition = "minesweeper/menu/menu-definition";
    inline constexpr std::string_view menu_item_component =
      "shared/ui/components/menu/menu-item/menu-item";
    inline constexpr std::string_view menu_bar_mode =
      "shared/ui/components/menu/menu-bar/menu-bar-mode";
    inline constexpr std::string_view custom_rendered_menu_item =
      "shared/ui/components/menu/menu-item/custom-rendered-menu-item";
    inline constexpr std::string_view fixed_width_menu_bar_mode =
      "shared/ui/components/menu/menu-bar/fixed-width-menu-bar-mode";
    inline constexpr std::string_view popup_menu_mode =
      "shared/ui/components/menu/popup-menu/popup-menu";
    inline constexpr std::string_view status_panel_face_new_game_state =
      "minesweeper/game-logic/status-panel-face-new-game-state";
    inline constexpr std::string_view new_game_state = "minesweeper/game-logic/new-game-state";
    inline constexpr std::string_view game_rules = "minesweeper/game-logic/game-rules";
    inline constexpr std::string_view button_component = "shared/ui/components/button";
    inline constexpr std::string_view text_node_component = "shared/ui/components/text-node";
    inline constexpr std::string_view win311_theme = "shared/ui/themes/win311";
  } // namespace unit_ids

  AppManifest implicit_fill_manifest();
  AppManifest canonical_manifest();
  AppManifest default_manifest();

  std::string main_namespace();
  std::vector<std::string> implicit_fill_entry_files();
  std::vector<std::string> canonical_entry_files();
  std::vector<std::string> entry_files();
} // namespace Pixils::Test::AppFixture::Minesweeper

#endif
