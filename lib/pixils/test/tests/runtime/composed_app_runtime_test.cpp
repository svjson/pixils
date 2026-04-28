#include "../appfixture/app_manifest.h"
#include "../appfixture/composable_app_session_fixture.h"
#include "../appfixture/minesweeper_app_manifest.h"
#include <pixils/runtime/view.h>

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>

namespace AppFixture = Pixils::Test::AppFixture;
using Pixils::Runtime::View;

namespace
{
  AppFixture::SourceUnit inline_unit(const std::string& id,
                                     const std::vector<std::string>& require_entries,
                                     const std::vector<std::string>& body)
  {
    return AppFixture::SourceUnit::inline_unit(id, require_entries, body);
  }

  AppFixture::AppManifest bootstrap_manifest()
  {
    return AppFixture::AppManifest(
      {inline_unit("main-api", {}, {"(def answer 42)"})},
      {AppFixture::ManifestFile{.id = "main",
                                .disk_path = "pixils/test/app/main.lisple",
                                .namespace_name = "pixils.test.app.main",
                                .unit_ids = {"main-api"}}});
  }

  View& only_child(View& view, size_t index)
  {
    EXPECT_GT(view.children.size(), index);
    return *view.children.at(index);
  }

  void expect_mode_name(const View& view, const std::string& expected_mode_name)
  {
    ASSERT_NE(view.mode, nullptr);
    EXPECT_EQ(view.mode->name, expected_mode_name);
  }

  View& child_with_mode_name(View& view, const std::string& expected_mode_name)
  {
    for (auto& child : view.children)
    {
      if (child && child->mode && child->mode->name == expected_mode_name) return *child;
    }

    ADD_FAILURE() << "Expected child view with mode '" << expected_mode_name << "'";
    return *view.children.at(0);
  }

  Lisple::sptr_rtval get_key(const Lisple::sptr_rtval& target, const std::string& key)
  {
    return Lisple::Dict::get_property(target, Lisple::RTValue::keyword(key));
  }

  Lisple::sptr_rtval get_index(const Lisple::sptr_rtval& target, size_t index)
  {
    if (!target) return nullptr;
    return Lisple::get_child(*target, index);
  }

  void expect_nil_key(const Lisple::sptr_rtval& target, const std::string& key)
  {
    auto value = get_key(target, key);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->to_string(), "nil");
  }

  void expect_key_string(const Lisple::sptr_rtval& target,
                         const std::string& key,
                         const std::string& expected_value)
  {
    auto value = get_key(target, key);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->to_string(), expected_value);
  }

  void expect_int_key(const Lisple::sptr_rtval& target,
                      const std::string& key,
                      int expected_value)
  {
    auto value = get_key(target, key);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->num().get_int(), expected_value);
  }

  int count_flagged_cells(const Lisple::sptr_rtval& board_mask)
  {
    if (!board_mask) return 0;

    int flagged = 0;
    for (size_t row_idx = 0; row_idx < board_mask->elements().size(); row_idx++)
    {
      auto row = get_index(board_mask, row_idx);
      if (!row) continue;

      for (size_t col_idx = 0; col_idx < row->elements().size(); col_idx++)
      {
        auto cell = get_index(row, col_idx);
        auto flagged_value = get_key(cell, "flagged?");
        if (flagged_value && flagged_value->to_string() == "true") flagged++;
      }
    }

    return flagged;
  }
} // namespace

class ComposedAppRuntimeTest : public ComposableAppSessionFixture
{
 protected:
  void load_bootstrap_app()
  {
    load_app(bootstrap_manifest(), "pixils.test.app.main", {"pixils/test/app/main.lisple"});
    EXPECT_EQ(eval("pixils.test.app.main/answer")->num().get_int(), 42);
  }

  void load_minesweeper_app()
  {
    load_app(AppFixture::Minesweeper::default_manifest(),
             AppFixture::Minesweeper::main_namespace(),
             AppFixture::Minesweeper::entry_files());
  }

  void use_default_frame_size() { set_frame_size({320, 200}); }
};

TEST_F(ComposedAppRuntimeTest, session_builds_expected_initial_view_tree_for_composed_app)
{
  // Given
  load_bootstrap_app();

  pixils().eval(R"(
    (pixils/defmode top-panel {})
    (pixils/defmode body-panel {})
    (pixils/defmode root-mode {
      :children [{:mode 'top-panel}
                 {:mode 'body-panel}]
    })
  )");

  // When
  session().push_mode("root-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& root = *session().active_mode;
  expect_mode_name(root, "root-mode");
  ASSERT_EQ(root.children.size(), 2u);

  View& top_panel = only_child(root, 0);
  expect_mode_name(top_panel, "top-panel");

  View& body_panel = only_child(root, 1);
  expect_mode_name(body_panel, "body-panel");
}

TEST_F(ComposedAppRuntimeTest, session_assigns_expected_bounds_for_explicit_viewport)
{
  // Given
  set_frame_size({320, 200});
  load_bootstrap_app();

  pixils().eval(R"(
    (pixils/defmode top-panel {
      :style {:height 40}
    })
    (pixils/defmode body-panel {})
    (pixils/defmode root-mode {
      :children [{:mode 'top-panel}
                 {:mode 'body-panel}]
    })
  )");

  ASSERT_EQ(frame_size(), (Pixils::Dimension{320, 200}));

  // When
  session().push_mode("root-mode", Lisple::Constant::NIL);
  render_cycle();

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& root = *session().active_mode;
  View& top_panel = only_child(root, 0);
  View& body_panel = only_child(root, 1);

  EXPECT_EQ(root.bounds, (Pixils::Rect{0, 0, 320, 200}));
  EXPECT_EQ(top_panel.bounds, (Pixils::Rect{0, 0, 320, 40}));
  EXPECT_EQ(body_panel.bounds, (Pixils::Rect{0, 40, 320, 160}));
}

TEST_F(ComposedAppRuntimeTest, session_builds_expected_initial_view_tree_for_default_fixture)
{
  // Given
  load_minesweeper_app();

  // When
  session().push_mode("window-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& window = *session().active_mode;
  expect_mode_name(window, "window-mode");
  ASSERT_EQ(window.children.size(), 2u);

  View& menu_bar = only_child(window, 0);
  expect_mode_name(menu_bar, "menu-bar-mode");

  View& game_layout = only_child(window, 1);
  expect_mode_name(game_layout, "game-layout-mode");
  ASSERT_EQ(game_layout.children.size(), 2u);

  View& status_panel = only_child(game_layout, 0);
  expect_mode_name(status_panel, "status-panel");
  ASSERT_EQ(status_panel.children.size(), 3u);
  expect_mode_name(*status_panel.children.at(0), "counter");
  expect_mode_name(*status_panel.children.at(1), "button");
  expect_mode_name(*status_panel.children.at(2), "counter");

  View& board_mode = only_child(game_layout, 1);
  expect_mode_name(board_mode, "board-mode");
  ASSERT_EQ(board_mode.children.size(), 2u);
  expect_mode_name(*board_mode.children.at(0), "mine-layer-mode");
  expect_mode_name(*board_mode.children.at(1), "board-buttons");
}

TEST_F(ComposedAppRuntimeTest,
       session_initializes_expected_root_state_for_default_fixture_game_layout)
{
  // Given
  load_minesweeper_app();

  // When
  session().push_mode("window-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& window = *session().active_mode;
  View& game_layout = child_with_mode_name(window, "game-layout-mode");

  ASSERT_NE(game_layout.state, nullptr);
  auto settings = get_key(game_layout.state, "settings");
  auto counters = get_key(game_layout.state, "counters");
  auto face = get_key(game_layout.state, "face");
  auto board = get_key(game_layout.state, "board");
  auto board_mask = get_key(game_layout.state, "board-mask");
  ASSERT_NE(settings, nullptr);
  ASSERT_NE(counters, nullptr);
  ASSERT_NE(face, nullptr);
  ASSERT_NE(board, nullptr);
  ASSERT_NE(board_mask, nullptr);
  expect_int_key(settings, "w", 9);
  expect_int_key(settings, "h", 9);
  expect_int_key(settings, "mines", 10);
  expect_int_key(counters, "mines-left", 10);
  expect_int_key(counters, "timer", 0);
  EXPECT_EQ(face->to_string(), ":status-panel/face-normal");
  EXPECT_EQ(board->elements().size(), 9u);
  EXPECT_EQ(board_mask->elements().size(), 9u);
}

TEST_F(ComposedAppRuntimeTest, session_projects_bound_state_into_status_panel_view)
{
  // Given
  load_minesweeper_app();

  // When
  session().push_mode("window-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& window = *session().active_mode;
  View& game_layout = child_with_mode_name(window, "game-layout-mode");
  View& status_panel = child_with_mode_name(game_layout, "status-panel");

  ASSERT_NE(status_panel.state, nullptr);
  expect_key_string(status_panel.state, "face", ":status-panel/face-normal");
  ASSERT_NE(get_key(status_panel.state, "counters"), nullptr);
  expect_nil_key(status_panel.state, "board");
  expect_nil_key(status_panel.state, "board-mask");
}

TEST_F(ComposedAppRuntimeTest, session_projects_bound_state_into_board_subtree_views)
{
  // Given
  load_minesweeper_app();

  // When
  session().push_mode("window-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& window = *session().active_mode;
  View& game_layout = child_with_mode_name(window, "game-layout-mode");
  View& board_mode = child_with_mode_name(game_layout, "board-mode");
  View& mine_layer = child_with_mode_name(board_mode, "mine-layer-mode");
  View& board_buttons = child_with_mode_name(board_mode, "board-buttons");

  ASSERT_NE(board_mode.state, nullptr);
  ASSERT_NE(get_key(board_mode.state, "board"), nullptr);
  ASSERT_NE(get_key(board_mode.state, "board-mask"), nullptr);
  expect_key_string(board_mode.state, "face", ":status-panel/face-normal");
  expect_nil_key(board_mode.state, "counters");

  ASSERT_NE(mine_layer.state, nullptr);
  ASSERT_NE(get_key(mine_layer.state, "board"), nullptr);
  ASSERT_NE(get_key(mine_layer.state, "board-mask"), nullptr);

  ASSERT_NE(board_buttons.state, nullptr);
  ASSERT_NE(get_key(board_buttons.state, "board-mask"), nullptr);
  expect_key_string(board_buttons.state, "face", ":status-panel/face-normal");
}

TEST_F(ComposedAppRuntimeTest, session_projects_bound_state_into_status_panel_leaf_views)
{
  // Given
  load_minesweeper_app();

  // When
  session().push_mode("window-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& window = *session().active_mode;
  View& game_layout = child_with_mode_name(window, "game-layout-mode");
  View& status_panel = child_with_mode_name(game_layout, "status-panel");

  View& mines_counter = *status_panel.children.at(0);
  View& face_button = *status_panel.children.at(1);
  View& timer_counter = *status_panel.children.at(2);
  expect_int_key(mines_counter.state, "value", 10);
  expect_int_key(timer_counter.state, "value", 0);
  EXPECT_EQ(get_key(face_button.state, "label")->str(), "Hehu");
  expect_key_string(face_button.state, "image", ":status-panel/face-normal");
}

TEST_F(ComposedAppRuntimeTest, session_assigns_expected_outer_bounds_for_default_fixture)
{
  // Given
  use_default_frame_size();
  load_minesweeper_app();

  ASSERT_EQ(frame_size(), (Pixils::Dimension{320, 200}));

  // When
  session().push_mode("window-mode", Lisple::Constant::NIL);
  render_cycle();

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  View& window = *session().active_mode;
  View& menu_bar = child_with_mode_name(window, "menu-bar-mode");
  View& game_layout = child_with_mode_name(window, "game-layout-mode");
  View& status_panel = child_with_mode_name(game_layout, "status-panel");
  View& board_mode = child_with_mode_name(game_layout, "board-mode");

  EXPECT_EQ(window.bounds, (Pixils::Rect{0, 0, 320, 200}));
  EXPECT_EQ(menu_bar.bounds, (Pixils::Rect{0, 0, 320, 18}));
  EXPECT_EQ(game_layout.bounds, (Pixils::Rect{0, 18, 320, 182}));
  EXPECT_EQ(status_panel.bounds, (Pixils::Rect{0, 18, 320, 50}));
  EXPECT_EQ(board_mode.bounds, (Pixils::Rect{0, 68, 320, 132}));
}

TEST_F(ComposedAppRuntimeTest, session_opens_popup_mode_from_menu_mouse_down)
{
  // Given
  use_default_frame_size();
  load_minesweeper_app();
  session().push_mode("window-mode", Lisple::Constant::NIL);
  render_cycle();

  ASSERT_NE(session().active_mode, nullptr);
  ASSERT_EQ(session().active_mode->mode->name, "window-mode");
  ASSERT_EQ(session().mode_stack.size(), 1u);

  // When
  input().mouse_down({5, 5});
  update_cycle();

  // Then
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "popup-menu-mode");
  EXPECT_EQ(session().mode_stack.size(), 2u);
  ASSERT_EQ(session().ctx_stack.size(), 1u);
  ASSERT_NE(session().ctx_stack.back(), nullptr);
  EXPECT_EQ(session().ctx_stack.back()->mode->name, "window-mode");
}
