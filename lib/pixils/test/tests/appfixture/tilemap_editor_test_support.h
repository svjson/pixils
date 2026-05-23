#pragma once

#include "../fixture.h"
#include "../render_fixture.h"

#include <filesystem>
#include <fstream>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>
#include <sstream>
#include <string>
#include <vector>

inline std::string lisp_string(const std::string& value)
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
  out.push_back('\"');
  return out;
}

inline std::string read_text_file(const std::filesystem::path& path)
{
  std::ifstream in(path);
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

inline void find_descendant_modes(const std::shared_ptr<Pixils::Runtime::View>& view,
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

inline std::shared_ptr<Pixils::Runtime::View> find_descendant_mode_containing_state(
  const std::shared_ptr<Pixils::Runtime::View>& view,
  const std::string& mode_name,
  const std::string& state_text)
{
  if (!view) return nullptr;
  if (view->mode && view->mode->name == mode_name && view->state &&
      view->state->to_string().find(state_text) != std::string::npos)
  {
    return view;
  }

  for (const auto& child : view->children)
  {
    auto match = find_descendant_mode_containing_state(child, mode_name, state_text);
    if (match) return match;
  }
  return nullptr;
}

inline void read_tilemap_editor_sources(Lisple::Runtime& runtime)
{
  runtime.read_file("examples/tilemap-editor/src/assets.lisple");
  runtime.read_file("examples/tilemap-editor/src/model/data.lisple");
  runtime.read_file("examples/tilemap-editor/src/model/tilemap.lisple");
  runtime.read_file("examples/tilemap-editor/src/io/project.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tilemap/renderer.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tilemap/canvas.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tilemap/inspector.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tilemap/palette.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tilemap/controls.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tilemap/layout.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/tileset/layout.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/menu.lisple");
  runtime.read_file("examples/tilemap-editor/src/view/theme.lisple");
  runtime.read_file("examples/tilemap-editor/src/root.lisple");
}

class TilemapEditorProjectIoTest : public BaseFixture
{
};

class TilemapEditorStartupTest : public RenderFixture
{
 protected:
  TilemapEditorStartupTest() { render_ctx.buffer_dim = {800, 600}; }
};
