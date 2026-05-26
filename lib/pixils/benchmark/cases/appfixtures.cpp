#include "../support/benchmark.h"
#include "appfixture/composable_app_session_fixture.h"
#include "appfixture/minesweeper_app_manifest.h"
#include "appfixture/tilemap_editor_app_manifest.h"
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <sdl2_mock/mock_resources.h>
#include <string>
#include <vector>

namespace
{
  namespace Minesweeper = Pixils::Test::AppFixture::Minesweeper;
  namespace TilemapEditor = Pixils::Test::AppFixture::TilemapEditor;

  std::shared_ptr<Pixils::Runtime::View> find_first_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;
    for (const auto& child : view->children)
    {
      if (auto match = find_first_mode(child, mode_name)) return match;
    }
    return nullptr;
  }

  void find_descendant_modes(const std::shared_ptr<Pixils::Runtime::View>& view,
                             const std::string& mode_name,
                             std::vector<std::shared_ptr<Pixils::Runtime::View>>& out)
  {
    if (!view) return;
    if (view->mode && view->mode->name == mode_name) out.push_back(view);
    for (const auto& child : view->children)
    {
      find_descendant_modes(child, mode_name, out);
    }
  }

  class AppFixtureBenchmark : public ComposableAppSessionFixture
  {
   protected:
    void load_minesweeper_simple_windowed()
    {
      set_frame_size({640, 480});
      load_app(Minesweeper::simple_windowed_manifest(),
               Minesweeper::main_namespace(),
               Minesweeper::simple_windowed_entry_files());
      load_program();
      warmup_frame_cycles(2);
    }

    void load_minesweeper_keyboard_menu_windowed()
    {
      set_frame_size({640, 480});
      load_app(Minesweeper::keyboard_menu_windowed_manifest(),
               Minesweeper::main_namespace(),
               Minesweeper::keyboard_menu_windowed_entry_files());
      load_program();
      warmup_frame_cycles(2);
    }

    void load_tilemap_editor_current()
    {
      set_frame_size({1200, 700});
      load_app(TilemapEditor::current_manifest(),
               TilemapEditor::main_namespace(),
               TilemapEditor::entry_files(),
               {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
      load_program();
      warmup_frame_cycles(2);
    }

    void load_tilemap_editor_layered()
    {
      set_frame_size({1200, 700});
      load_app(TilemapEditor::layered_manifest(),
               TilemapEditor::main_namespace(),
               TilemapEditor::layered_entry_files(),
               {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
      load_program();
      warmup_frame_cycles(2);
    }

    void clear_render_ops() { render_target()->render_ops.clear(); }

    void benchmark_frame_cycle(const std::string& name, std::size_t iterations)
    {
      Pixils::Benchmark::Case(name, Pixils::Benchmark::appfixture_category())
        .with_iterations(iterations)
        .run(
          [&]()
          {
            frame_cycle();
            Pixils::Benchmark::consume(
              static_cast<std::int64_t>(render_target()->render_ops.size()));
            clear_render_ops();
          });
    }

    void benchmark_render_cycle(const std::string& name, std::size_t iterations)
    {
      Pixils::Benchmark::Case(name, Pixils::Benchmark::appfixture_category())
        .with_iterations(iterations)
        .run(
          [&]()
          {
            render_cycle();
            Pixils::Benchmark::consume(
              static_cast<std::int64_t>(render_target()->render_ops.size()));
            clear_render_ops();
          });
    }

    void benchmark_update_flow(const std::string& name,
                               std::size_t iterations,
                               const std::function<void()>& workload)
    {
      Pixils::Benchmark::Case(name, Pixils::Benchmark::appfixture_category())
        .with_iterations(iterations)
        .run(
          [&]()
          {
            workload();
            Pixils::Benchmark::consume(
              session().active_mode ? session().active_mode->children.size() : 0);
            clear_render_ops();
          });
    }

    std::shared_ptr<Pixils::Runtime::View> first_mode(const std::string& mode_name)
    {
      return find_first_mode(session().active_mode, mode_name);
    }

    std::vector<std::shared_ptr<Pixils::Runtime::View>> descendant_modes(
      const std::string& mode_name)
    {
      std::vector<std::shared_ptr<Pixils::Runtime::View>> matches;
      find_descendant_modes(session().active_mode, mode_name, matches);
      return matches;
    }

    std::shared_ptr<Pixils::Runtime::View> first_child_of_first_mode(
      const std::string& mode_name)
    {
      auto parent = first_mode(mode_name);
      if (!parent || parent->children.empty()) return nullptr;
      return parent->children.front();
    }

    InputSimulator::Coord center_of(const std::shared_ptr<Pixils::Runtime::View>& view)
    {
      return {view->bounds.x + (view->bounds.w / 2), view->bounds.y + (view->bounds.h / 2)};
    }

    void click_at(InputSimulator::Coord position, Uint8 button = SDL_BUTTON_LEFT)
    {
      input().mouse_down(position, button);
      frame_cycle();
      input().mouse_up(position, button);
      frame_cycle();
    }

    void right_click_at(InputSimulator::Coord position)
    {
      click_at(position, SDL_BUTTON_RIGHT);
    }

   private:
    void warmup_frame_cycles(std::size_t cycles)
    {
      for (std::size_t i = 0; i < cycles; i++)
      {
        frame_cycle();
        clear_render_ops();
      }
    }
  };
} // namespace

TEST_F(AppFixtureBenchmark, minesweeper_simple_windowed_frame_cycle)
{
  ASSERT_NO_THROW(load_minesweeper_simple_windowed());

  benchmark_frame_cycle("appfixture_minesweeper_simple_windowed_frame_cycle", 100);
}

TEST_F(AppFixtureBenchmark, minesweeper_simple_window_drag_frame_cycle)
{
  ASSERT_NO_THROW(load_minesweeper_simple_windowed());
  render_cycle();

  auto title_bar = first_mode("window-title-bar");
  ASSERT_NE(title_bar, nullptr);
  const auto start = center_of(title_bar);
  input().mouse_down(start);
  frame_cycle();
  clear_render_ops();

  int offset = 0;
  benchmark_update_flow(
    "realworld_appfixture_minesweeper_simple_window_drag_frame_cycle",
    100,
    [&]()
    {
      offset++;
      input().mouse_move({start.first + offset, start.second + offset});
      frame_cycle();
    });
}

TEST_F(AppFixtureBenchmark, minesweeper_keyboard_menu_windowed_frame_cycle)
{
  ASSERT_NO_THROW(load_minesweeper_keyboard_menu_windowed());

  benchmark_frame_cycle("appfixture_minesweeper_keyboard_menu_windowed_frame_cycle", 100);
}

TEST_F(AppFixtureBenchmark, tilemap_editor_current_frame_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_current());

  benchmark_frame_cycle("appfixture_tilemap_editor_current_frame_cycle", 50);
}

TEST_F(AppFixtureBenchmark, tilemap_editor_layered_render_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_layered());

  benchmark_render_cycle("appfixture_tilemap_editor_layered_render_cycle", 50);
}

TEST_F(AppFixtureBenchmark, minesweeper_simple_menu_hover_toggle_frame_cycle)
{
  ASSERT_NO_THROW(load_minesweeper_simple_windowed());
  render_cycle();
  clear_render_ops();

  auto menu_item = first_mode("menu-item");
  ASSERT_NE(menu_item, nullptr);
  const auto hover = center_of(menu_item);
  bool inside = false;

  benchmark_update_flow(
    "realworld_appfixture_minesweeper_simple_menu_hover_toggle_frame_cycle",
    100,
    [&]()
    {
      inside = !inside;
      input().mouse_move(inside ? hover : InputSimulator::Coord{2, 2});
      frame_cycle();
    });
}

TEST_F(AppFixtureBenchmark, minesweeper_simple_open_popup_frame_cycle)
{
  ASSERT_NO_THROW(load_minesweeper_simple_windowed());
  render_cycle();

  auto menu_item = first_mode("menu-item");
  ASSERT_NE(menu_item, nullptr);
  const auto center = center_of(menu_item);
  input().mouse_down(center);
  update_cycle();
  input().mouse_up(center);
  update_cycle();
  clear_render_ops();

  benchmark_frame_cycle("realworld_appfixture_minesweeper_simple_open_popup_frame_cycle",
                        100);
}

TEST_F(AppFixtureBenchmark, minesweeper_simple_board_button_press_frame_cycle)
{
  ASSERT_NO_THROW(load_minesweeper_simple_windowed());
  render_cycle();

  auto board_buttons = descendant_modes("board-button");
  ASSERT_GT(board_buttons.size(), 20u);
  const auto target = center_of(board_buttons[board_buttons.size() / 2]);
  clear_render_ops();

  benchmark_update_flow(
    "realworld_appfixture_minesweeper_simple_board_button_press_frame_cycle",
    100,
    [&]() { click_at(target); });
}

TEST_F(AppFixtureBenchmark, tilemap_editor_tile_swatch_toggle_frame_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_current());
  render_cycle();

  auto tile_swatches = descendant_modes("tile-swatch");
  ASSERT_GT(tile_swatches.size(), 1u);
  bool second = false;
  clear_render_ops();

  benchmark_update_flow("realworld_appfixture_tilemap_editor_tile_swatch_toggle_frame_cycle",
                        50,
                        [&]()
                        {
                          second = !second;
                          auto target = tile_swatches[second ? 1 : 0];
                          const auto center = center_of(target);
                          input().mouse_down(center);
                          frame_cycle();
                          input().mouse_up(center);
                          frame_cycle();
                        });
}

TEST_F(AppFixtureBenchmark, tilemap_editor_canvas_paint_frame_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_current());
  render_cycle();

  auto canvas = first_mode("map-canvas");
  ASSERT_NE(canvas, nullptr);
  const auto canvas_center = center_of(canvas);
  bool right_button = false;
  clear_render_ops();

  benchmark_update_flow(
    "realworld_appfixture_tilemap_editor_canvas_paint_frame_cycle",
    50,
    [&]()
    {
      right_button = !right_button;
      click_at(canvas_center, right_button ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT);
    });
}

TEST_F(AppFixtureBenchmark, tilemap_editor_layer_row_select_frame_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_layered());
  render_cycle();

  auto layer_rows = descendant_modes("layer-row");
  ASSERT_GT(layer_rows.size(), 2u);
  const auto first = center_of(layer_rows[1]);
  const auto second = center_of(layer_rows[2]);
  bool choose_second = false;
  clear_render_ops();

  benchmark_update_flow("realworld_appfixture_tilemap_editor_layer_row_select_frame_cycle",
                        50,
                        [&]()
                        {
                          choose_second = !choose_second;
                          click_at(choose_second ? second : first);
                        });
}

TEST_F(AppFixtureBenchmark, tilemap_editor_layer_visibility_toggle_frame_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_layered());
  render_cycle();

  auto toggles = descendant_modes("layer-visibility-toggle");
  ASSERT_GT(toggles.size(), 1u);
  const auto first = center_of(toggles[0]);
  const auto second = center_of(toggles[1]);
  bool choose_second = false;
  clear_render_ops();

  benchmark_update_flow(
    "realworld_appfixture_tilemap_editor_layer_visibility_toggle_frame_cycle",
    50,
    [&]()
    {
      choose_second = !choose_second;
      click_at(choose_second ? second : first);
    });
}

TEST_F(AppFixtureBenchmark, tilemap_editor_delete_layer_dialog_frame_cycle)
{
  ASSERT_NO_THROW(load_tilemap_editor_current());
  render_cycle();

  auto layer_rows = descendant_modes("layer-row");
  ASSERT_FALSE(layer_rows.empty());
  right_click_at(center_of(layer_rows.front()));
  render_cycle();

  auto menu_items = descendant_modes("ui/popup-menu-item");
  ASSERT_FALSE(menu_items.empty());
  click_at(center_of(menu_items.back()));
  frame_cycle();
  render_cycle();

  ASSERT_NE(first_mode("layer-confirm-dialog-body"), nullptr);
  clear_render_ops();

  benchmark_frame_cycle(
    "realworld_appfixture_tilemap_editor_delete_layer_dialog_frame_cycle",
    50);
}
