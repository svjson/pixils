#include "composable_app_session_fixture.h"
#include "tilemap_editor_app_manifest.h"
#include <pixils/program.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_layout.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <sdl2_mock/mock_resources.h>
#include <vector>

namespace TilemapEditor = Pixils::Test::AppFixture::TilemapEditor;

class TilemapEditorAppFixtureTest : public ComposableAppSessionFixture
{
 protected:
  void load_tilemap_editor()
  {
    load_app(TilemapEditor::manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void load_layered_tilemap_editor()
  {
    load_app(TilemapEditor::layered_manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::layered_entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void load_current_tilemap_editor()
  {
    load_app(TilemapEditor::current_manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void load_layered_tilemap_editor_with_plain_layer_list()
  {
    load_app(TilemapEditor::layered_plain_layer_list_manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::layered_entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void load_layered_tilemap_editor_with_content_layer_list()
  {
    load_app(TilemapEditor::layered_content_layer_list_manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::layered_entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void load_layered_tilemap_editor_with_plain_overflow_layer_list()
  {
    load_app(TilemapEditor::layered_plain_overflow_layer_list_manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::layered_entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void load_layered_tilemap_editor_with_content_overflow_layer_list()
  {
    load_app(TilemapEditor::layered_content_overflow_layer_list_manifest(),
             TilemapEditor::main_namespace(),
             TilemapEditor::layered_entry_files(),
             {{TilemapEditor::spritesheet_asset_path(), "assets/simples_pimples.png"}});
  }

  void layout_active_mode()
  {
    Pixils::UI::layout_view_tree(session().active_mode,
                                 {0, 0, frame_size().w, frame_size().h},
                                 pixils(),
                                 session().hook_args.render_args[1]);
  }

  void update_and_layout()
  {
    update_cycle();
    layout_active_mode();
  }

  std::shared_ptr<Pixils::Runtime::View> find_first_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;

    for (const auto& child : view->children)
    {
      if (auto found = find_first_mode(child, mode_name)) return found;
    }

    return nullptr;
  }

  std::shared_ptr<Pixils::Runtime::View> find_descendant_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    for (const auto& child : view->children)
    {
      if (child && child->mode && child->mode->name == mode_name) return child;
      if (auto found = find_descendant_mode(child, mode_name)) return found;
    }
    return nullptr;
  }

  void find_descendant_modes(const std::shared_ptr<Pixils::Runtime::View>& view,
                             const std::string& mode_name,
                             std::vector<std::shared_ptr<Pixils::Runtime::View>>& out)
  {
    if (!view) return;
    for (const auto& child : view->children)
    {
      if (child && child->mode && child->mode->name == mode_name) out.push_back(child);
      find_descendant_modes(child, mode_name, out);
    }
  }

  bool has_fill_rect(const std::vector<RenderOperation>& ops, const SDL_Rect& rect)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::FILL_RECT && op.rendered_rect.x == rect.x &&
          op.rendered_rect.y == rect.y && op.rendered_rect.w == rect.w &&
          op.rendered_rect.h == rect.h)
      {
        return true;
      }

      if (has_fill_rect(op.sub_ops, rect)) return true;
    }

    return false;
  }

  SDL_Rect intersect_rect(const Pixils::Rect& a, const Pixils::Rect& b)
  {
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.w, b.x + b.w);
    int y2 = std::min(a.y + a.h, b.y + b.h);
    return SDL_Rect{x1, y1, std::max(0, x2 - x1), std::max(0, y2 - y1)};
  }

  void hover_last_layer_row()
  {
    auto layer_list = find_first_mode(session().active_mode, "layer-list");
    ASSERT_NE(layer_list, nullptr);

    std::vector<std::shared_ptr<Pixils::Runtime::View>> layer_rows;
    find_descendant_modes(layer_list, "layer-row", layer_rows);
    ASSERT_EQ(layer_rows.size(), 4u);

    auto content = layer_list->effective_style.content_rect(layer_list->bounds);
    auto last_row = layer_rows.back();
    ASSERT_NE(last_row, nullptr);
    ASSERT_GE(content.y + content.h - 1, last_row->bounds.y);
    ASSERT_LT(content.y + content.h - 1, last_row->bounds.y + last_row->bounds.h);

    input().mouse_move(last_row->bounds.x + 4, content.y + content.h - 1);
    ASSERT_NO_THROW(update_and_layout());
  }

  void expect_layer_list_border_rendered(bool use_content_class)
  {
    set_frame_size({1200, 700});
    if (use_content_class)
      load_layered_tilemap_editor_with_content_layer_list();
    else
      load_layered_tilemap_editor_with_plain_layer_list();
    load_program();

    ASSERT_NO_THROW(update_and_layout());
    ASSERT_NO_THROW(update_and_layout());

    auto layer_list = find_first_mode(session().active_mode, "layer-list");
    ASSERT_NE(layer_list, nullptr);
    EXPECT_EQ(layer_list->effective_style.clip.value_or(false), use_content_class);
    ASSERT_TRUE(layer_list->effective_style.border.has_value());

    ASSERT_NO_THROW(render_cycle());

    const auto& bounds = layer_list->bounds;
    EXPECT_TRUE(
      has_fill_rect(render_target()->render_ops, SDL_Rect{bounds.x, bounds.y, bounds.w, 1}));
    EXPECT_TRUE(has_fill_rect(render_target()->render_ops,
                              SDL_Rect{bounds.x + bounds.w - 1, bounds.y, 1, bounds.h}));
    EXPECT_TRUE(has_fill_rect(render_target()->render_ops,
                              SDL_Rect{bounds.x, bounds.y + bounds.h - 1, bounds.w, 1}));
    EXPECT_TRUE(
      has_fill_rect(render_target()->render_ops, SDL_Rect{bounds.x, bounds.y, 1, bounds.h}));
  }

  void expect_layer_list_hover_background(bool use_content_class, bool clipped)
  {
    set_frame_size({1200, 700});
    if (use_content_class)
      load_layered_tilemap_editor_with_content_overflow_layer_list();
    else
      load_layered_tilemap_editor_with_plain_overflow_layer_list();
    load_program();

    ASSERT_NO_THROW(update_and_layout());
    ASSERT_NO_THROW(update_and_layout());
    hover_last_layer_row();

    auto layer_list = find_first_mode(session().active_mode, "layer-list");
    ASSERT_NE(layer_list, nullptr);
    EXPECT_EQ(layer_list->effective_style.clip.value_or(false), clipped);

    std::vector<std::shared_ptr<Pixils::Runtime::View>> layer_rows;
    find_descendant_modes(layer_list, "layer-row", layer_rows);
    ASSERT_EQ(layer_rows.size(), 4u);
    auto last_row = layer_rows.back();
    ASSERT_NE(last_row, nullptr);
    ASSERT_TRUE(last_row->effective_style.background.has_value());

    auto content = layer_list->effective_style.content_rect(layer_list->bounds);
    ASSERT_GT(last_row->bounds.y + last_row->bounds.h, content.y + content.h);

    ASSERT_NO_THROW(render_cycle());

    SDL_Rect full_hover_background{last_row->bounds.x,
                                   last_row->bounds.y,
                                   last_row->bounds.w,
                                   last_row->bounds.h};
    SDL_Rect clipped_hover_background = intersect_rect(last_row->bounds, content);

    if (clipped)
    {
      EXPECT_FALSE(has_fill_rect(render_target()->render_ops, full_hover_background));
      EXPECT_TRUE(has_fill_rect(render_target()->render_ops, clipped_hover_background));
    }
    else
    {
      EXPECT_TRUE(has_fill_rect(render_target()->render_ops, full_hover_background));
    }
  }

  std::shared_ptr<Pixils::Runtime::View> find_body_containing_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/header-panel-body" &&
        find_descendant_mode(view, mode_name))
      return view;

    for (const auto& child : view->children)
    {
      if (auto found = find_body_containing_mode(child, mode_name)) return found;
    }
    return nullptr;
  }

  std::shared_ptr<Pixils::Runtime::View> find_header_panel_containing_mode(
    const std::shared_ptr<Pixils::Runtime::View>& view,
    const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/header-panel" &&
        find_descendant_mode(view, mode_name))
      return view;

    for (const auto& child : view->children)
    {
      if (auto found = find_header_panel_containing_mode(child, mode_name)) return found;
    }

    return nullptr;
  }
};

TEST_F(TilemapEditorAppFixtureTest, loads_program_and_lays_out_initial_frame)
{
  set_frame_size({800, 500});
  load_tilemap_editor();

  Pixils::Program& program = load_program();

  EXPECT_EQ(program.get_name(), "tilemap-editor");
  EXPECT_EQ(program.initial_mode, "main-mode");
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "main-mode");

  ASSERT_NO_THROW(update_cycle());
  ASSERT_NO_THROW(layout_active_mode());
  EXPECT_GT(session().active_mode->bounds.w, 0);
  EXPECT_GT(session().active_mode->bounds.h, 0);
}

TEST_F(TilemapEditorAppFixtureTest,
       layered_manifest_loads_program_and_lays_out_initial_frame)
{
  set_frame_size({1200, 700});
  load_layered_tilemap_editor();

  Pixils::Program& program = load_program();

  EXPECT_EQ(program.get_name(), "tilemap-editor");
  EXPECT_EQ(program.initial_mode, "main-mode");
  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "main-mode");

  ASSERT_NO_THROW(update_cycle());
  ASSERT_NO_THROW(layout_active_mode());
  EXPECT_GT(session().active_mode->bounds.w, 0);
  EXPECT_GT(session().active_mode->bounds.h, 0);
}

TEST_F(TilemapEditorAppFixtureTest, layered_layer_list_clips_partially_visible_layer_rows)
{
  set_frame_size({1200, 700});
  load_layered_tilemap_editor_with_content_overflow_layer_list();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  auto layer_list = find_first_mode(session().active_mode, "layer-list");
  ASSERT_NE(layer_list, nullptr);

  ASSERT_TRUE(layer_list->effective_style.clip.has_value());
  EXPECT_TRUE(*layer_list->effective_style.clip);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> layer_rows;
  find_descendant_modes(layer_list, "layer-row", layer_rows);
  ASSERT_EQ(layer_rows.size(), 4u);

  auto content = layer_list->effective_style.content_rect(layer_list->bounds);
  auto last_row = layer_rows.back();
  ASSERT_NE(last_row, nullptr);
  EXPECT_GT(last_row->bounds.y + last_row->bounds.h, content.y + content.h);
}

TEST_F(TilemapEditorAppFixtureTest, layered_plain_layer_list_renders_content_border)
{
  expect_layer_list_border_rendered(false);
}

TEST_F(TilemapEditorAppFixtureTest, layered_content_layer_list_renders_content_border)
{
  expect_layer_list_border_rendered(true);
}

TEST_F(TilemapEditorAppFixtureTest,
       layered_plain_layer_list_hover_background_draws_outside_parent_boundary)
{
  expect_layer_list_hover_background(false, false);
}

TEST_F(TilemapEditorAppFixtureTest,
       layered_content_layer_list_hover_background_clips_to_parent_boundary)
{
  expect_layer_list_hover_background(true, true);
}

TEST_F(TilemapEditorAppFixtureTest, initial_layout_keeps_workspace_and_side_panel_visible)
{
  set_frame_size({800, 500});
  load_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  ASSERT_NE(session().active_mode, nullptr);
  ASSERT_EQ(session().active_mode->children.size(), 2u);
  auto workspace = session().active_mode->children[1];
  ASSERT_NE(workspace, nullptr);
  ASSERT_EQ(workspace->children.size(), 2u);

  auto map_area = workspace->children[0];
  auto side_panel = workspace->children[1];
  ASSERT_NE(map_area, nullptr);
  ASSERT_NE(side_panel, nullptr);

  EXPECT_GT(workspace->bounds.w, 0);
  EXPECT_GT(workspace->bounds.h, 0);
  EXPECT_GT(map_area->bounds.w, 0);
  EXPECT_GT(map_area->bounds.h, 0);
  EXPECT_GT(side_panel->bounds.w, 0);
  EXPECT_GT(side_panel->bounds.h, 0);
  EXPECT_GT(side_panel->bounds.x, map_area->bounds.x);
  EXPECT_LE(side_panel->bounds.x + side_panel->bounds.w,
            workspace->bounds.x + workspace->bounds.w);
}

TEST_F(TilemapEditorAppFixtureTest, toolbar_checkbox_is_laid_out_inside_toolbar)
{
  set_frame_size({800, 500});
  load_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  auto checkbox = find_first_mode(session().active_mode, "ui/checkbox");
  auto checkbox_box = find_first_mode(session().active_mode, "ui/checkbox-box");
  ASSERT_NE(checkbox, nullptr);
  ASSERT_NE(checkbox_box, nullptr);

  EXPECT_GT(checkbox->bounds.w, 0);
  EXPECT_GT(checkbox->bounds.h, 0);
  EXPECT_GT(checkbox_box->bounds.w, 0);
  EXPECT_GT(checkbox_box->bounds.h, 0);
  EXPECT_GE(checkbox->bounds.x, session().active_mode->bounds.x);
  EXPECT_GE(checkbox->bounds.y, session().active_mode->bounds.y);
  EXPECT_GT(checkbox_box->bounds.x, session().active_mode->bounds.x);
  EXPECT_GT(checkbox_box->bounds.y, session().active_mode->bounds.y);
}

TEST_F(TilemapEditorAppFixtureTest, tile_palette_scroll_pane_stays_inside_panel_body)
{
  set_frame_size({800, 500});
  load_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  auto panel_body = find_body_containing_mode(session().active_mode, "ui/scroll-pane");
  auto panel = find_header_panel_containing_mode(session().active_mode, "ui/scroll-pane");
  ASSERT_NE(panel, nullptr);
  ASSERT_NE(panel_body, nullptr);
  auto scroll_pane = find_descendant_mode(panel_body, "ui/scroll-pane");
  ASSERT_NE(scroll_pane, nullptr);

  EXPECT_LE(panel_body->bounds.x + panel_body->bounds.w, panel->bounds.x + panel->bounds.w);
  EXPECT_LE(panel_body->bounds.y + panel_body->bounds.h, panel->bounds.y + panel->bounds.h);
  EXPECT_GE(scroll_pane->bounds.x, panel_body->bounds.x);
  EXPECT_GE(scroll_pane->bounds.y, panel_body->bounds.y);
  EXPECT_LE(scroll_pane->bounds.x + scroll_pane->bounds.w,
            panel_body->bounds.x + panel_body->bounds.w);
  EXPECT_LE(scroll_pane->bounds.y + scroll_pane->bounds.h,
            panel_body->bounds.y + panel_body->bounds.h);
}

TEST_F(TilemapEditorAppFixtureTest,
       current_layer_name_dialog_shrink_height_contains_buttons)
{
  set_frame_size({1200, 700});
  ASSERT_NO_THROW(load_current_tilemap_editor());
  ASSERT_NO_THROW(load_program());

  session().push_mode("layer-name-dialog-modal",
                      eval("{:operation :rename "
                           " :index 0 "
                           " :title \"Rename Layer\" "
                           " :name \"Terrain\"}"));

  ASSERT_NO_THROW(layout_active_mode());

  ASSERT_NE(session().active_mode, nullptr);
  EXPECT_EQ(session().active_mode->mode->name, "layer-name-dialog-modal");

  auto dialog_body = find_first_mode(session().active_mode, "layer-name-dialog-body");
  ASSERT_NE(dialog_body, nullptr);
  auto button_row = find_descendant_mode(dialog_body, "layer-name-dialog-buttons");
  ASSERT_NE(button_row, nullptr);

  auto content = dialog_body->effective_style.content_rect(dialog_body->bounds);
  EXPECT_GE(button_row->bounds.x, content.x);
  EXPECT_GE(button_row->bounds.y, content.y);
  EXPECT_LE(button_row->bounds.x + button_row->bounds.w, content.x + content.w);
  EXPECT_LE(button_row->bounds.y + button_row->bounds.h, content.y + content.h);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> buttons;
  find_descendant_modes(button_row, "ui/button", buttons);
  ASSERT_EQ(buttons.size(), 2u);

  for (const auto& button : buttons)
  {
    ASSERT_NE(button, nullptr);
    EXPECT_GE(button->bounds.y, content.y);
    EXPECT_LE(button->bounds.y + button->bounds.h, content.y + content.h);
  }
}

TEST_F(TilemapEditorAppFixtureTest, current_layer_name_dialog_focuses_text_input)
{
  set_frame_size({1200, 700});
  load_current_tilemap_editor();
  load_program();

  session().push_mode("layer-name-dialog-modal",
                      eval("{:operation :rename "
                           " :index 0 "
                           " :title \"Rename Layer\" "
                           " :name \"Terrain\"}"));
  ASSERT_NO_THROW(session().process_messages());

  auto text_input_inner = find_first_mode(session().active_mode, "ui/text-input-inner");
  ASSERT_NE(text_input_inner, nullptr);
  ASSERT_TRUE(session().focus_state.has_focus());
  EXPECT_EQ(session().focus_state.focused.lock().get(), text_input_inner.get());
  EXPECT_TRUE(text_input_inner->interaction.focused);

  auto window = find_first_mode(session().active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_TRUE(window->interaction.focus_within);
}

TEST_F(TilemapEditorAppFixtureTest, current_layer_confirm_dialog_focuses_cancel_button)
{
  set_frame_size({1200, 700});
  load_current_tilemap_editor();
  load_program();

  session().push_mode("layer-confirm-dialog-modal",
                      eval("{:operation :delete "
                           " :index 0 "
                           " :title \"Delete Layer\" "
                           " :message \"Delete Terrain?\" "
                           " :confirm-label \"Delete\"}"));
  ASSERT_NO_THROW(session().process_messages());

  std::vector<std::shared_ptr<Pixils::Runtime::View>> buttons;
  find_descendant_modes(session().active_mode, "ui/button", buttons);
  ASSERT_EQ(buttons.size(), 2u);
  ASSERT_TRUE(session().focus_state.has_focus());
  EXPECT_EQ(session().focus_state.focused.lock().get(), buttons[0].get());
  EXPECT_TRUE(buttons[0]->interaction.focused);

  auto window = find_first_mode(session().active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_TRUE(window->interaction.focus_within);
}

TEST_F(TilemapEditorAppFixtureTest, current_layer_and_tile_palette_use_list_boxes)
{
  set_frame_size({1200, 700});
  load_current_tilemap_editor();
  load_program();

  ASSERT_NO_THROW(update_and_layout());
  ASSERT_NO_THROW(update_and_layout());

  auto layer_list = find_first_mode(session().active_mode, "layer-list");
  ASSERT_NE(layer_list, nullptr);
  auto layer_list_box = find_descendant_mode(layer_list, "ui/list-box");
  ASSERT_NE(layer_list_box, nullptr);

  std::vector<std::shared_ptr<Pixils::Runtime::View>> layer_rows;
  find_descendant_modes(layer_list, "layer-row", layer_rows);
  ASSERT_EQ(layer_rows.size(), 4u);
  auto layer_viewport = find_descendant_mode(layer_list_box, "ui/scroll-pane-viewport");
  ASSERT_NE(layer_viewport, nullptr);
  EXPECT_EQ(layer_rows[0]->bounds.w, layer_viewport->bounds.w);

  auto first_layer_name =
    Lisple::Dict::get_property(layer_rows[0]->state, Lisple::keyword("layer-name"));
  ASSERT_NE(first_layer_name, nullptr);
  EXPECT_EQ(first_layer_name->to_string(), "\"Terrain\"");
  auto first_visibility_label = Lisple::Dict::get_property(
    layer_rows[0]->state, Lisple::keyword("visibility-label"));
  ASSERT_NE(first_visibility_label, nullptr);
  EXPECT_EQ(first_visibility_label->to_string(), "\"V\"");

  std::vector<std::shared_ptr<Pixils::Runtime::View>> tile_swatches;
  find_descendant_modes(session().active_mode, "tile-swatch", tile_swatches);
  ASSERT_GT(tile_swatches.size(), 1u);
  auto first_tile_name =
    Lisple::Dict::get_property(tile_swatches[0]->state, Lisple::keyword("name"));
  ASSERT_NE(first_tile_name, nullptr);
  EXPECT_EQ(first_tile_name->to_string(), "\"Grass\"");

  std::vector<std::shared_ptr<Pixils::Runtime::View>> list_boxes;
  find_descendant_modes(session().active_mode, "ui/list-box", list_boxes);
  EXPECT_GE(list_boxes.size(), 2u);

  auto second_tile = tile_swatches[1];
  input().mouse_down({static_cast<float>(second_tile->bounds.x + 4),
                      static_cast<float>(second_tile->bounds.y + 4)});
  ASSERT_NO_THROW(update_cycle());
  input().mouse_up({static_cast<float>(second_tile->bounds.x + 4),
                    static_cast<float>(second_tile->bounds.y + 4)});
  ASSERT_NO_THROW(update_cycle());

  auto selected_tile =
    Lisple::Dict::get_property(session().active_mode->state,
                               Lisple::keyword("selected-tile"));
  ASSERT_NE(selected_tile, nullptr);
  EXPECT_EQ(selected_tile->to_string(), "1");

  std::vector<std::shared_ptr<Pixils::Runtime::View>> visibility_toggles;
  find_descendant_modes(layer_list, "layer-visibility-toggle", visibility_toggles);
  ASSERT_EQ(visibility_toggles.size(), 4u);
  auto first_toggle = visibility_toggles[0];
  input().mouse_down({static_cast<float>(first_toggle->bounds.x + 4),
                      static_cast<float>(first_toggle->bounds.y + 4)});
  ASSERT_NO_THROW(update_cycle());
  input().mouse_up({static_cast<float>(first_toggle->bounds.x + 4),
                    static_cast<float>(first_toggle->bounds.y + 4)});
  ASSERT_NO_THROW(update_cycle());
  ASSERT_NO_THROW(update_and_layout());

  auto hidden_layer_indices =
    Lisple::Dict::get_property(session().active_mode->state,
                               Lisple::keyword("hidden-layer-indices"));
  ASSERT_NE(hidden_layer_indices, nullptr);
  EXPECT_EQ(hidden_layer_indices->to_string(), "[0]");

  layer_rows.clear();
  find_descendant_modes(layer_list, "layer-row", layer_rows);
  ASSERT_EQ(layer_rows.size(), 4u);
  first_visibility_label = Lisple::Dict::get_property(
    layer_rows[0]->state, Lisple::keyword("visibility-label"));
  ASSERT_NE(first_visibility_label, nullptr);
  EXPECT_EQ(first_visibility_label->to_string(), "\"-\"");
}
