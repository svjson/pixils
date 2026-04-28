#ifndef PIXILS__TEST__APPFIXTURE__COMPOSABLE_APP_SESSION_FIXTURE_H
#define PIXILS__TEST__APPFIXTURE__COMPOSABLE_APP_SESSION_FIXTURE_H

#include "../input_simulator.h"
#include "app_manifest.h"
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/session.h>
#include <pixils/script.h>

#include <SDL2/SDL_render.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>
#include <memory>
#include <string>
#include <vector>

class ComposableAppSessionFixture : public ::testing::Test
{
 protected:
  struct RuntimeAssetCopy
  {
    std::filesystem::path source_path;
    std::filesystem::path dest_relative_path;
  };

  Pixils::RenderContext render_ctx{};
  Pixils::FrameEvents events;
  InputSimulator input_simulator;
  std::unique_ptr<Pixils::HookContext> hook_ctx;
  std::filesystem::path app_root;
  std::unique_ptr<Lisple::Runtime> lisple_runtime;
  std::unique_ptr<Pixils::Runtime::Session> pixils_session;

  ComposableAppSessionFixture();

  void TearDown() override;

  void load_app(const Pixils::Test::AppFixture::AppManifest& manifest,
                const std::string& main_namespace,
                const std::vector<std::string>& entry_files,
                const std::vector<RuntimeAssetCopy>& runtime_assets = {});

  void set_frame_size(const Pixils::Dimension& dim);
  const Pixils::Dimension& frame_size() const;
  InputSimulator& input();
  void update_cycle();
  void render_cycle();
  void frame_cycle();
  Lisple::Runtime& pixils();
  Pixils::Runtime::Session& session();
  Lisple::sptr_rtval eval(const std::string& source);
  SDL_Texture* render_target();
  const std::filesystem::path& app_root_dir() const;

 private:
  static std::filesystem::path make_temp_app_root();
  void stage_runtime_assets(const std::vector<RuntimeAssetCopy>& runtime_assets);
};

#endif
