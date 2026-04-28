#include "minesweeper_app_manifest.h"

#include <gtest/gtest.h>

namespace AppFixture = Pixils::Test::AppFixture;
namespace Minesweeper = Pixils::Test::AppFixture::Minesweeper;

namespace
{
  const AppFixture::ComposedFile* find_file(const std::vector<AppFixture::ComposedFile>& files,
                                            const std::string& namespace_name)
  {
    for (const auto& file : files)
    {
      if (file.namespace_name == namespace_name) return &file;
    }

    return nullptr;
  }
} // namespace

TEST(MinesweeperAppManifestTest, materializes_current_default_catalog)
{
  auto files = Minesweeper::default_manifest().materialize_files();

  ASSERT_EQ(files.size(), 5u);

  const auto* core = find_file(files, Minesweeper::main_namespace());
  ASSERT_NE(core, nullptr);
  ASSERT_EQ(core->units.size(), 7u);
  EXPECT_EQ(core->units[0].id, Minesweeper::unit_ids::program);
  EXPECT_EQ(core->units[1].id, Minesweeper::unit_ids::window_mode);
  EXPECT_EQ(core->units[2].id, Minesweeper::unit_ids::game_layout);
  EXPECT_EQ(core->units[3].id, Minesweeper::unit_ids::mine_layer);
  EXPECT_EQ(core->units[4].id, Minesweeper::unit_ids::board_buttons);
  EXPECT_EQ(core->units[5].id, Minesweeper::unit_ids::board_mode);
  EXPECT_EQ(core->units[6].id, Minesweeper::unit_ids::status_panel);

  const auto* game_logic = find_file(files, "pixils.test.app.minesweeper.game-logic");
  ASSERT_NE(game_logic, nullptr);
  ASSERT_EQ(game_logic->units.size(), 1u);
  EXPECT_EQ(game_logic->units[0].id, Minesweeper::unit_ids::game_rules);
}

TEST(MinesweeperAppManifestTest, exposes_entry_files_in_dependency_load_order)
{
  EXPECT_EQ(Minesweeper::entry_files(),
            (std::vector<std::string>{
              "pixils/test/app/shared/ui/themes/win311.lisple",
              "pixils/test/app/shared/ui/components/button.lisple",
              "pixils/test/app/shared/ui/components/menu.lisple",
              "pixils/test/app/minesweeper/game-logic.lisple",
              "pixils/test/app/minesweeper/core.lisple",
            }));
}
