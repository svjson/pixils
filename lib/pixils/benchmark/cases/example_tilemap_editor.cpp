#include "../support/benchmark.h"
#include "render_fixture.h"
#include <pixils/runtime/view.h>

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <memory>
#include <sdl2_mock/mock_resources.h>
#include <string>
#include <system_error>
#include <vector>

namespace
{
  using View = Pixils::Runtime::View;

  std::string lisp_string(const std::string& value)
  {
    std::string out = "\"";
    for (char c : value)
    {
      if (c == '\\' || c == '"') out.push_back('\\');
      out.push_back(c);
    }
    out.push_back('"');
    return out;
  }

  std::shared_ptr<View> find_first_mode(const std::shared_ptr<View>& view,
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

  void find_descendant_modes(const std::shared_ptr<View>& view,
                             const std::string& mode_name,
                             std::vector<std::shared_ptr<View>>& out)
  {
    if (!view) return;
    if (view->mode && view->mode->name == mode_name) out.push_back(view);
    for (const auto& child : view->children)
    {
      find_descendant_modes(child, mode_name, out);
    }
  }

  void read_tilemap_editor_sources(Roo::Runtime& runtime)
  {
    runtime.read_file("examples/tilemap-editor/src/assets.roo");
    runtime.read_file("examples/tilemap-editor/src/model/data.roo");
    runtime.read_file("examples/tilemap-editor/src/model/tilemap.roo");
    runtime.read_file("examples/tilemap-editor/src/io/project.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/renderer.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/canvas.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/inspector.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/palette.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/controls.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tilemap/layout.roo");
    runtime.read_file("examples/tilemap-editor/src/view/resources/model.roo");
    runtime.read_file("examples/tilemap-editor/src/view/resources/dialogs.roo");
    runtime.read_file("examples/tilemap-editor/src/view/resources/panels.roo");
    runtime.read_file("examples/tilemap-editor/src/view/resources/layout.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tileset/model.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tileset/panels.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tileset/dialogs.roo");
    runtime.read_file("examples/tilemap-editor/src/view/tileset/layout.roo");
    runtime.read_file("examples/tilemap-editor/src/view/theme.roo");
    runtime.read_file("examples/tilemap-editor/src/root.roo");
  }

  class ExampleTilemapEditorBenchmark : public RenderFixture
  {
   protected:
    ExampleTilemapEditorBenchmark() { render_ctx.buffer_dim = {800, 600}; }

    void TearDown() override
    {
      std::error_code ec;
      for (const auto& path : cleanup_paths)
      {
        std::filesystem::remove(path, ec);
      }
      RenderFixture::TearDown();
    }

    void load_example_map_project()
    {
      const auto history_path = std::filesystem::temp_directory_path() /
                                "pixils-benchmark-example-tilemap-history.edn";
      cleanup_paths.push_back(history_path);
      std::error_code ec;
      std::filesystem::remove(history_path, ec);

      load_main_mode();
      set_project_history_path(history_path);
      simulate_open_project_file("examples/tilemap-editor/example-maps/map1.edn",
                                 "examples/tilemap-editor/example-maps",
                                 "map1.edn");
    }

    void load_resource_project()
    {
      const auto history_path = std::filesystem::temp_directory_path() /
                                "pixils-benchmark-resource-tilemap-history.edn";
      const auto project_path = std::filesystem::temp_directory_path() /
                                "pixils-benchmark-resource-tilemap-project.edn";
      cleanup_paths.push_back(history_path);
      cleanup_paths.push_back(project_path);

      std::error_code ec;
      std::filesystem::remove(history_path, ec);
      std::filesystem::remove(project_path, ec);

      {
        std::ofstream out(project_path);
        out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {:editor-assets
                       {:images {:spritesheet
                                 {:file-name "assets/simples_pimples.png"}}}
                       :project-assets
                       {:images {:spritesheet
                                 {:file-name "examples/tilemap-editor/assets/simples_pimples.png"
                                  :name "Spritesheet"}}}}}
 :tilesets [{:id :loaded
             :label "Loaded"
             :tile-size 16
             :tiles [{:id :sprite
                      :name "Sprite"
                      :char "s"
                      :type :sprite
                      :image :project-assets/spritesheet
                      :source {:x 0 :y 0 :w 16 :h 16}}]}]
 :tilemap {:width 2
           :height 2
           :tile-size 16
           :layers []}})";
      }

      load_main_mode();
      set_project_history_path(history_path);
      simulate_open_project_file(project_path.string(),
                                 project_path.parent_path().string(),
                                 "resource-project.edn");
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
              session.active_mode ? session.active_mode->children.size() : 0);
            clear_render_ops();
          });
    }

    void clear_render_ops() { render_target()->render_ops.clear(); }

    std::shared_ptr<View> first_mode(const std::string& mode_name)
    {
      return find_first_mode(session.active_mode, mode_name);
    }

    std::vector<std::shared_ptr<View>> descendant_modes(const std::string& mode_name)
    {
      std::vector<std::shared_ptr<View>> matches;
      find_descendant_modes(session.active_mode, mode_name, matches);
      return matches;
    }

    std::shared_ptr<View> tab_at(std::size_t index)
    {
      if (!session.active_mode || session.active_mode->children.size() < 2)
      {
        ADD_FAILURE() << "Tilemap editor main mode has no tab panel";
        return nullptr;
      }
      const auto& tab_panel = session.active_mode->children[1];
      if (!tab_panel || tab_panel->children.empty())
      {
        ADD_FAILURE() << "Tilemap editor tab panel has no tab strip";
        return nullptr;
      }
      const auto& tab_strip = tab_panel->children[0];
      if (!tab_strip || tab_strip->children.size() <= index)
      {
        ADD_FAILURE() << "Tilemap editor tab strip is missing tab " << index;
        return nullptr;
      }
      return tab_strip->children[index];
    }

    InputSimulator::Coord center_of(const std::shared_ptr<View>& view)
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

    void click_tab(std::size_t index)
    {
      auto tab = tab_at(index);
      ASSERT_NE(tab, nullptr);
      click_at(center_of(tab));
    }

    void select_context_menu_action(const std::string& payload)
    {
      session.pop_mode(runtime.eval(payload));
      frame_cycle();
      frame_cycle();
    }

   private:
    std::vector<std::filesystem::path> cleanup_paths;

    void load_main_mode()
    {
      read_tilemap_editor_sources(runtime);
      session.push_mode("main-mode", Roo::Constant::NIL);
      frame_cycle();
      clear_render_ops();
    }

    void set_project_history_path(const std::filesystem::path& history_path)
    {
      ASSERT_NE(session.active_mode, nullptr);
      Roo::Dict::set_property(session.active_mode->state,
                                 Roo::keyword("project-history-path"),
                                 Roo::string(history_path.string()));
      Roo::Dict::set_property(session.active_mode->state,
                                 Roo::keyword("recent-projects"),
                                 Roo::vector({}));
    }

    void simulate_open_project_file(const std::string& path,
                                    const std::string& directory,
                                    const std::string& filename)
    {
      auto origin = Roo::map({Roo::keyword("view"),
                                 Pixils::Script::ViewAdapter::make_ref(*session.active_mode),
                                 Roo::keyword("event"),
                                 Roo::keyword("project/file-dialog-result")});
      auto overrides = Roo::map({Roo::keyword("origin"), origin});
      session.push_mode("ui/tab-panel-empty", Roo::Constant::NIL, overrides);
      session.pop_mode(runtime.eval(R"({:type :confirm
                                      :mode :file-dialog/open
                                      :path )" +
                                    lisp_string(path) +
                                    R"(
                                      :directory )" +
                                    lisp_string(directory) +
                                    R"(
                                      :filename )" +
                                    lisp_string(filename) + R"(})"));

      frame_cycle();
      frame_cycle();
      clear_render_ops();
    }
  };
} // namespace

TEST_F(ExampleTilemapEditorBenchmark, loaded_project_tab_round_trip_frame_cycle)
{
  ASSERT_NO_THROW(load_example_map_project());

  auto map_tab = tab_at(0);
  auto tilesets_tab = tab_at(1);
  ASSERT_NE(map_tab, nullptr);
  ASSERT_NE(tilesets_tab, nullptr);

  const auto map_center = center_of(map_tab);
  const auto tilesets_center = center_of(tilesets_tab);
  bool show_tilesets = false;
  clear_render_ops();

  benchmark_update_flow(
    "realworld_example_tilemap_editor_loaded_project_tab_round_trip_frame_cycle",
    50,
    [&]()
    {
      show_tilesets = !show_tilesets;
      click_at(show_tilesets ? tilesets_center : map_center);
    });
}

TEST_F(ExampleTilemapEditorBenchmark, loaded_project_tileset_keyboard_frame_cycle)
{
  ASSERT_NO_THROW(load_example_map_project());
  click_tab(1);

  auto tile_grid = first_mode("tileset-tile-grid");
  ASSERT_NE(tile_grid, nullptr);
  click_at({tile_grid->bounds.x + 5, tile_grid->bounds.y + 5});

  bool right = false;
  clear_render_ops();

  benchmark_update_flow(
    "realworld_example_tilemap_editor_tileset_tile_grid_keyboard_frame_cycle",
    50,
    [&]()
    {
      const SDL_Keycode key = right ? SDLK_RIGHT : SDLK_LEFT;
      right = !right;
      input().key_down(key);
      frame_cycle();
      input().key_up(key);
      frame_cycle();
    });
}

TEST_F(ExampleTilemapEditorBenchmark, resource_tab_frame_cycle)
{
  ASSERT_NO_THROW(load_resource_project());
  click_tab(2);

  ASSERT_EQ(descendant_modes("resource-bundle-list").size(), 1u);
  ASSERT_EQ(descendant_modes("resource-bundle-row").size(), 1u);
  clear_render_ops();

  benchmark_frame_cycle("realworld_example_tilemap_editor_resource_tab_frame_cycle", 50);
}

TEST_F(ExampleTilemapEditorBenchmark, resource_tileset_tab_switch_frame_cycle)
{
  ASSERT_NO_THROW(load_resource_project());

  auto tilesets_tab = tab_at(1);
  auto resources_tab = tab_at(2);
  ASSERT_NE(tilesets_tab, nullptr);
  ASSERT_NE(resources_tab, nullptr);

  const auto tilesets_center = center_of(tilesets_tab);
  const auto resources_center = center_of(resources_tab);
  bool show_resources = false;
  clear_render_ops();

  benchmark_update_flow(
    "realworld_example_tilemap_editor_resource_tileset_tab_switch_frame_cycle",
    50,
    [&]()
    {
      show_resources = !show_resources;
      click_at(show_resources ? resources_center : tilesets_center);
    });
}

TEST_F(ExampleTilemapEditorBenchmark, resource_bundle_delete_dialog_frame_cycle)
{
  ASSERT_NO_THROW(load_resource_project());
  click_tab(2);

  auto rows = descendant_modes("resource-bundle-row");
  ASSERT_EQ(rows.size(), 1u);
  click_at(center_of(rows.front()), SDL_BUTTON_RIGHT);
  render_cycle();

  auto menu_items = descendant_modes("ui/popup-menu-item");
  ASSERT_GE(menu_items.size(), 1u);
  select_context_menu_action("{:type :action :action :resource/context-dummy "
                             ":payload {:kind :bundle :command :delete}}");
  render_cycle();

  ASSERT_NE(first_mode("ui/dialog-frame"), nullptr);
  clear_render_ops();

  benchmark_frame_cycle(
    "realworld_example_tilemap_editor_resource_bundle_delete_dialog_frame_cycle",
    50);
}

TEST_F(ExampleTilemapEditorBenchmark, tile_import_dialog_frame_cycle)
{
  ASSERT_NO_THROW(load_resource_project());
  click_tab(1);

  auto add_button = first_mode("tile-add-menu-button");
  ASSERT_NE(add_button, nullptr);
  click_at(center_of(add_button));
  render_cycle();

  auto menu_items = descendant_modes("ui/popup-menu-item");
  ASSERT_GE(menu_items.size(), 1u);
  select_context_menu_action("{:type :action :action :tile/add-menu-dummy "
                             ":payload {:command :from-resource}}");
  render_cycle();

  ASSERT_NE(first_mode("tile-import-dialog-body"), nullptr);
  clear_render_ops();

  benchmark_frame_cycle("realworld_example_tilemap_editor_tile_import_dialog_frame_cycle",
                        50);
}
