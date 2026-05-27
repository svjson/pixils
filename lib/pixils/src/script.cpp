
#include "pixils/script.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/audio_namespace.h>
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

#include <lisple/io/dir_root_file_system.h>
#include <lisple/lang/io/io_namespace.h>

namespace Pixils
{
  std::vector<std::unique_ptr<Lisple::Namespace>> make_lisple_native_namespaces(
    RenderContext& ctx)
  {
    std::vector<std::unique_ptr<Lisple::Namespace>> namespaces;
    namespaces.push_back(std::make_unique<Pixils::Script::PixilsNamespace>(ctx));
    namespaces.push_back(std::make_unique<Pixils::Script::ResourceNamespace>());
    namespaces.push_back(std::make_unique<Pixils::Script::AudioNamespace>());
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

  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      const std::vector<std::string>& source_files)
  {
    return init_lisple_runtime(ctx, default_namespace, nullptr, source_files);
  }
  Lisple::Runtime init_lisple_runtime(RenderContext& ctx,
                                      const std::string& default_namespace,
                                      std::function<void(RuntimeConfiguration*)> init_fn,
                                      const std::vector<std::string>& source_files)
  {
    std::vector<std::unique_ptr<Lisple::Namespace>> namespaces =
      make_lisple_native_namespaces(ctx);
    namespaces.push_back(std::make_unique<Lisple::Namespace>(Lisple::make_io_namespace()));

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

    std::unique_ptr<Lisple::DirRootFileSystem> fs =
      std::make_unique<Lisple::DirRootFileSystem>(rtconfig.load_path);

    Lisple::Runtime lisple_runtime(default_namespace,
                                   std::move(rtconfig.native_namespaces),
                                   std::move(fs.release()));
    lisple_runtime.set_namespace_roots(rtconfig.namespace_roots);
    UI::Components::register_text_node_component(lisple_runtime);
    for (const auto& embedded_source : EmbeddedLisp::core_sources())
    {
      lisple_runtime.eval(embedded_source.source);
    }
    lisple_runtime.eval("(ns " + default_namespace + ")");
    for (auto& file_name : source_files)
    {
      lisple_runtime.read_file(file_name);
    }

    return lisple_runtime;
  }

  std::unique_ptr<Lisple::Runtime> make_lisple_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    const std::vector<std::string>& source_files)
  {
    return make_lisple_runtime(ctx, default_namespace, nullptr, source_files);
  }

  std::unique_ptr<Lisple::Runtime> make_lisple_runtime(
    RenderContext& ctx,
    const std::string& default_namespace,
    std::function<void(RuntimeConfiguration*)> init_fn,
    const std::vector<std::string>& source_files)
  {
    std::vector<std::unique_ptr<Lisple::Namespace>> namespaces =
      make_lisple_native_namespaces(ctx);
    namespaces.push_back(std::make_unique<Lisple::Namespace>(Lisple::make_io_namespace()));

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

    std::unique_ptr<Lisple::DirRootFileSystem> fs =
      std::make_unique<Lisple::DirRootFileSystem>(rtconfig.load_path);

    auto lisple_runtime =
      std::make_unique<Lisple::Runtime>(default_namespace,
                                        std::move(rtconfig.native_namespaces),
                                        std::move(fs.release()));
    lisple_runtime->set_namespace_roots(rtconfig.namespace_roots);
    UI::Components::register_text_node_component(*lisple_runtime);
    for (const auto& embedded_source : EmbeddedLisp::core_sources())
    {
      lisple_runtime->eval(embedded_source.source);
    }
    lisple_runtime->eval("(ns " + default_namespace + ")");
    for (auto& file_name : source_files)
    {
      lisple_runtime->read_file(file_name);
    }

    return lisple_runtime;
  }
} // namespace Pixils
