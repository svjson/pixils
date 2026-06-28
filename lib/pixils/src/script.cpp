
#include "pixils/script.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/audio_namespace.h>
#include <pixils/binding/clipboard_namespace.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/image_namespace.h>
#include <pixils/binding/keyboard_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/render_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/binding/state_counter_namespace.h>
#include <pixils/binding/state_timer_namespace.h>
#include <pixils/binding/ui/style/style_namespace.h>
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/embedded_lisp_sources.h>
#include <pixils/font_registry.h>
#include <pixils/ui/components/text_node.h>

#include <roo/io/dir_root_file_system.h>
#include <roo/lang/io/io_namespace.h>

#include <stdexcept>

namespace Pixils
{
  std::vector<std::unique_ptr<Roo::Namespace>> make_roo_native_namespaces(
    RenderContext& ctx)
  {
    std::vector<std::unique_ptr<Roo::Namespace>> namespaces;
    namespaces.push_back(std::make_unique<Pixils::Script::PixilsNamespace>(ctx));
    namespaces.push_back(std::make_unique<Pixils::Script::ResourceNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::AudioNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::ClipboardNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::ColorNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::ImageNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::KeyboardNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::PointNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::RectNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::RenderNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::StateCounterNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::StateTimerNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::StyleNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::UINamespace>());
    return namespaces;
  }

  Roo::Runtime init_roo_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      const std::vector<std::string>& source_files)
  {
    return init_roo_runtime(ctx, default_namespace, nullptr, source_files);
  }
  Roo::Runtime init_roo_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      std::function<void(RuntimeConfiguration*)> init_fn,
                                      const std::vector<std::string>& source_files)
  {
    std::vector<std::unique_ptr<Roo::Namespace>> namespaces =
      make_roo_native_namespaces(ctx);
    namespaces.push_back(std::make_unique<Roo::Namespace>(Roo::make_io_namespace()));

    RuntimeConfiguration rtconfig{.native_namespaces = std::move(namespaces),
                                  .load_path = {"."},
                                  .namespace_roots = {}};

    if (init_fn)
    {
      init_fn(&rtconfig);
    }

    ctx.asset_registry = std::make_unique<Asset::Registry>(ctx, rtconfig.asset_base_path);
    ctx.font_registry = std::make_unique<FontRegistry>();
    if (ctx.renderer)
    {
      ctx.asset_registry->load_embedded_assets();
    }

    std::unique_ptr<Roo::DirRootFileSystem> fs =
      std::make_unique<Roo::DirRootFileSystem>(rtconfig.load_path);

    Roo::Runtime roo_runtime(default_namespace,
                                   std::move(rtconfig.native_namespaces),
                                   std::move(fs.release()));
    roo_runtime.set_namespace_roots(rtconfig.namespace_roots);
    UI::Components::register_text_node_component(roo_runtime);
    for (const auto& embedded_source : EmbeddedLisp::core_sources())
    {
      try
      {
        roo_runtime.eval(embedded_source.source);
      }
      catch (const std::exception& e)
      {
        throw std::runtime_error(std::string("Failed to evaluate embedded Roo source ") +
                                 embedded_source.path + ": " + e.what());
      }
    }
    roo_runtime.eval("(ns " + default_namespace + ")");
    for (auto& file_name : source_files)
    {
      roo_runtime.read_file(file_name);
    }

    return roo_runtime;
  }

  std::unique_ptr<Roo::Runtime> make_roo_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    const std::vector<std::string>& source_files)
  {
    return make_roo_runtime(ctx, default_namespace, nullptr, source_files);
  }

  std::unique_ptr<Roo::Runtime> make_roo_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    std::function<void(RuntimeConfiguration*)> init_fn,
    const std::vector<std::string>& source_files)
  {
    std::vector<std::unique_ptr<Roo::Namespace>> namespaces =
      make_roo_native_namespaces(ctx);
    namespaces.push_back(std::make_unique<Roo::Namespace>(Roo::make_io_namespace()));

    RuntimeConfiguration rtconfig{.native_namespaces = std::move(namespaces),
                                  .load_path = {"."},
                                  .namespace_roots = {}};

    if (init_fn)
    {
      init_fn(&rtconfig);
    }

    ctx.asset_registry = std::make_unique<Asset::Registry>(ctx, rtconfig.asset_base_path);
    ctx.font_registry = std::make_unique<FontRegistry>();
    if (ctx.renderer)
    {
      ctx.asset_registry->load_embedded_assets();
    }

    std::unique_ptr<Roo::DirRootFileSystem> fs =
      std::make_unique<Roo::DirRootFileSystem>(rtconfig.load_path);

    auto roo_runtime =
      std::make_unique<Roo::Runtime>(default_namespace,
                                        std::move(rtconfig.native_namespaces),
                                        std::move(fs.release()));
    roo_runtime->set_namespace_roots(rtconfig.namespace_roots);
    UI::Components::register_text_node_component(*roo_runtime);
    for (const auto& embedded_source : EmbeddedLisp::core_sources())
    {
      try
      {
        roo_runtime->eval(embedded_source.source);
      }
      catch (const std::exception& e)
      {
        throw std::runtime_error(std::string("Failed to evaluate embedded Roo source ") +
                                 embedded_source.path + ": " + e.what());
      }
    }
    roo_runtime->eval("(ns " + default_namespace + ")");
    for (auto& file_name : source_files)
    {
      roo_runtime->read_file(file_name);
    }

    return roo_runtime;
  }
} // namespace Pixils
