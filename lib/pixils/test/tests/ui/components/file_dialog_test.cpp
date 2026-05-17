#include "../../render_fixture.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using FileDialogTest = RenderFixture;
using Pixils::Runtime::View;

namespace
{
  namespace fs = std::filesystem;

  std::string lisp_string(const std::string& value)
  {
    std::string out = "\"";
    for (char c : value)
    {
      if (c == '\\' || c == '"')
      {
        out.push_back('\\');
      }
      out.push_back(c);
    }
    out.push_back('"');
    return out;
  }

  struct TempProject
  {
    fs::path root;

    TempProject()
    {
      const auto suffix =
        std::chrono::steady_clock::now().time_since_epoch().count();
      root = fs::temp_directory_path() /
             ("pixils-file-dialog-test-" + std::to_string(suffix));
      fs::create_directories(root / "assets");
      write(root / "tilemap-editor.edn", "{:name \"editor\"}");
      write(root / "demo-map.edn", "{:name \"demo\"}");
      write(root / "readme.txt", "text");
      write(root / "assets" / "terrain.edn", "{:terrain true}");
      write(root / "assets" / "tilesets.edn", "{:tilesets []}");
    }

    ~TempProject()
    {
      std::error_code ec;
      fs::remove_all(root, ec);
    }

    static void write(const fs::path& path, const std::string& contents)
    {
      std::ofstream out(path);
      out << contents;
    }

    std::string path() const { return root.string(); }
  };

  Lisple::sptr_val get_key(const Lisple::sptr_val& value, const std::string& key)
  {
    return Lisple::Dict::get_property(value, Lisple::keyword(key));
  }

  std::shared_ptr<View> find_button_with_label(const std::shared_ptr<View>& view,
                                               const std::string& label_text)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/button")
    {
      auto label = get_key(view->state, "label");
      if (label && label->str() == label_text) return view;
    }

    for (const auto& child : view->children)
    {
      auto match = find_button_with_label(child, label_text);
      if (match) return match;
    }

    return nullptr;
  }

  std::shared_ptr<View> find_list_item_with_label(const std::shared_ptr<View>& view,
                                                  const std::string& label_text)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == "ui/list-box-item")
    {
      auto label = get_key(view->state, "label");
      if (label && label->str() == label_text) return view;
    }

    for (const auto& child : view->children)
    {
      auto match = find_list_item_with_label(child, label_text);
      if (match) return match;
    }

    return nullptr;
  }

  std::shared_ptr<View> find_mode(const std::shared_ptr<View>& view,
                                  const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;

    for (const auto& child : view->children)
    {
      auto match = find_mode(child, mode_name);
      if (match) return match;
    }

    return nullptr;
  }

} // namespace

TEST_F(FileDialogTest, open_file_dialog_returns_selected_file)
{
  render_ctx.buffer_dim = {640, 480};
  TempProject project;

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.file-dialog/open-file-dialog!
                  ctx
                  {:title "Open Project"
                   :mode :file-dialog/open
                   :path )" + lisp_string(project.path()) + R"(
                   :result-event :project/open-result})
                 {:result nil}))
       :on {:project/open-result (fn [state event ctx]
                                   (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.process_messages();
  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  session.update_mode();
  session.render_mode();

  auto window_body = find_mode(session.active_mode, "ui/window-body");
  auto file_dialog_body = find_mode(session.active_mode, "ui/file-dialog-body");
  ASSERT_NE(window_body, nullptr);
  ASSERT_NE(file_dialog_body, nullptr);
  EXPECT_EQ(file_dialog_body->bounds.w, window_body->bounds.w);

  auto entry = find_list_item_with_label(session.active_mode, "    tilemap-editor.edn");
  ASSERT_NE(entry, nullptr);
  input().mouse_down({entry->bounds.x + (entry->bounds.w / 2),
                      entry->bounds.y + (entry->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();
  input().mouse_up({entry->bounds.x + (entry->bounds.w / 2),
                    entry->bounds.y + (entry->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();

  auto open_button = find_button_with_label(session.active_mode, "Open");
  ASSERT_NE(open_button, nullptr);
  EXPECT_GT(open_button->bounds.w, 0);
  EXPECT_GT(open_button->bounds.h, 0);
  input().mouse_down({open_button->bounds.x + (open_button->bounds.w / 2),
                      open_button->bounds.y + (open_button->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();
  input().mouse_up({open_button->bounds.x + (open_button->bounds.w / 2),
                    open_button->bounds.y + (open_button->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(get_key(result, "type")->str(), "confirm");
  EXPECT_EQ(get_key(result, "mode")->str(), "file-dialog/open");
  EXPECT_EQ(get_key(result, "path")->str(),
            (project.root / "tilemap-editor.edn").string());
  EXPECT_EQ(get_key(result, "directory")->str(), project.path());
  EXPECT_EQ(get_key(result, "filename")->str(), "tilemap-editor.edn");
}

TEST_F(FileDialogTest, save_file_dialog_returns_entered_filename_path)
{
  render_ctx.buffer_dim = {640, 480};
  TempProject project;

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.file-dialog/open-file-dialog!
                  ctx
                  {:title "Save Project"
                   :mode :file-dialog/save
                   :path )" + lisp_string(project.path()) + R"(
                   :filename "new-map.edn"
                   :result-event :project/save-result})
                 {:result nil}))
       :on {:project/save-result (fn [state event ctx]
                                   (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.process_messages();
  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  session.update_mode();
  session.render_mode();

  auto save_button = find_button_with_label(session.active_mode, "Save");
  ASSERT_NE(save_button, nullptr);
  EXPECT_GT(save_button->bounds.w, 0);
  EXPECT_GT(save_button->bounds.h, 0);
  input().mouse_down({save_button->bounds.x + (save_button->bounds.w / 2),
                      save_button->bounds.y + (save_button->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();
  input().mouse_up({save_button->bounds.x + (save_button->bounds.w / 2),
                    save_button->bounds.y + (save_button->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(get_key(result, "type")->str(), "confirm");
  EXPECT_EQ(get_key(result, "mode")->str(), "file-dialog/save");
  EXPECT_EQ(get_key(result, "path")->str(),
            (project.root / "new-map.edn").string());
  EXPECT_EQ(get_key(result, "filename")->str(), "new-map.edn");
}

TEST_F(FileDialogTest, filter_combo_box_updates_confirm_result_filter)
{
  render_ctx.buffer_dim = {640, 480};
  TempProject project;

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.file-dialog/open-file-dialog!
                  ctx
                  {:title "Open Project"
                   :mode :file-dialog/open
                   :path )" + lisp_string(project.path()) + R"(
                   :result-event :project/open-result})
                 {:result nil}))
       :on {:project/open-result (fn [state event ctx]
                                   (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.process_messages();
  session.update_mode();
  session.render_mode();

  auto combo = find_mode(session.active_mode, "ui/combo-box");
  ASSERT_NE(combo, nullptr);
  input().mouse_down({combo->bounds.x + (combo->bounds.w / 2),
                      combo->bounds.y + (combo->bounds.h / 2)});
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "ui/combo-box-popup");
  session.render_mode();

  auto all_files = find_list_item_with_label(session.active_mode, "All files (*)");
  ASSERT_NE(all_files, nullptr);
  input().mouse_down({all_files->bounds.x + (all_files->bounds.w / 2),
                      all_files->bounds.y + (all_files->bounds.h / 2)});
  update_cycle();
  input().mouse_up({all_files->bounds.x + (all_files->bounds.w / 2),
                    all_files->bounds.y + (all_files->bounds.h / 2)});
  update_cycle();
  update_cycle();
  session.render_mode();

  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  auto entry = find_list_item_with_label(session.active_mode, "    readme.txt");
  ASSERT_NE(entry, nullptr);
  input().mouse_down({entry->bounds.x + (entry->bounds.w / 2),
                      entry->bounds.y + (entry->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();
  input().mouse_up({entry->bounds.x + (entry->bounds.w / 2),
                    entry->bounds.y + (entry->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();

  auto open_button = find_button_with_label(session.active_mode, "Open");
  ASSERT_NE(open_button, nullptr);
  input().mouse_down({open_button->bounds.x + (open_button->bounds.w / 2),
                      open_button->bounds.y + (open_button->bounds.h / 2)},
                     SDL_BUTTON_LEFT);
  update_cycle();
  session.render_mode();
  input().mouse_up({open_button->bounds.x + (open_button->bounds.w / 2),
                    open_button->bounds.y + (open_button->bounds.h / 2)},
                   SDL_BUTTON_LEFT);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  auto filter = get_key(result, "filter");
  ASSERT_NE(filter, nullptr);
  EXPECT_EQ(get_key(result, "path")->str(), (project.root / "readme.txt").string());
  EXPECT_EQ(get_key(filter, "label")->str(), "All files (*)");
}

TEST_F(FileDialogTest, double_click_directory_navigates_into_it)
{
  render_ctx.buffer_dim = {640, 480};
  TempProject project;

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.file-dialog/open-file-dialog!
                  ctx
                  {:title "Open Project"
                   :mode :file-dialog/open
                   :path )" + lisp_string(project.path()) + R"(
                   :result-event :project/open-result})
                 {:result nil}))
       :on {:project/open-result (fn [state event ctx]
                                   (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.process_messages();
  session.update_mode();
  session.render_mode();

  auto assets = find_list_item_with_label(session.active_mode, "[D] assets");
  ASSERT_NE(assets, nullptr);
  input().mouse_down({assets->bounds.x + (assets->bounds.w / 2),
                      assets->bounds.y + (assets->bounds.h / 2)});
  update_cycle();
  input().mouse_up({assets->bounds.x + (assets->bounds.w / 2),
                    assets->bounds.y + (assets->bounds.h / 2)});
  update_cycle();
  session.render_mode();
  input().mouse_down({assets->bounds.x + (assets->bounds.w / 2),
                      assets->bounds.y + (assets->bounds.h / 2)},
                     SDL_BUTTON_LEFT,
                     2);
  update_cycle();
  input().mouse_up({assets->bounds.x + (assets->bounds.w / 2),
                    assets->bounds.y + (assets->bounds.h / 2)},
                   SDL_BUTTON_LEFT,
                   2);
  update_cycle();
  update_cycle();
  session.render_mode();

  ASSERT_EQ(session.active_mode->mode->name, "ui/dialog-frame");
  auto file_dialog_body = find_mode(session.active_mode, "ui/file-dialog-body");
  ASSERT_NE(file_dialog_body, nullptr);
  auto path = get_key(file_dialog_body->state, "path");
  ASSERT_NE(path, nullptr);
  EXPECT_EQ(path->str(), (project.root / "assets").string());

  auto terrain = find_list_item_with_label(session.active_mode, "    terrain.edn");
  ASSERT_NE(terrain, nullptr);
}

TEST_F(FileDialogTest, double_click_file_confirms_dialog)
{
  render_ctx.buffer_dim = {640, 480};
  TempProject project;

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do
                 (pixils.ui.file-dialog/open-file-dialog!
                  ctx
                  {:title "Open Project"
                   :mode :file-dialog/open
                   :path )" + lisp_string(project.path()) + R"(
                   :result-event :project/open-result})
                 {:result nil}))
       :on {:project/open-result (fn [state event ctx]
                                   (assoc state :result (:payload event)))}})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.process_messages();
  session.update_mode();
  session.render_mode();

  auto entry = find_list_item_with_label(session.active_mode, "    demo-map.edn");
  ASSERT_NE(entry, nullptr);
  input().mouse_down({entry->bounds.x + (entry->bounds.w / 2),
                      entry->bounds.y + (entry->bounds.h / 2)});
  update_cycle();
  input().mouse_up({entry->bounds.x + (entry->bounds.w / 2),
                    entry->bounds.y + (entry->bounds.h / 2)});
  update_cycle();
  session.render_mode();
  input().mouse_down({entry->bounds.x + (entry->bounds.w / 2),
                      entry->bounds.y + (entry->bounds.h / 2)},
                     SDL_BUTTON_LEFT,
                     2);
  update_cycle();
  input().mouse_up({entry->bounds.x + (entry->bounds.w / 2),
                    entry->bounds.y + (entry->bounds.h / 2)},
                   SDL_BUTTON_LEFT,
                   2);
  update_cycle();

  ASSERT_EQ(session.active_mode->mode->name, "root-mode");
  auto result = get_key(session.active_mode->state, "result");
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(get_key(result, "type")->str(), "confirm");
  EXPECT_EQ(get_key(result, "path")->str(), (project.root / "demo-map.edn").string());
  EXPECT_EQ(get_key(result, "filename")->str(), "demo-map.edn");
}
