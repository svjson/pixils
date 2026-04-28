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
    inline constexpr std::string_view shared_button = "shared/ui/components/button";
    inline constexpr std::string_view shared_menu = "shared/ui/components/menu";
    inline constexpr std::string_view shared_win311 = "shared/ui/themes/win311";
  } // namespace file_ids

  namespace unit_ids
  {
    inline constexpr std::string_view program = "minesweeper/program";
    inline constexpr std::string_view window_mode = "minesweeper/window-mode";
    inline constexpr std::string_view game_layout = "minesweeper/game-layout";
    inline constexpr std::string_view mine_layer = "minesweeper/board/mine-layer";
    inline constexpr std::string_view board_buttons = "minesweeper/board/board-buttons";
    inline constexpr std::string_view board_mode = "minesweeper/board/board-mode";
    inline constexpr std::string_view status_panel = "minesweeper/status-panel/status-panel";
    inline constexpr std::string_view game_rules = "minesweeper/game-logic/game-rules";
    inline constexpr std::string_view button_component = "shared/ui/components/button";
    inline constexpr std::string_view menu_components = "shared/ui/components/menu";
    inline constexpr std::string_view win311_theme = "shared/ui/themes/win311";
  } // namespace unit_ids

  AppManifest default_manifest();

  std::string main_namespace();
  std::vector<std::string> entry_files();
} // namespace Pixils::Test::AppFixture::Minesweeper

#endif
