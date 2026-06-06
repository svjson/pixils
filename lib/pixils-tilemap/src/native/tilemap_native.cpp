#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/color.h>
#include <pixils/context.h>
#include <pixils/geom.h>

#include <SDL2/SDL_render.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <optional>
#include <roo-package/native_abi.h>
#include <roo/exec.h>
#include <roo/namespace.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>
#include <string>
#include <tuple>
#include <vector>

namespace
{
  std::string package_last_error;

  Roo::sptr_val prop(const Roo::sptr_val& map, const std::string& key)
  {
    if (!map || map->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;
    return Roo::Dict::get_property(map, Roo::keyword(key));
  }

  bool nil_value(const Roo::sptr_val& value)
  {
    return !value || value->type == Roo::Value::Type::NIL ||
           (value->type == Roo::Value::Type::SYMBOL && value->str() == "nil");
  }

  std::string value_name(const Roo::sptr_val& value)
  {
    if (nil_value(value)) return "";
    std::string name = value->str();
    if (!name.empty() && name.front() == ':') name = name.substr(1);
    return name;
  }

  bool keyword_named(const Roo::sptr_val& value, const std::string& expected)
  {
    return value && value->type == Roo::Value::Type::KEYWORD &&
           value_name(value) == expected;
  }

  int int_prop(const Roo::sptr_val& map, const std::string& key, int fallback)
  {
    auto value = prop(map, key);
    if (nil_value(value)) return fallback;
    if (value->type != Roo::Value::Type::NUMBER)
    {
      throw Roo::TypeError(":" + key + " must be a number");
    }
    return value->num().get_int();
  }

  double number_prop(const Roo::sptr_val& map, const std::string& key, double fallback)
  {
    auto value = prop(map, key);
    if (nil_value(value)) return fallback;
    if (value->type != Roo::Value::Type::NUMBER)
    {
      throw Roo::TypeError(":" + key + " must be a number");
    }
    return value->num().get_double();
  }

  bool truthy_prop(const Roo::sptr_val& map, const std::string& key)
  {
    auto value = prop(map, key);
    return value && Roo::is_truthy(*value);
  }

  Pixils::Rect rect_from_map(const Roo::sptr_val& map)
  {
    return Pixils::Rect{int_prop(map, "x", 0),
                        int_prop(map, "y", 0),
                        int_prop(map, "w", 0),
                        int_prop(map, "h", 0)};
  }

  std::optional<Pixils::Rect> optional_rect_prop(const Roo::sptr_val& map,
                                                 const std::string& key)
  {
    auto value = prop(map, key);
    if (nil_value(value)) return std::nullopt;
    if (value->type != Roo::Value::Type::MAP)
    {
      throw Roo::TypeError(":" + key + " must be a rect map");
    }
    return rect_from_map(value);
  }

  Pixils::Rect intersect_clip_rect(const std::optional<Pixils::Rect>& current,
                                   const Pixils::Rect& requested)
  {
    if (!current) return requested;

    int x1 = std::max(current->x, requested.x);
    int y1 = std::max(current->y, requested.y);
    int x2 = std::min(current->x + current->w, requested.x + requested.w);
    int y2 = std::min(current->y + current->h, requested.y + requested.h);

    return Pixils::Rect{x1, y1, std::max(0, x2 - x1), std::max(0, y2 - y1)};
  }

  int ceil_positive_div(int value, int divisor)
  {
    value = std::max(0, value);
    int q = value / divisor;
    return q * divisor == value ? q : q + 1;
  }

  struct AxisRange
  {
    int start = 0;
    int end = 0;
  };

  AxisRange render_axis_range(int size, int offset, int tile_size, int max_count)
  {
    if (size <= 0 || tile_size <= 0 || max_count <= 0) return {};

    int start = std::max(0, std::min(max_count, std::max(0, offset) / tile_size));
    int end = std::max(0, std::min(max_count, ceil_positive_div(size + offset, tile_size)));
    return AxisRange{start, std::max(start, end)};
  }

  struct RenderRanges
  {
    AxisRange x;
    AxisRange y;
  };

  struct RenderInput
  {
    int map_width = 0;
    int map_height = 0;
    int tile_size = 16;
    double zoom = 1.0;
    Pixils::Rect target_rect{0, 0, 0, 0};
    bool has_target_rect = false;
    Pixils::Rect offset{0, 0, 0, 0};
    Pixils::Rect render_offset{0, 0, 0, 0};
    RenderRanges ranges;
  };

  int scaled_tile_size(const RenderInput& input)
  {
    return static_cast<int>(std::round(input.tile_size * input.zoom));
  }

  RenderRanges render_ranges(const RenderInput& input)
  {
    int size = scaled_tile_size(input);
    Pixils::Rect rect =
      input.has_target_rect
        ? input.target_rect
        : Pixils::Rect{0, 0, input.map_width * size, input.map_height * size};
    return RenderRanges{render_axis_range(rect.w, input.offset.x, size, input.map_width),
                        render_axis_range(rect.h, input.offset.y, size, input.map_height)};
  }

  RenderInput render_input(const Roo::sptr_val& tilemap, const Roo::sptr_val& opts)
  {
    RenderInput input;
    input.map_width = int_prop(tilemap, "width", 0);
    input.map_height = int_prop(tilemap, "height", 0);
    input.tile_size = int_prop(tilemap, "tile-size", 16);
    input.zoom = number_prop(opts, "zoom", 1.0);

    auto offset = prop(opts, "offset");
    if (!nil_value(offset))
    {
      input.offset.x = int_prop(offset, "x", 0);
      input.offset.y = int_prop(offset, "y", 0);
    }

    auto target_rect = optional_rect_prop(opts, "target-rect");
    if (target_rect)
    {
      input.has_target_rect = true;
      input.target_rect = *target_rect;
      input.render_offset.x = input.offset.x - input.target_rect.x;
      input.render_offset.y = input.offset.y - input.target_rect.y;
    }
    else
    {
      input.render_offset = input.offset;
    }

    input.ranges = render_ranges(input);
    return input;
  }

  bool int_vector_contains(const Roo::sptr_val& values, int target)
  {
    if (nil_value(values)) return false;
    for (const auto& value : Roo::get_children(*values))
    {
      if (value && value->type == Roo::Value::Type::NUMBER &&
          value->num().get_int() == target)
      {
        return true;
      }
    }
    return false;
  }

  Roo::sptr_val layers_value(const Roo::sptr_val& tilemap, const Roo::sptr_val& opts)
  {
    auto layers = prop(opts, "layers");
    if (!nil_value(layers)) return layers;
    return prop(tilemap, "layers");
  }

  bool supported_layer(const Roo::sptr_val& layer)
  {
    return layer && layer->type == Roo::Value::Type::MAP &&
           keyword_named(prop(layer, "kind"), "prepared-tile-stack");
  }

  bool can_render(const Roo::sptr_val& tilemap, const Roo::sptr_val& opts)
  {
    if (truthy_prop(opts, "show-grid?")) return false;

    auto substitutions = prop(opts, "tile-substitutions");
    if (!nil_value(substitutions) && substitutions->type == Roo::Value::Type::MAP &&
        !Roo::Dict::keys(*substitutions).empty())
    {
      return false;
    }

    auto layers = layers_value(tilemap, opts);
    if (nil_value(layers)) return true;

    auto hidden = prop(opts, "hidden-layer-indices");
    int index = 0;
    for (const auto& layer : Roo::get_children(*layers))
    {
      if (!int_vector_contains(hidden, index) && !supported_layer(layer)) return false;
      index++;
    }
    return true;
  }

  Roo::sptr_val layer_cell(const Roo::sptr_val& tiles, int x, int y)
  {
    if (nil_value(tiles)) return Roo::Constant::NIL;
    auto rows = Roo::get_children(*tiles);
    if (y < 0 || y >= static_cast<int>(rows.size())) return Roo::Constant::NIL;
    auto row = rows[y];
    if (nil_value(row)) return Roo::Constant::NIL;
    auto cells = Roo::get_children(*row);
    if (x < 0 || x >= static_cast<int>(cells.size())) return Roo::Constant::NIL;
    return cells[x];
  }

  std::vector<Roo::sptr_val> tile_stack(const Roo::sptr_val& cell)
  {
    if (nil_value(cell)) return {};
    return Roo::get_children(*cell);
  }

  Pixils::Rect tile_rect(const RenderInput& input, int x, int y)
  {
    int size = scaled_tile_size(input);
    return Pixils::Rect{(x * size) - input.render_offset.x,
                        (y * size) - input.render_offset.y,
                        size,
                        size};
  }

  SDL_Rect centered_dest(const Pixils::Rect& rect, int source_w, int source_h)
  {
    double scale = std::min(static_cast<double>(rect.w) / static_cast<double>(source_w),
                            static_cast<double>(rect.h) / static_cast<double>(source_h));
    int width = static_cast<int>(std::round(source_w * scale));
    int height = static_cast<int>(std::round(source_h * scale));
    return SDL_Rect{rect.x + ((rect.w - width) / 2),
                    rect.y + ((rect.h - height) / 2),
                    width,
                    height};
  }

  bool image_key(const Roo::sptr_val& tile, std::string* bundle, std::string* asset)
  {
    auto image = prop(tile, "image");
    if (!image || image->type != Roo::Value::Type::KEYWORD) return false;
    auto [qualified_bundle, qualified_asset] = image->qual();
    *bundle = qualified_bundle;
    *asset = qualified_asset;
    return !bundle->empty() && !asset->empty();
  }

  std::optional<SDL_Rect> source_rect(const Roo::sptr_val& tile)
  {
    auto source = prop(tile, "source");
    if (nil_value(source)) return std::nullopt;
    Pixils::Rect rect = rect_from_map(source);
    if (rect.w <= 0 || rect.h <= 0) return std::nullopt;
    return rect.to_SDL_rect();
  }

  void fill_rect(SDL_Renderer* renderer,
                 const Pixils::Rect& rect,
                 Uint8 r,
                 Uint8 g,
                 Uint8 b,
                 Uint8 a)
  {
    SDL_Rect sdl_rect = rect.to_SDL_rect();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, &sdl_rect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  void draw_missing_tile(Pixils::RenderContext& rc, const Pixils::Rect& rect)
  {
    fill_rect(rc.renderer, rect, 0xff, 0x00, 0xff, 0xff);
  }

  void draw_color_tile(Pixils::RenderContext& rc,
                       const Roo::sptr_val& tile,
                       const Pixils::Rect& rect)
  {
    auto color = prop(tile, "color");
    if (nil_value(color))
    {
      draw_missing_tile(rc, rect);
      return;
    }

    if (Pixils::Script::HostType::COLOR.is_type_of(*color))
    {
      const Pixils::Color& native_color = Roo::obj<Pixils::Color>(*color);
      fill_rect(rc.renderer,
                rect,
                native_color.r,
                native_color.g,
                native_color.b,
                native_color.a);
      return;
    }

    fill_rect(rc.renderer,
              rect,
              static_cast<Uint8>(int_prop(color, "r", 0)),
              static_cast<Uint8>(int_prop(color, "g", 0)),
              static_cast<Uint8>(int_prop(color, "b", 0)),
              static_cast<Uint8>(int_prop(color, "a", 0xff)));
  }

  void draw_texture_tile(Pixils::RenderContext& rc,
                         const Roo::sptr_val& tile,
                         const Pixils::Rect& rect,
                         bool image_background)
  {
    std::string bundle;
    std::string asset;
    if (!image_key(tile, &bundle, &asset))
    {
      draw_missing_tile(rc, rect);
      return;
    }

    SDL_Texture* texture = rc.asset_registry->get_image(bundle, asset);
    if (!texture) return;

    std::optional<SDL_Rect> source = source_rect(tile);
    int source_w = 0;
    int source_h = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &source_w, &source_h);
    if (source)
    {
      source_w = source->w;
      source_h = source->h;
    }
    if (source_w <= 0 || source_h <= 0)
    {
      draw_missing_tile(rc, rect);
      return;
    }

    if (image_background)
    {
      fill_rect(rc.renderer, rect, 0x12, 0x15, 0x18, 0xff);
    }

    SDL_Rect dest = centered_dest(rect, source_w, source_h);
    const SDL_Rect* source_ptr = source ? &*source : nullptr;
    SDL_RenderCopy(rc.renderer, texture, source_ptr, &dest);
  }

  void draw_tile(Pixils::RenderContext& rc,
                 const Roo::sptr_val& tile,
                 const Pixils::Rect& rect)
  {
    if (nil_value(tile) || tile->type != Roo::Value::Type::MAP)
    {
      draw_missing_tile(rc, rect);
      return;
    }

    std::string type = value_name(prop(tile, "type"));
    if (type == "color")
    {
      draw_color_tile(rc, tile, rect);
    }
    else if (type == "image")
    {
      draw_texture_tile(rc, tile, rect, true);
    }
    else if (type == "sprite")
    {
      draw_texture_tile(rc, tile, rect, false);
    }
    else
    {
      draw_missing_tile(rc, rect);
    }
  }

  void render_layer(Pixils::RenderContext& rc,
                    const RenderInput& input,
                    const Roo::sptr_val& layer)
  {
    auto tiles = prop(layer, "tiles");
    for (int y = input.ranges.y.start; y < input.ranges.y.end; y++)
    {
      for (int x = input.ranges.x.start; x < input.ranges.x.end; x++)
      {
        Roo::sptr_val cell = layer_cell(tiles, x, y);
        if (nil_value(cell)) continue;

        auto stack = tile_stack(cell);
        if (stack.empty()) continue;

        Pixils::Rect rect = tile_rect(input, x, y);
        for (const auto& tile : stack)
        {
          draw_tile(rc, tile, rect);
        }
      }
    }
  }

  bool render_layers(Pixils::RenderContext& rc,
                     const Roo::sptr_val& tilemap,
                     const Roo::sptr_val& opts)
  {
    if (!can_render(tilemap, opts)) return false;

    RenderInput input = render_input(tilemap, opts);
    auto layers = layers_value(tilemap, opts);
    auto hidden = prop(opts, "hidden-layer-indices");

    std::optional<Pixils::Rect> previous_clip = rc.current_clip_rect;
    bool clipped = false;
    if (input.has_target_rect)
    {
      Pixils::Rect effective_clip = intersect_clip_rect(previous_clip, input.target_rect);
      if (effective_clip.w <= 0 || effective_clip.h <= 0) return true;
      rc.set_clip_rect(effective_clip);
      clipped = true;
    }

    try
    {
      int index = 0;
      for (const auto& layer : Roo::get_children(*layers))
      {
        if (!int_vector_contains(hidden, index))
        {
          render_layer(rc, input, layer);
        }
        index++;
      }
    }
    catch (...)
    {
      if (clipped) rc.set_clip_rect(previous_clip);
      throw;
    }

    if (clipped) rc.set_clip_rect(previous_clip);
    return true;
  }

  namespace Function
  {
    FUNC(RenderLayersBang, render_layers);

    FUNC_IMPL(RenderLayersBang,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&RenderLayersBang::exec_render_layers))));

    EXEC_BODY(RenderLayersBang, exec_render_layers)
    {
      Pixils::RenderContext& rc = Roo::obj<Pixils::RenderContext>(
        *ctx.lookup(Pixils::Script::ID__PIXILS__RENDER_CONTEXT));
      return Roo::Value::boolean(render_layers(rc, args[0], args[1]));
    }
  } // namespace Function

  class RenderImplNamespace : public Roo::Namespace
  {
   public:
    RenderImplNamespace()
      : Roo::Namespace("pixils.tilemap.render-impl")
    {
      values.emplace("render-layers!", Function::RenderLayersBang::make());
    }
  };

  int load_native_package(const RooNativeHostV1* host)
  {
    try
    {
      auto ns = std::make_unique<RenderImplNamespace>();
      ns->set_origin(Roo::Namespace::Origin::native());
      if (host->register_namespace(host->user, ns.release()) != 0)
      {
        return 1;
      }
      return 0;
    }
    catch (const std::exception& e)
    {
      package_last_error = e.what();
      return 1;
    }
  }

  void unload_native_package()
  {
    package_last_error.clear();
  }

  const char* last_error()
  {
    return package_last_error.c_str();
  }
} // namespace

extern "C" ROO_NATIVE_EXPORT const RooNativePackageV1* roo_native_package_v1()
{
  static const RooNativePackageV1 package{
    ROO_NATIVE_ABI_VERSION,
    sizeof(RooNativePackageV1),
    "pixils-tilemap-native",
    "0.1.0",
    ROO_NATIVE_CXX_ABI,
    load_native_package,
    unload_native_package,
    last_error,
  };
  return &package;
}
