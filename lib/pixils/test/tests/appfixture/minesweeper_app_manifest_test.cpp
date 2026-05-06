#include "minesweeper_app_manifest.h"

#include <gtest/gtest.h>

namespace AppFixture = Pixils::Test::AppFixture;
namespace Minesweeper = Pixils::Test::AppFixture::Minesweeper;

namespace
{
  const AppFixture::ComposedFile* find_file(
    const std::vector<AppFixture::ComposedFile>& files,
    const std::string& namespace_name)
  {
    for (const auto& file : files)
    {
      if (file.namespace_name == namespace_name) return &file;
    }

    return nullptr;
  }
} // namespace

TEST(MinesweeperAppManifestTest, materializes_implicit_fill_catalog)
{
  auto files = Minesweeper::implicit_fill_manifest().materialize_files();

  ASSERT_EQ(files.size(), 5u);

  const auto* core = find_file(files, Minesweeper::main_namespace());
  ASSERT_NE(core, nullptr);
  ASSERT_EQ(core->units.size(), 16u);
  EXPECT_EQ(core->units[0].id, Minesweeper::unit_ids::program);
  EXPECT_EQ(core->units[1].id, Minesweeper::unit_ids::implicit_fill_window_mode);
  EXPECT_EQ(core->units[2].id, Minesweeper::unit_ids::implicit_fill_game_layout);
  EXPECT_EQ(core->units[3].id, Minesweeper::unit_ids::mine_layer_render);
  EXPECT_EQ(core->units[4].id, Minesweeper::unit_ids::render_only_mine_layer);
  EXPECT_EQ(core->units[5].id, Minesweeper::unit_ids::inhibit_only_board_button_component);
  EXPECT_EQ(core->units[6].id, Minesweeper::unit_ids::board_button_row_component);
  EXPECT_EQ(core->units[7].id, Minesweeper::unit_ids::board_buttons_component);
  EXPECT_EQ(core->units[8].id, Minesweeper::unit_ids::overlay_button_board_mode);
  EXPECT_EQ(core->units[9].id, Minesweeper::unit_ids::status_panel_left_pad);
  EXPECT_EQ(core->units[10].id,
            Minesweeper::unit_ids::fixed_size_rendered_counter_component);
  EXPECT_EQ(core->units[11].id, Minesweeper::unit_ids::fixed_size_counter_status_panel);
  EXPECT_EQ(core->units[12].id, Minesweeper::unit_ids::counter_font);
  EXPECT_EQ(core->units[13].id, Minesweeper::unit_ids::custom_rendered_menu_item);
  EXPECT_EQ(core->units[14].id, Minesweeper::unit_ids::fixed_width_menu_bar_mode);
  EXPECT_EQ(core->units[15].id, Minesweeper::unit_ids::popup_menu_mode);

  const auto* game_logic = find_file(files, "pixils.test.app.minesweeper.game-logic");
  ASSERT_NE(game_logic, nullptr);
  ASSERT_EQ(game_logic->units.size(), 2u);
  EXPECT_EQ(game_logic->units[0].id,
            Minesweeper::unit_ids::status_panel_face_new_game_state);
  EXPECT_EQ(game_logic->units[1].id, Minesweeper::unit_ids::game_rules);

  const auto* menu_definition =
    find_file(files, "pixils.test.app.minesweeper.menu-definition");
  ASSERT_NE(menu_definition, nullptr);
  ASSERT_EQ(menu_definition->units.size(), 1u);
  EXPECT_EQ(menu_definition->units[0].id, Minesweeper::unit_ids::menu_definition);
}

TEST(MinesweeperAppManifestTest, exposes_implicit_fill_entry_files_in_dependency_load_order)
{
  EXPECT_EQ(Minesweeper::implicit_fill_entry_files(),
            (std::vector<std::string>{
              "pixils/test/app/shared/ui/themes/win311.lisple",
              "pixils/test/app/shared/ui/components/button.lisple",
              "pixils/test/app/minesweeper/menu-definition.lisple",
              "pixils/test/app/minesweeper/game-logic.lisple",
              "pixils/test/app/minesweeper/core.lisple",
            }));
}

TEST(MinesweeperAppManifestTest, materializes_canonical_catalog)
{
  auto files = Minesweeper::canonical_manifest().materialize_files();

  ASSERT_EQ(files.size(), 7u);

  const auto* core = find_file(files, Minesweeper::main_namespace());
  ASSERT_NE(core, nullptr);
  ASSERT_EQ(core->units.size(), 15u);
  EXPECT_EQ(core->units[0].id, Minesweeper::unit_ids::program);
  EXPECT_EQ(core->units[1].id, Minesweeper::unit_ids::main_mode);
  EXPECT_EQ(core->units[2].id, Minesweeper::unit_ids::window_mode);
  EXPECT_EQ(core->units[3].id, Minesweeper::unit_ids::game_mode);
  EXPECT_EQ(core->units[4].id, Minesweeper::unit_ids::status_bundle);
  EXPECT_EQ(core->units[5].id, Minesweeper::unit_ids::counter_bundle);
  EXPECT_EQ(core->units[6].id, Minesweeper::unit_ids::status_panel_left_pad);
  EXPECT_EQ(core->units[7].id, Minesweeper::unit_ids::counter_component);
  EXPECT_EQ(core->units[8].id, Minesweeper::unit_ids::status_panel_component);
  EXPECT_EQ(core->units[9].id, Minesweeper::unit_ids::mine_layer_render);
  EXPECT_EQ(core->units[10].id, Minesweeper::unit_ids::mine_layer_mode);
  EXPECT_EQ(core->units[11].id, Minesweeper::unit_ids::board_button_component);
  EXPECT_EQ(core->units[12].id, Minesweeper::unit_ids::board_button_row_component);
  EXPECT_EQ(core->units[13].id, Minesweeper::unit_ids::board_buttons_component);
  EXPECT_EQ(core->units[14].id, Minesweeper::unit_ids::board_mode);

  const auto* game_logic = find_file(files, "pixils.test.app.minesweeper.game-logic");
  ASSERT_NE(game_logic, nullptr);
  ASSERT_EQ(game_logic->units.size(), 2u);
  EXPECT_EQ(game_logic->units[0].id, Minesweeper::unit_ids::new_game_state);
  EXPECT_EQ(game_logic->units[1].id, Minesweeper::unit_ids::game_rules);

  const auto* menu_definition =
    find_file(files, "pixils.test.app.minesweeper.menu-definition");
  ASSERT_NE(menu_definition, nullptr);
  ASSERT_EQ(menu_definition->units.size(), 1u);
  EXPECT_EQ(menu_definition->units[0].id, Minesweeper::unit_ids::menu_definition);

  const auto* shared_text_node =
    find_file(files, "pixils.test.app.shared.ui.components.text-node");
  ASSERT_NE(shared_text_node, nullptr);
  ASSERT_EQ(shared_text_node->units.size(), 1u);
  EXPECT_EQ(shared_text_node->units[0].id, Minesweeper::unit_ids::text_node_component);

  const auto* shared_menu = find_file(files, "pixils.test.app.shared.ui.components.menu");
  ASSERT_NE(shared_menu, nullptr);
  ASSERT_EQ(shared_menu->units.size(), 3u);
  EXPECT_EQ(shared_menu->units[0].id, Minesweeper::unit_ids::menu_item_component);
  EXPECT_EQ(shared_menu->units[1].id, Minesweeper::unit_ids::menu_bar_mode);
  EXPECT_EQ(shared_menu->units[2].id, Minesweeper::unit_ids::popup_menu_mode);
}

TEST(MinesweeperAppManifestTest, exposes_canonical_entry_files_in_dependency_load_order)
{
  EXPECT_EQ(Minesweeper::canonical_entry_files(),
            (std::vector<std::string>{
              "pixils/test/app/shared/ui/themes/win311.lisple",
              "pixils/test/app/shared/ui/components/button.lisple",
              "pixils/test/app/shared/ui/components/text-node.lisple",
              "pixils/test/app/minesweeper/menu-definition.lisple",
              "pixils/test/app/shared/ui/components/menu.lisple",
              "pixils/test/app/minesweeper/game-logic.lisple",
              "pixils/test/app/minesweeper/core.lisple",
            }));
}

TEST(MinesweeperAppManifestTest, default_manifest_points_at_canonical_baseline)
{
  EXPECT_EQ(Minesweeper::entry_files(), Minesweeper::canonical_entry_files());

  const auto files = Minesweeper::default_manifest().materialize_files();
  const auto* shared_text_node =
    find_file(files, "pixils.test.app.shared.ui.components.text-node");
  ASSERT_NE(shared_text_node, nullptr);
  ASSERT_EQ(shared_text_node->units.size(), 1u);
  EXPECT_EQ(shared_text_node->units[0].id, Minesweeper::unit_ids::text_node_component);
}
