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

  bool seq_value(const Roo::sptr_val& value)
  {
    return value &&
           (value->type == Roo::Value::Type::VECTOR ||
            value->type == Roo::Value::Type::LIST);
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

  struct DrawRangePadding
  {
    int left = 0;
    int right = 0;
    int top = 0;
    int bottom = 0;
  };

  struct TileDim
  {
    int w = 16;
    int h = 16;
  };

  TileDim tile_dim_value(const Roo::sptr_val& value, int fallback)
  {
    if (nil_value(value)) return TileDim{fallback, fallback};
    if (value->type == Roo::Value::Type::NUMBER)
    {
      int size = std::max(1, value->num().get_int());
      return TileDim{size, size};
    }
    if (value->type == Roo::Value::Type::MAP)
    {
      return TileDim{std::max(1, int_prop(value, "w", fallback)),
                     std::max(1, int_prop(value, "h", fallback))};
    }
    throw Roo::TypeError(":tile-size must be a number or dimension map");
  }

  struct RenderInput
  {
    int map_width = 0;
    int map_height = 0;
    TileDim tile_size;
    double zoom = 1.0;
    Pixils::Rect target_rect{0, 0, 0, 0};
    bool has_target_rect = false;
    Pixils::Rect offset{0, 0, 0, 0};
    Pixils::Rect render_offset{0, 0, 0, 0};
    RenderRanges ranges;
  };

  TileDim scaled_tile_size(const RenderInput& input)
  {
    return TileDim{
      std::max(1, static_cast<int>(std::round(input.tile_size.w * input.zoom))),
      std::max(1, static_cast<int>(std::round(input.tile_size.h * input.zoom)))};
  }

  RenderRanges render_ranges(const RenderInput& input)
  {
    TileDim size = scaled_tile_size(input);
    Pixils::Rect rect =
      input.has_target_rect
        ? input.target_rect
        : Pixils::Rect{0, 0, input.map_width * size.w, input.map_height * size.h};
    return RenderRanges{render_axis_range(rect.w, input.offset.x, size.w, input.map_width),
                        render_axis_range(rect.h, input.offset.y, size.h, input.map_height)};
  }

  RenderInput render_input(const Roo::sptr_val& tilemap, const Roo::sptr_val& opts)
  {
    RenderInput input;
    input.map_width = int_prop(tilemap, "width", 0);
    input.map_height = int_prop(tilemap, "height", 0);
    input.tile_size = tile_dim_value(prop(tilemap, "tile-size"), 16);
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
    if (!seq_value(values)) return false;
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
    if (!seq_value(layers)) return false;

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
    if (!seq_value(tiles)) return Roo::Constant::NIL;
    auto rows = Roo::get_children(*tiles);
    if (y < 0 || y >= static_cast<int>(rows.size())) return Roo::Constant::NIL;
    auto row = rows[y];
    if (!seq_value(row)) return Roo::Constant::NIL;
    auto cells = Roo::get_children(*row);
    if (x < 0 || x >= static_cast<int>(cells.size())) return Roo::Constant::NIL;
    return cells[x];
  }

  std::vector<Roo::sptr_val> tile_stack(const Roo::sptr_val& cell)
  {
    if (!seq_value(cell)) return {};
    return Roo::get_children(*cell);
  }

  Pixils::Rect tile_rect(const RenderInput& input, int x, int y)
  {
    TileDim size = scaled_tile_size(input);
    return Pixils::Rect{(x * size.w) - input.render_offset.x,
                        (y * size.h) - input.render_offset.y,
                        size.w,
                        size.h};
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

  bool explicit_draw_geometry(const Roo::sptr_val& tile)
  {
    return !nil_value(prop(tile, "draw-offset")) || !nil_value(prop(tile, "draw-size"));
  }

  TileDim tile_draw_size(const Roo::sptr_val& tile, int source_w, int source_h)
  {
    auto size = prop(tile, "draw-size");
    if (!nil_value(size) && size->type == Roo::Value::Type::MAP)
    {
      return TileDim{std::max(1, int_prop(size, "w", source_w)),
                     std::max(1, int_prop(size, "h", source_h))};
    }
    return TileDim{std::max(1, source_w), std::max(1, source_h)};
  }

  SDL_Rect explicit_draw_dest(const Roo::sptr_val& tile,
                              const Pixils::Rect& rect,
                              int source_w,
                              int source_h,
                              double zoom)
  {
    auto offset = prop(tile, "draw-offset");
    int offset_x = 0;
    int offset_y = 0;
    if (!nil_value(offset) && offset->type == Roo::Value::Type::MAP)
    {
      offset_x = int_prop(offset, "x", 0);
      offset_y = int_prop(offset, "y", 0);
    }

    TileDim size = tile_draw_size(tile, source_w, source_h);
    return SDL_Rect{
      rect.x + static_cast<int>(std::round(offset_x * zoom)),
      rect.y + static_cast<int>(std::round(offset_y * zoom)),
      static_cast<int>(std::round(size.w * zoom)),
      static_cast<int>(std::round(size.h * zoom))};
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
    if (source->type != Roo::Value::Type::MAP) return std::nullopt;
    Pixils::Rect rect = rect_from_map(source);
    if (rect.w <= 0 || rect.h <= 0) return std::nullopt;
    return rect.to_SDL_rect();
  }

  DrawRangePadding max_padding(const DrawRangePadding& a,
                               const DrawRangePadding& b)
  {
    return DrawRangePadding{std::max(a.left, b.left),
                            std::max(a.right, b.right),
                            std::max(a.top, b.top),
                            std::max(a.bottom, b.bottom)};
  }

  bool tile_source_size(Pixils::RenderContext& rc,
                        const Roo::sptr_val& tile,
                        int* source_w,
                        int* source_h)
  {
    if (auto source = source_rect(tile))
    {
      *source_w = source->w;
      *source_h = source->h;
      return true;
    }

    auto draw_size = prop(tile, "draw-size");
    if (!nil_value(draw_size) && draw_size->type == Roo::Value::Type::MAP)
    {
      *source_w = int_prop(draw_size, "w", 0);
      *source_h = int_prop(draw_size, "h", 0);
      return *source_w > 0 && *source_h > 0;
    }

    std::string bundle;
    std::string asset;
    if (!image_key(tile, &bundle, &asset) || !rc.asset_registry) return false;
    SDL_Texture* texture = rc.asset_registry->get_image(bundle, asset);
    if (!texture) return false;
    SDL_QueryTexture(texture, nullptr, nullptr, source_w, source_h);
    return *source_w > 0 && *source_h > 0;
  }

  DrawRangePadding draw_range_padding_for_tile(Pixils::RenderContext& rc,
                                               const Roo::sptr_val& tile,
                                               const RenderInput& input)
  {
    if (!tile || tile->type != Roo::Value::Type::MAP ||
        !explicit_draw_geometry(tile))
    {
      return {};
    }

    std::string type = value_name(prop(tile, "type"));
    if (type != "sprite" && type != "image") return {};

    int source_w = 0;
    int source_h = 0;
    if (!tile_source_size(rc, tile, &source_w, &source_h)) return {};

    TileDim size = tile_draw_size(tile, source_w, source_h);
    auto offset = prop(tile, "draw-offset");
    int offset_x = 0;
    int offset_y = 0;
    if (!nil_value(offset) && offset->type == Roo::Value::Type::MAP)
    {
      offset_x = int_prop(offset, "x", 0);
      offset_y = int_prop(offset, "y", 0);
    }

    return DrawRangePadding{
      ceil_positive_div(std::max(0, -offset_x), input.tile_size.w),
      ceil_positive_div(std::max(0, (offset_x + size.w) - input.tile_size.w),
                        input.tile_size.w),
      ceil_positive_div(std::max(0, -offset_y), input.tile_size.h),
      ceil_positive_div(std::max(0, (offset_y + size.h) - input.tile_size.h),
                        input.tile_size.h)};
  }

  DrawRangePadding draw_range_padding_for_layers(Pixils::RenderContext& rc,
                                                 const Roo::sptr_val& layers,
                                                 const Roo::sptr_val& hidden,
                                                 const RenderInput& input)
  {
    if (!seq_value(layers)) return {};

    DrawRangePadding padding;
    int index = 0;
    for (const auto& layer : Roo::get_children(*layers))
    {
      if (int_vector_contains(hidden, index))
      {
        index++;
        continue;
      }

      auto tiles = prop(layer, "tiles");
      if (seq_value(tiles))
      {
        for (const auto& row : Roo::get_children(*tiles))
        {
          if (!seq_value(row)) continue;
          for (const auto& cell : Roo::get_children(*row))
          {
            for (const auto& tile : tile_stack(cell))
            {
              padding = max_padding(
                padding,
                draw_range_padding_for_tile(rc, tile, input));
            }
          }
        }
      }
      index++;
    }
    return padding;
  }

  void pad_axis_range(AxisRange* range, int before, int after, int max_count)
  {
    range->start = std::max(0, range->start - before);
    range->end = std::min(max_count, range->end + after);
  }

  void pad_render_ranges(RenderInput* input, const DrawRangePadding& padding)
  {
    pad_axis_range(&input->ranges.x, padding.left, padding.right, input->map_width);
    pad_axis_range(&input->ranges.y, padding.top, padding.bottom, input->map_height);
  }

  void fill_rect(SDL_Renderer* renderer,
                 const Pixils::Rect& rect,
                 Uint8 r,
                 Uint8 g,
                 Uint8 b,
                 Uint8 a)
  {
    if (!renderer) return;
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
                         bool image_background,
                         double zoom)
  {
    std::string bundle;
    std::string asset;
    if (!image_key(tile, &bundle, &asset))
    {
      draw_missing_tile(rc, rect);
      return;
    }

    if (!rc.asset_registry) return;
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

    SDL_Rect dest = explicit_draw_geometry(tile)
                      ? explicit_draw_dest(tile, rect, source_w, source_h, zoom)
                      : centered_dest(rect, source_w, source_h);
    const SDL_Rect* source_ptr = source ? &*source : nullptr;
    SDL_RenderCopy(rc.renderer, texture, source_ptr, &dest);
  }

  void draw_tile(Pixils::RenderContext& rc,
                 const Roo::sptr_val& tile,
                 const Pixils::Rect& rect,
                 double zoom)
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
      draw_texture_tile(rc, tile, rect, true, zoom);
    }
    else if (type == "sprite")
    {
      draw_texture_tile(rc, tile, rect, false, zoom);
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
          draw_tile(rc, tile, rect, input.zoom);
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
    if (nil_value(layers)) return true;
    if (!seq_value(layers)) return false;
    pad_render_ranges(&input,
                      draw_range_padding_for_layers(rc, layers, hidden, input));

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
