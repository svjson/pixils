#include "tilemap_editor_test_support.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>

TEST_F(TilemapEditorStartupTest, loaded_project_brush_tab_lists_layer_tilesets)
{
  const auto history_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-brush-history.edn";
  const auto project_path =
    std::filesystem::temp_directory_path() / "pixils-tilemap-editor-brush-project.edn";
  std::error_code ec;
  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);

  {
    std::ofstream out(project_path);
    out << R"({:format :pixils.tilemap-editor/project
 :version 1
 :resources {:bundles {}}
 :tilesets [{:id :loaded
             :label "Loaded"
             :tile-size 16
             :tiles [{:id :grass
                      :name "Grass"
                      :char "g"
                      :type :color
                      :color {:r 0 :g 255 :b 0}}]}]
 :brushes [{:id :brush
            :label "Brush"
            :kind :pattern
            :pattern {:size {:w 3 :h 3}
                      :anchor {:x 1 :y 1}
                      :layers []}}]
 :tilemap {:width 2
           :height 2
           :tile-size 16
           :layers []}})";
  }

  read_tilemap_editor_sources(runtime);

  session.push_mode("main-mode", Lisple::Constant::NIL);
  update_cycle();
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("project-history-path"),
                             Lisple::string(history_path.string()));
  Lisple::Dict::set_property(session.active_mode->state,
                             Lisple::keyword("recent-projects"),
                             Lisple::vector({}));

  auto origin = Lisple::map({Lisple::keyword("view"),
                             Pixils::Script::ViewAdapter::make_ref(*session.active_mode),
                             Lisple::keyword("event"),
                             Lisple::keyword("project/file-dialog-result")});
  auto overrides = Lisple::map({Lisple::keyword("origin"), origin});
  session.push_mode("ui/tab-panel-empty", Lisple::Constant::NIL, overrides);
  session.pop_mode(runtime.eval(R"({:type :confirm
                                  :mode :file-dialog/open
                                  :path )" +
                                lisp_string(project_path.string()) + R"(
                                  :directory )" +
                                lisp_string(project_path.parent_path().string()) + R"(
                                  :filename "brush-project.edn"})"));

  update_cycle();
  update_cycle();
  session.render_mode();

  auto tab_panel = session.active_mode->children[1];
  ASSERT_NE(tab_panel, nullptr);
  auto tab_strip = tab_panel->children[0];
  ASSERT_GE(tab_strip->children.size(), 3u);

  auto brushes_tab = tab_strip->children[2];
  ASSERT_NE(brushes_tab, nullptr);
  input().mouse_down({brushes_tab->bounds.x + brushes_tab->bounds.w / 2,
                      brushes_tab->bounds.y + brushes_tab->bounds.h / 2});
  update_cycle();
  input().mouse_up({brushes_tab->bounds.x + brushes_tab->bounds.w / 2,
                    brushes_tab->bounds.y + brushes_tab->bounds.h / 2});
  update_cycle();
  update_cycle();
  session.render_mode();

  std::vector<std::shared_ptr<Pixils::Runtime::View>> selectors;
  find_descendant_modes(session.active_mode, "pattern-layer-tileset-selector", selectors);
  ASSERT_EQ(selectors.size(), 1u);
  EXPECT_NE(selectors[0]->state->to_string().find(":loaded"), std::string::npos);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> combos;
  find_descendant_modes(session.active_mode, "ui/combo-box", combos);
  ASSERT_GE(combos.size(), 1u);
  EXPECT_NE(combos[0]->state->to_string().find("Loaded"), std::string::npos);
  EXPECT_NE(combos[0]->state->to_string().find(":disabled? false"), std::string::npos);

  std::filesystem::remove(history_path, ec);
  std::filesystem::remove(project_path, ec);
}
