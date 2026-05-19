#include "../support/benchmark.h"

#include "appfixture/composable_app_session_fixture.h"
#include "appfixture/minesweeper_app_manifest.h"
#include "appfixture/tilemap_editor_app_manifest.h"

#include <gtest/gtest.h>
#include <sdl2_mock/mock_resources.h>

#include <cstdint>

namespace
{
  namespace Minesweeper = Pixils::Test::AppFixture::Minesweeper;
  namespace TilemapEditor = Pixils::Test::AppFixture::TilemapEditor;

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
               {{TilemapEditor::spritesheet_asset_path(),
                 "assets/simples_pimples.png"}});
      load_program();
      warmup_frame_cycles(2);
    }

    void load_tilemap_editor_layered()
    {
      set_frame_size({1200, 700});
      load_app(TilemapEditor::layered_manifest(),
               TilemapEditor::main_namespace(),
               TilemapEditor::layered_entry_files(),
               {{TilemapEditor::spritesheet_asset_path(),
                 "assets/simples_pimples.png"}});
      load_program();
      warmup_frame_cycles(2);
    }

    void clear_render_ops()
    {
      render_target()->render_ops.clear();
    }

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
