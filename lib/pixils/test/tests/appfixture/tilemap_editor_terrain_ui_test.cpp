#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest, terrain_unit_combo_updates_selected_terrain_set)
{
  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  update_cycle();

  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("workspace-tab"),
                             Lisple::keyword("terrains"));
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("selected-terrain-set-index"),
                             Lisple::number(0));
  Lisple::Dict::set_property(
    session.active_mode->state,
    Lisple::keyword("terrain-sets"),
    runtime.eval(R"([{:id :mountains
                      :label "Mountains"
                      :terrains []}])"));

  update_cycle();
  session.render_mode();

  std::vector<std::shared_ptr<Pixils::Runtime::View>> combo_boxes;
  find_descendant_modes(session.active_mode, "ui/combo-box", combo_boxes);

  std::shared_ptr<Pixils::Runtime::View> terrain_unit_combo;
  for (const auto& combo : combo_boxes)
  {
    if (combo->state && combo->state->to_string().find(":unit-2x2") != std::string::npos)
    {
      terrain_unit_combo = combo;
      break;
    }
  }
  ASSERT_NE(terrain_unit_combo, nullptr);

  input().mouse_down({terrain_unit_combo->bounds.x + 5, terrain_unit_combo->bounds.y + 5});
  update_cycle();
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  session.render_mode();

  auto popup_panel = session.active_mode->children[0];
  ASSERT_NE(popup_panel, nullptr);
  input().mouse_down({popup_panel->bounds.x + 5, popup_panel->bounds.y + 25});
  update_cycle();
  input().mouse_up({popup_panel->bounds.x + 5, popup_panel->bounds.y + 25});
  update_cycle();
  update_cycle();
  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "main-mode");
  std::vector<std::shared_ptr<Pixils::Runtime::View>> workspaces;
  find_descendant_modes(session.active_mode, "terrain-definition-workspace", workspaces);
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_NE(session.active_mode->state->to_string().find(":terrain-unit {:w 2 :h 2}"),
            std::string::npos)
    << "root: " << session.active_mode->state->to_string()
    << "\nworkspace: " << workspaces[0]->state->to_string();

  combo_boxes.clear();
  find_descendant_modes(session.active_mode, "ui/combo-box", combo_boxes);
  terrain_unit_combo = nullptr;
  for (const auto& combo : combo_boxes)
  {
    if (combo->state && combo->state->to_string().find(":unit-2x2") != std::string::npos)
    {
      terrain_unit_combo = combo;
      break;
    }
  }
  ASSERT_NE(terrain_unit_combo, nullptr);
  std::vector<std::shared_ptr<Pixils::Runtime::View>> unit_selectors;
  find_descendant_modes(session.active_mode, "terrain-unit-selector", unit_selectors);
  ASSERT_EQ(unit_selectors.size(), 1u);
  EXPECT_NE(terrain_unit_combo->state->to_string().find(":selected-index 1"),
            std::string::npos)
    << "selector: " << unit_selectors[0]->state->to_string()
    << "\ncombo: " << terrain_unit_combo->state->to_string();
  EXPECT_NE(terrain_unit_combo->state->to_string().find(":selected-label \"2 x 2\""),
            std::string::npos)
    << terrain_unit_combo->state->to_string();
}
