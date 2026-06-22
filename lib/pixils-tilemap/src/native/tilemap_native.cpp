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
#include <unordered_map>
#include <unordered_set>
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

  std::string value_key(const Roo::sptr_val& value)
  {
    return nil_value(value) ? "nil" : value->to_string();
  }

  bool same_value(const Roo::sptr_val& a, const Roo::sptr_val& b)
  {
    if (nil_value(a) && nil_value(b)) return true;
    if (nil_value(a) || nil_value(b)) return false;
    return *a == *b;
  }

  Roo::sptr_val map_value(std::initializer_list<Roo::sptr_val> entries)
  {
    return Roo::Value::map(Roo::sptr_val_v(entries));
  }

  void map_set(Roo::sptr_val& map, const std::string& key, const Roo::sptr_val& value)
  {
    Roo::Dict::set_property(map, Roo::keyword(key), value);
  }

  Roo::sptr_val keyword_value(const std::string& name)
  {
    return Roo::keyword(name);
  }

  std::string id_label(const Roo::sptr_val& value)
  {
    std::string text = value_key(value);
    if (!text.empty() && text.front() == ':') return text.substr(1);
    return text;
  }

  Roo::sptr_val render_layer_id_value(const Roo::sptr_val& source_layer_id,
                                      const Roo::sptr_val& tileset)
  {
    return keyword_value(id_label(source_layer_id) + "/" + id_label(tileset));
  }

  std::vector<Roo::sptr_val> seq_children(const Roo::sptr_val& value)
  {
    if (!seq_value(value)) return {};
    return Roo::get_children(*value);
  }

  std::vector<std::vector<Roo::sptr_val>> tile_rows(const Roo::sptr_val& tiles)
  {
    std::vector<std::vector<Roo::sptr_val>> rows;
    for (const auto& row : seq_children(tiles))
    {
      rows.push_back(seq_children(row));
    }
    return rows;
  }

  Roo::sptr_val rows_value(const std::vector<std::vector<Roo::sptr_val>>& rows)
  {
    Roo::sptr_val_v row_values;
    row_values.reserve(rows.size());
    for (const auto& row : rows)
    {
      row_values.push_back(Roo::Value::vector(row));
    }
    return Roo::Value::vector(row_values);
  }

  std::vector<std::vector<Roo::sptr_val>> empty_rows(int width, int height)
  {
    std::vector<std::vector<Roo::sptr_val>> rows;
    rows.reserve(std::max(0, height));
    for (int y = 0; y < height; y++)
    {
      rows.emplace_back(std::max(0, width), Roo::Constant::NIL);
    }
    return rows;
  }

  Roo::sptr_val tile_at(const std::vector<std::vector<Roo::sptr_val>>& rows,
                        int x,
                        int y)
  {
    if (y < 0 || y >= static_cast<int>(rows.size())) return Roo::Constant::NIL;
    const auto& row = rows[y];
    if (x < 0 || x >= static_cast<int>(row.size())) return Roo::Constant::NIL;
    return row[x];
  }

  struct TerrainPreview
  {
    Roo::sptr_val tileset = Roo::Constant::NIL;
    Roo::sptr_val tile = Roo::Constant::NIL;
  };

  struct TerrainSet
  {
    Roo::sptr_val id = Roo::Constant::NIL;
    Roo::sptr_val tileset = Roo::Constant::NIL;
    int unit_w = 1;
    int unit_h = 1;
    std::vector<TerrainPreview> definitions;
    std::unordered_map<std::string, TerrainPreview> by_id;
  };

  struct TileSubstitutionChoice
  {
    Roo::sptr_val tile = Roo::Constant::NIL;
    int weight = 1;
  };

  struct TileSubstitutionRule
  {
    Roo::sptr_val tileset = Roo::Constant::NIL;
    Roo::sptr_val tile = Roo::Constant::NIL;
    Roo::sptr_val algorithm = Roo::Constant::NIL;
    int seed = 0;
    std::vector<TileSubstitutionChoice> choices;
  };

  struct OutputLayer
  {
    Roo::sptr_val source = Roo::Constant::NIL;
    Roo::sptr_val id = Roo::Constant::NIL;
    Roo::sptr_val label = Roo::Constant::NIL;
    Roo::sptr_val target_layer = Roo::Constant::NIL;
    Roo::sptr_val tileset = Roo::Constant::NIL;
    std::vector<std::vector<Roo::sptr_val>> tiles;
  };

  struct TerrainStampRule
  {
    Roo::sptr_val source = Roo::Constant::NIL;
    Roo::sptr_val center = Roo::Constant::NIL;
    Roo::sptr_val output = Roo::Constant::NIL;
    std::unordered_map<std::string, Roo::sptr_val> match;
    std::vector<OutputLayer> output_layers;
    int anchor_x = 0;
    int anchor_y = 0;
    bool exclusive = false;
  };

  struct TerrainStampRuleset
  {
    Roo::sptr_val source = Roo::Constant::NIL;
    Roo::sptr_val id = Roo::Constant::NIL;
    Roo::sptr_val label = Roo::Constant::NIL;
    Roo::sptr_val terrain_set = Roo::Constant::NIL;
    Roo::sptr_val output_tileset = Roo::Constant::NIL;
    Roo::sptr_val source_layer = Roo::Constant::NIL;
    std::vector<Roo::sptr_val> source_layers;
    int unit_w = 1;
    int unit_h = 1;
    std::vector<TerrainStampRule> rules;
  };

  struct GeneratedLayer
  {
    Roo::sptr_val id = Roo::Constant::NIL;
    Roo::sptr_val label = Roo::Constant::NIL;
    Roo::sptr_val tileset = Roo::Constant::NIL;
    Roo::sptr_val source_layer_id = Roo::Constant::NIL;
    Roo::sptr_val ruleset_id = Roo::Constant::NIL;
    std::vector<std::vector<Roo::sptr_val>> tiles;
    std::vector<std::vector<bool>> source_mask;
  };

  int positive_int_prop(const Roo::sptr_val& map,
                        const std::string& key,
                        int fallback)
  {
    return std::max(1, int_prop(map, key, fallback));
  }

  TerrainSet terrain_set_from_value(const Roo::sptr_val& value)
  {
    TerrainSet set;
    set.id = prop(value, "id");
    set.tileset = prop(value, "tileset");
    auto unit = prop(value, "terrain-unit");
    if (nil_value(unit) && (!nil_value(prop(value, "w")) || !nil_value(prop(value, "h"))))
    {
      unit = value;
    }
    if (!nil_value(unit) && unit->type == Roo::Value::Type::MAP)
    {
      set.unit_w = positive_int_prop(unit, "w", 1);
      set.unit_h = positive_int_prop(unit, "h", 1);
    }

    for (const auto& terrain : seq_children(prop(value, "terrains")))
    {
      TerrainPreview preview;
      preview.tileset = nil_value(prop(terrain, "tileset"))
                          ? set.tileset
                          : prop(terrain, "tileset");
      preview.tile = prop(terrain, "tile");
      set.definitions.push_back(preview);
      set.by_id[value_key(prop(terrain, "id"))] = preview;
    }
    return set;
  }

  std::unordered_map<std::string, TerrainSet> terrain_sets_by_id(
    const Roo::sptr_val& terrain_sets_value)
  {
    std::unordered_map<std::string, TerrainSet> out;
    for (const auto& value : seq_children(terrain_sets_value))
    {
      TerrainSet set = terrain_set_from_value(value);
      out[value_key(set.id)] = set;
    }
    return out;
  }

  TerrainPreview terrain_preview(const TerrainSet* terrain_set,
                                 const Roo::sptr_val& terrain_ref)
  {
    if (!terrain_set || nil_value(terrain_ref)) return {};
    auto found = terrain_set->by_id.find(value_key(terrain_ref));
    if (found == terrain_set->by_id.end()) return {};
    return found->second;
  }

  std::vector<Roo::sptr_val> terrain_definition_preview_tilesets(
    const TerrainSet* terrain_set)
  {
    std::vector<Roo::sptr_val> out;
    std::unordered_set<std::string> seen;
    if (!terrain_set) return out;
    for (const auto& preview : terrain_set->definitions)
    {
      if (nil_value(preview.tileset)) continue;
      std::string key = value_key(preview.tileset);
      if (seen.insert(key).second) out.push_back(preview.tileset);
    }
    return out;
  }

  std::unordered_map<std::string, std::unordered_set<std::string>> tile_ids_by_tileset(
    const Roo::sptr_val& tilesets)
  {
    std::unordered_map<std::string, std::unordered_set<std::string>> out;
    for (const auto& tileset : seq_children(tilesets))
    {
      auto tileset_id = prop(tileset, "id");
      auto& tile_ids = out[value_key(tileset_id)];
      for (const auto& tile : seq_children(prop(tileset, "tiles")))
      {
        tile_ids.insert(value_key(prop(tile, "id")));
      }
    }
    return out;
  }

  bool tile_ref_resolves(
    const std::unordered_map<std::string, std::unordered_set<std::string>>& tile_ids,
    const Roo::sptr_val& tileset,
    const Roo::sptr_val& tile)
  {
    auto tileset_found = tile_ids.find(value_key(tileset));
    if (tileset_found == tile_ids.end()) return false;
    return tileset_found->second.contains(value_key(tile));
  }

  Roo::sptr_val terrain_value_at(const std::vector<std::vector<Roo::sptr_val>>& rows,
                                 const TerrainStampRuleset& ruleset,
                                 const std::string& direction,
                                 int x,
                                 int y)
  {
    if (ruleset.unit_w != 1 || ruleset.unit_h != 1) return Roo::Constant::NIL;
    static const std::unordered_map<std::string, std::pair<int, int>> offsets = {
      {"nw", {-1, -1}}, {"n", {0, -1}}, {"ne", {1, -1}}, {"w", {-1, 0}},
      {"e", {1, 0}},    {"sw", {-1, 1}}, {"s", {0, 1}},   {"se", {1, 1}}};
    auto found = offsets.find(direction);
    if (found == offsets.end()) return tile_at(rows, x, y);
    return tile_at(rows, x + found->second.first, y + found->second.second);
  }

  bool known_terrain_value(const Roo::sptr_val& value)
  {
    return !nil_value(value) && value_key(value) != ":terrain/unknown";
  }

  bool condition_matches(const Roo::sptr_val& center,
                         const Roo::sptr_val& condition,
                         const Roo::sptr_val& value)
  {
    if (nil_value(condition) || keyword_named(condition, "ignore")) return true;
    if (keyword_named(condition, "same")) return same_value(value, center);
    if (keyword_named(condition, "none")) return nil_value(value);
    if (keyword_named(condition, "any-terrain")) return known_terrain_value(value);
    if (keyword_named(condition, "not-same") ||
        keyword_named(condition, "other-terrain"))
    {
      return known_terrain_value(value) && !same_value(value, center);
    }
    if (condition && condition->type == Roo::Value::Type::MAP)
    {
      return same_value(value, prop(condition, "terrain"));
    }
    return true;
  }

  bool terrain_rule_matches(const std::vector<std::vector<Roo::sptr_val>>& rows,
                            const TerrainStampRuleset& ruleset,
                            const TerrainStampRule& rule,
                            int x,
                            int y)
  {
    if (!same_value(terrain_value_at(rows, ruleset, "center", x, y), rule.center))
    {
      return false;
    }
    static const std::vector<std::string> directions = {
      "nw", "n", "ne", "w", "e", "sw", "s", "se"};
    for (const auto& direction : directions)
    {
      auto condition = rule.match.find(direction);
      if (condition == rule.match.end()) continue;
      if (!condition_matches(rule.center,
                             condition->second,
                             terrain_value_at(rows, ruleset, direction, x, y)))
      {
        return false;
      }
    }
    return true;
  }

  Roo::sptr_val terrain_match_center(const Roo::sptr_val& rule)
  {
    auto terrain = prop(rule, "terrain");
    if (!nil_value(terrain)) return terrain;
    return prop(prop(prop(rule, "match"), "center"), "terrain");
  }

  OutputLayer output_layer_from_value(const Roo::sptr_val& value)
  {
    OutputLayer layer;
    layer.source = value;
    layer.id = prop(value, "id");
    layer.label = prop(value, "label");
    layer.target_layer = prop(value, "target-layer");
    layer.tileset = prop(value, "tileset");
    layer.tiles = tile_rows(prop(value, "tiles"));
    return layer;
  }

  TerrainStampRule terrain_stamp_rule_from_value(const Roo::sptr_val& value)
  {
    TerrainStampRule rule;
    rule.source = value;
    rule.center = terrain_match_center(value);
    rule.output = prop(value, "output");
    rule.exclusive = truthy_prop(value, "exclusive?");
    auto match = prop(value, "match");
    static const std::vector<std::string> directions = {
      "nw", "n", "ne", "w", "e", "sw", "s", "se"};
    for (const auto& direction : directions)
    {
      auto condition = prop(match, direction);
      if (!nil_value(condition)) rule.match[direction] = condition;
    }
    auto anchor = prop(rule.output, "anchor");
    rule.anchor_x = int_prop(anchor, "x", 0);
    rule.anchor_y = int_prop(anchor, "y", 0);
    for (const auto& layer : seq_children(prop(rule.output, "layers")))
    {
      rule.output_layers.push_back(output_layer_from_value(layer));
    }
    return rule;
  }

  TerrainStampRuleset terrain_stamp_ruleset_from_value(
    const Roo::sptr_val& value,
    const std::unordered_map<std::string, TerrainSet>& terrain_sets)
  {
    TerrainStampRuleset ruleset;
    ruleset.source = value;
    ruleset.id = prop(value, "id");
    ruleset.label = prop(value, "label");
    ruleset.terrain_set = prop(value, "terrain-set");
    ruleset.output_tileset = prop(value, "output-tileset");
    ruleset.source_layer = prop(value, "source-layer");
    for (const auto& source_layer : seq_children(prop(value, "source-layers")))
    {
      ruleset.source_layers.push_back(source_layer);
    }
    auto terrain_set_id = value_key(ruleset.terrain_set);
    auto terrain_set = terrain_sets.find(terrain_set_id);
    if (terrain_set != terrain_sets.end())
    {
      ruleset.unit_w = terrain_set->second.unit_w;
      ruleset.unit_h = terrain_set->second.unit_h;
    }
    for (const auto& rule : seq_children(prop(value, "rules")))
    {
      ruleset.rules.push_back(terrain_stamp_rule_from_value(rule));
    }
    return ruleset;
  }

  std::vector<TerrainStampRuleset> terrain_stamp_rulesets(
    const Roo::sptr_val& rulesets,
    const std::unordered_map<std::string, TerrainSet>& terrain_sets)
  {
    std::vector<TerrainStampRuleset> out;
    for (const auto& ruleset : seq_children(rulesets))
    {
      if (keyword_named(prop(ruleset, "kind"), "terrain-stamp"))
      {
        out.push_back(terrain_stamp_ruleset_from_value(ruleset, terrain_sets));
      }
    }
    return out;
  }

  bool ruleset_applies_to_layer(const TerrainStampRuleset& ruleset,
                                const Roo::sptr_val& layer)
  {
    auto layer_id = prop(layer, "id");
    if (same_value(ruleset.source_layer, layer_id)) return true;
    for (const auto& source_layer : ruleset.source_layers)
    {
      if (same_value(source_layer, layer_id)) return true;
    }
    return nil_value(ruleset.source_layer) && ruleset.source_layers.empty() &&
           keyword_named(prop(layer, "data-kind"), "terrain") &&
           same_value(ruleset.terrain_set, prop(layer, "terrain-set"));
  }

  Roo::sptr_val terrain_stamp_output_tileset(const TerrainStampRuleset& ruleset,
                                             const OutputLayer& output_layer)
  {
    return nil_value(output_layer.tileset) ? ruleset.output_tileset
                                           : output_layer.tileset;
  }

  Roo::sptr_val terrain_stamp_output_layer_key(const Roo::sptr_val& source_layer,
                                               const TerrainStampRuleset& ruleset,
                                               const OutputLayer& output_layer)
  {
    if (!nil_value(output_layer.target_layer)) return output_layer.target_layer;
    Roo::sptr_val suffix = !nil_value(output_layer.id)
                             ? output_layer.id
                             : terrain_stamp_output_tileset(ruleset, output_layer);
    if (nil_value(suffix)) suffix = Roo::string("output");
    return keyword_value(id_label(prop(source_layer, "id")) + "/" +
                         id_label(ruleset.id) + "/" + id_label(suffix));
  }

  GeneratedLayer generated_layer_base(int width,
                                      int height,
                                      const Roo::sptr_val& source_layer,
                                      const TerrainStampRuleset& ruleset,
                                      const OutputLayer& output_layer)
  {
    GeneratedLayer layer;
    layer.id = terrain_stamp_output_layer_key(source_layer, ruleset, output_layer);
    layer.label = !nil_value(output_layer.label)
                    ? output_layer.label
                    : (!nil_value(ruleset.label) ? ruleset.label
                                                 : Roo::string(layer.id->to_string()));
    layer.tileset = terrain_stamp_output_tileset(ruleset, output_layer);
    layer.source_layer_id = prop(source_layer, "id");
    layer.ruleset_id = ruleset.id;
    layer.tiles = empty_rows(width, height);
    layer.source_mask =
      std::vector<std::vector<bool>>(height, std::vector<bool>(width, false));
    return layer;
  }

  int generated_layer_index(const std::vector<GeneratedLayer>& layers,
                            const Roo::sptr_val& layer_id)
  {
    for (int index = 0; index < static_cast<int>(layers.size()); index++)
    {
      if (same_value(layers[index].id, layer_id)) return index;
    }
    return -1;
  }

  GeneratedLayer& ensure_generated_layer(std::vector<GeneratedLayer>& layers,
                                         int width,
                                         int height,
                                         const Roo::sptr_val& source_layer,
                                         const TerrainStampRuleset& ruleset,
                                         const OutputLayer& output_layer)
  {
    auto layer_id = terrain_stamp_output_layer_key(source_layer, ruleset, output_layer);
    int index = generated_layer_index(layers, layer_id);
    if (index < 0)
    {
      layers.push_back(
        generated_layer_base(width, height, source_layer, ruleset, output_layer));
      return layers.back();
    }
    return layers[index];
  }

  bool output_entry_occupies_source(
    bool known_tilesets,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& tile_ids,
    const TerrainStampRuleset& ruleset,
    const OutputLayer& output_layer,
    const Roo::sptr_val& tile)
  {
    bool resolves = true;
    if (!nil_value(tile) && known_tilesets)
    {
      resolves = tile_ref_resolves(
        tile_ids, terrain_stamp_output_tileset(ruleset, output_layer), tile);
    }
    if (!resolves) return false;
    return !nil_value(tile) || ruleset.unit_w != 1 || ruleset.unit_h != 1;
  }

  void apply_output_layer(std::vector<GeneratedLayer>& generated_layers,
                          int width,
                          int height,
                          bool known_tilesets,
                          const std::unordered_map<std::string, std::unordered_set<std::string>>&
                            tile_ids,
                          const Roo::sptr_val& source_layer,
                          const TerrainStampRuleset& ruleset,
                          const TerrainStampRule& rule,
                          const OutputLayer& output_layer,
                          int source_x,
                          int source_y)
  {
    GeneratedLayer& layer = ensure_generated_layer(
      generated_layers, width, height, source_layer, ruleset, output_layer);

    if (ruleset.unit_w == 1 && ruleset.unit_h == 1)
    {
      Roo::sptr_val tile = tile_at(output_layer.tiles, rule.anchor_x, rule.anchor_y);
      int x = source_x;
      int y = source_y;
      if (x >= 0 && x < width && y >= 0 && y < height)
      {
        layer.tiles[y][x] = tile;
        layer.source_mask[y][x] =
          output_entry_occupies_source(known_tilesets, tile_ids, ruleset, output_layer, tile);
      }
      return;
    }

    for (int tile_y = 0; tile_y < static_cast<int>(output_layer.tiles.size()); tile_y++)
    {
      for (int tile_x = 0;
           tile_x < static_cast<int>(output_layer.tiles[tile_y].size());
           tile_x++)
      {
        int x = source_x + tile_x - rule.anchor_x;
        int y = source_y + tile_y - rule.anchor_y;
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        Roo::sptr_val tile = output_layer.tiles[tile_y][tile_x];
        layer.tiles[y][x] = tile;
        layer.source_mask[y][x] =
          output_entry_occupies_source(known_tilesets, tile_ids, ruleset, output_layer, tile);
      }
    }
  }

  std::vector<GeneratedLayer> materialize_terrain_stamp_layers(
    int width,
    int height,
    bool known_tilesets,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& tile_ids,
    const Roo::sptr_val& source_layer,
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const std::vector<TerrainStampRuleset>& rulesets)
  {
    std::vector<GeneratedLayer> generated_layers;
    for (const auto& ruleset : rulesets)
    {
      if (!ruleset_applies_to_layer(ruleset, source_layer)) continue;
      for (int y = 0; y < height; y++)
      {
        for (int x = 0; x < width; x++)
        {
          for (const auto& rule : ruleset.rules)
          {
            if (!terrain_rule_matches(source_rows, ruleset, rule, x, y)) continue;
            for (const auto& output_layer : rule.output_layers)
            {
              apply_output_layer(generated_layers,
                                 width,
                                 height,
                                 known_tilesets,
                                 tile_ids,
                                 source_layer,
                                 ruleset,
                                 rule,
                                 output_layer,
                                 x,
                                 y);
            }
            if (rule.exclusive) break;
          }
        }
      }
    }
    return generated_layers;
  }

  bool source_masked(const std::vector<GeneratedLayer>& generated_layers, int x, int y)
  {
    for (const auto& layer : generated_layers)
    {
      if (y >= 0 && y < static_cast<int>(layer.source_mask.size()) &&
          x >= 0 && x < static_cast<int>(layer.source_mask[y].size()) &&
          layer.source_mask[y][x])
      {
        return true;
      }
    }
    return false;
  }

  std::vector<std::vector<Roo::sptr_val>> mask_terrain_layer(
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const std::vector<GeneratedLayer>& generated_layers)
  {
    auto rows = source_rows;
    for (int y = 0; y < static_cast<int>(rows.size()); y++)
    {
      for (int x = 0; x < static_cast<int>(rows[y].size()); x++)
      {
        if (source_masked(generated_layers, x, y)) rows[y][x] = Roo::Constant::NIL;
      }
    }
    return rows;
  }

  Roo::sptr_val layer_with_tiles(const Roo::sptr_val& source,
                                 const std::vector<std::vector<Roo::sptr_val>>& rows)
  {
    auto layer = Roo::Dict::shallow_copy(source);
    map_set(layer, "tiles", rows_value(rows));
    return layer;
  }

  Roo::sptr_val materialize_terrain_layer_for_tileset(
    const Roo::sptr_val& source_layer,
    const TerrainSet* terrain_set,
    const Roo::sptr_val& tileset,
    bool split,
    const std::vector<std::vector<Roo::sptr_val>>& source_rows)
  {
    std::vector<std::vector<Roo::sptr_val>> rows;
    rows.reserve(source_rows.size());
    for (const auto& row : source_rows)
    {
      std::vector<Roo::sptr_val> out_row;
      out_row.reserve(row.size());
      for (const auto& terrain_ref : row)
      {
        TerrainPreview preview = terrain_preview(terrain_set, terrain_ref);
        out_row.push_back(same_value(preview.tileset, tileset) ? preview.tile
                                                               : Roo::Constant::NIL);
      }
      rows.push_back(std::move(out_row));
    }

    auto layer = layer_with_tiles(source_layer, rows);
    if (split) map_set(layer, "id", render_layer_id_value(prop(source_layer, "id"), tileset));
    map_set(layer, "render-source-layer-id", prop(source_layer, "id"));
    map_set(layer, "data-kind", keyword_value("tile-ref"));
    map_set(layer, "tileset", tileset);
    return layer;
  }

  std::vector<Roo::sptr_val> materialize_terrain_layer(
    const Roo::sptr_val& source_layer,
    const std::unordered_map<std::string, TerrainSet>& terrain_sets,
    const std::vector<std::vector<Roo::sptr_val>>& source_rows)
  {
    auto terrain_set_id = value_key(prop(source_layer, "terrain-set"));
    auto terrain_set_found = terrain_sets.find(terrain_set_id);
    const TerrainSet* terrain_set =
      terrain_set_found == terrain_sets.end() ? nullptr : &terrain_set_found->second;
    auto tilesets = terrain_definition_preview_tilesets(terrain_set);
    bool split = tilesets.size() > 1;
    std::vector<Roo::sptr_val> out;
    if (tilesets.empty())
    {
      auto layer = Roo::Dict::shallow_copy(source_layer);
      map_set(layer, "data-kind", keyword_value("tile-ref"));
      map_set(layer, "tileset", Roo::Constant::NIL);
      out.push_back(layer);
      return out;
    }
    for (const auto& tileset : tilesets)
    {
      out.push_back(materialize_terrain_layer_for_tileset(
        source_layer, terrain_set, tileset, split, source_rows));
    }
    return out;
  }

  Roo::sptr_val generated_layer_value(const GeneratedLayer& layer)
  {
    std::vector<std::vector<Roo::sptr_val>> mask_rows;
    mask_rows.reserve(layer.source_mask.size());
    for (const auto& row : layer.source_mask)
    {
      std::vector<Roo::sptr_val> mask_row;
      mask_row.reserve(row.size());
      for (bool value : row)
      {
        mask_row.push_back(Roo::Value::boolean(value));
      }
      mask_rows.push_back(std::move(mask_row));
    }

    return map_value({keyword_value("id"),
                      layer.id,
                      keyword_value("label"),
                      layer.label,
                      keyword_value("kind"),
                      keyword_value("tile"),
                      keyword_value("role"),
                      keyword_value("derived"),
                      keyword_value("data-kind"),
                      keyword_value("tile-ref"),
                      keyword_value("tileset"),
                      layer.tileset,
                      keyword_value("render-source-layer-id"),
                      layer.source_layer_id,
                      keyword_value("ruleset"),
                      layer.ruleset_id,
                      keyword_value("replaces-source?"),
                      Roo::Value::boolean(true),
                      keyword_value("tiles"),
                      rows_value(layer.tiles),
                      keyword_value("source-mask"),
                      rows_value(mask_rows)});
  }

  std::vector<Roo::sptr_val> materialize_layer_with_rules(
    int width,
    int height,
    bool show_rules,
    bool known_tilesets,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& tile_ids,
    const Roo::sptr_val& layer,
    const std::unordered_map<std::string, TerrainSet>& terrain_sets,
    const std::vector<TerrainStampRuleset>& rulesets)
  {
    bool terrain_layer = keyword_named(prop(layer, "data-kind"), "terrain");
    auto source_rows = tile_rows(prop(layer, "tiles"));
    std::vector<GeneratedLayer> generated_layers =
      (show_rules && terrain_layer)
        ? materialize_terrain_stamp_layers(width,
                                           height,
                                           known_tilesets,
                                           tile_ids,
                                           layer,
                                           source_rows,
                                           rulesets)
        : std::vector<GeneratedLayer>{};

    std::vector<Roo::sptr_val> out;
    if (terrain_layer)
    {
      auto masked_rows = mask_terrain_layer(source_rows, generated_layers);
      auto terrain_layers = materialize_terrain_layer(layer, terrain_sets, masked_rows);
      out.insert(out.end(), terrain_layers.begin(), terrain_layers.end());
    }
    else
    {
      auto copied = Roo::Dict::shallow_copy(layer);
      if (nil_value(prop(copied, "data-kind")))
      {
        map_set(copied, "data-kind", keyword_value("tile-ref"));
      }
      out.push_back(copied);
    }
    for (const auto& generated_layer : generated_layers)
    {
      out.push_back(generated_layer_value(generated_layer));
    }
    return out;
  }

  int positive_mod(long value, int divisor)
  {
    if (divisor == 0) return 0;
    long mod = value % divisor;
    return static_cast<int>((mod + divisor) % divisor);
  }

  int stable_tile_substitution_bucket(int seed, int x, int y, int total)
  {
    const int hash_mod = 1000003;
    long source = positive_mod((seed + 1L) * 374761L + (x + 1L) * 668263L +
                                 (y + 1L) * 982451L +
                                 (x + 3L) * (y + 5L) * 154858L,
                               hash_mod);
    long mixed = positive_mod(source * (source + 127417L) +
                                (x + 1L) * (x + 7L) * 524287L +
                                (y + 1L) * (y + 11L) * 8191L,
                              hash_mod);
    return positive_mod(mixed, total);
  }

  std::vector<TileSubstitutionChoice> substitution_choices(const Roo::sptr_val& rule)
  {
    std::vector<TileSubstitutionChoice> out;
    for (const auto& choice : seq_children(prop(rule, "choices")))
    {
      TileSubstitutionChoice out_choice;
      out_choice.tile = prop(choice, "tile");
      out_choice.weight = int_prop(choice, "weight", 1);
      if (!nil_value(out_choice.tile) && out_choice.weight > 0)
      {
        out.push_back(out_choice);
      }
    }
    return out;
  }

  std::optional<TileSubstitutionRule> substitution_rule_from_config(
    const Roo::sptr_val& tileset,
    const Roo::sptr_val& tile,
    const Roo::sptr_val& config)
  {
    if (!config || config->type != Roo::Value::Type::MAP) return std::nullopt;
    TileSubstitutionRule rule;
    rule.tileset = tileset;
    rule.tile = tile;
    rule.algorithm = prop(config, "algorithm");
    rule.seed = int_prop(config, "seed", 0);
    rule.choices = substitution_choices(config);
    if (rule.choices.empty()) return std::nullopt;
    return rule;
  }

  Roo::sptr_val tile_substitution_config(const Roo::sptr_val& tile)
  {
    auto config = prop(tile, "substitution");
    if (!nil_value(config)) return config;
    config = prop(tile, "tile-substitution");
    if (!nil_value(config)) return config;
    return prop(tile, "substitutions");
  }

  std::vector<TileSubstitutionRule> materialize_tile_substitution_rules(
    const Roo::sptr_val& tilemap,
    const Roo::sptr_val& opts)
  {
    std::vector<TileSubstitutionRule> out;
    auto explicit_rules = prop(opts, "tile-substitution-rules");
    if (nil_value(explicit_rules)) explicit_rules = prop(tilemap, "tile-substitution-rules");
    for (const auto& rule_value : seq_children(explicit_rules))
    {
      TileSubstitutionRule rule;
      rule.tileset = prop(rule_value, "tileset");
      rule.tile = nil_value(prop(rule_value, "tile"))
                    ? prop(rule_value, "source-tile")
                    : prop(rule_value, "tile");
      rule.algorithm = prop(rule_value, "algorithm");
      rule.seed = int_prop(rule_value, "seed", 0);
      rule.choices = substitution_choices(rule_value);
      out.push_back(rule);
    }

    auto tilesets = prop(opts, "tilesets");
    if (nil_value(tilesets)) tilesets = prop(tilemap, "tilesets");
    for (const auto& tileset : seq_children(tilesets))
    {
      for (const auto& tile : seq_children(prop(tileset, "tiles")))
      {
        if (auto rule = substitution_rule_from_config(
              prop(tileset, "id"), prop(tile, "id"), tile_substitution_config(tile)))
        {
          out.push_back(*rule);
        }
      }
    }
    return out;
  }

  Roo::sptr_val apply_substitution_rule(const TileSubstitutionRule& rule,
                                        const Roo::sptr_val& tile_ref,
                                        int x,
                                        int y)
  {
    if (!nil_value(rule.algorithm) && !keyword_named(rule.algorithm, "weighted-random"))
    {
      return tile_ref;
    }
    int total = 0;
    for (const auto& choice : rule.choices) total += choice.weight;
    if (total <= 0) return tile_ref;
    int bucket = stable_tile_substitution_bucket(rule.seed, x, y, total);
    for (const auto& choice : rule.choices)
    {
      if (bucket < choice.weight) return choice.tile;
      bucket -= choice.weight;
    }
    return tile_ref;
  }

  Roo::sptr_val apply_substitution_rules_to_cell(
    const std::vector<TileSubstitutionRule>& rules,
    const Roo::sptr_val& layer,
    const Roo::sptr_val& tile_ref,
    int x,
    int y)
  {
    if (nil_value(tile_ref)) return tile_ref;
    auto layer_tileset = prop(layer, "tileset");
    for (const auto& rule : rules)
    {
      if (same_value(layer_tileset, rule.tileset) && same_value(tile_ref, rule.tile))
      {
        return apply_substitution_rule(rule, tile_ref, x, y);
      }
    }
    return tile_ref;
  }

  Roo::sptr_val apply_substitution_rules_to_layer(
    const std::vector<TileSubstitutionRule>& rules,
    const Roo::sptr_val& layer)
  {
    if (!keyword_named(prop(layer, "data-kind"), "tile-ref")) return layer;
    auto rows = tile_rows(prop(layer, "tiles"));
    for (int y = 0; y < static_cast<int>(rows.size()); y++)
    {
      for (int x = 0; x < static_cast<int>(rows[y].size()); x++)
      {
        rows[y][x] = apply_substitution_rules_to_cell(rules, layer, rows[y][x], x, y);
      }
    }
    return layer_with_tiles(layer, rows);
  }

  bool show_terrain_rules(const Roo::sptr_val& tilemap, const Roo::sptr_val& opts)
  {
    auto value = prop(opts, "show-terrain-rules?");
    if (nil_value(value)) value = prop(tilemap, "show-terrain-rules?");
    if (nil_value(value)) return true;
    return Roo::is_truthy(*value);
  }

  Roo::sptr_val native_materialize_render_map(const Roo::sptr_val& tilemap,
                                              const Roo::sptr_val& opts)
  {
    if (!tilemap || tilemap->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;
    int width = int_prop(tilemap, "width", 0);
    int height = int_prop(tilemap, "height", 0);
    if (width < 0 || height < 0) return Roo::Constant::NIL;

    auto source_layers_value = prop(opts, "layers");
    if (nil_value(source_layers_value)) source_layers_value = prop(tilemap, "layers");
    auto terrain_sets_value = prop(opts, "terrain-sets");
    if (nil_value(terrain_sets_value)) terrain_sets_value = prop(tilemap, "terrain-sets");
    auto rulesets_value = prop(opts, "rulesets");
    if (nil_value(rulesets_value)) rulesets_value = prop(tilemap, "rulesets");

    auto terrain_sets = terrain_sets_by_id(terrain_sets_value);
    auto tile_ids = tile_ids_by_tileset(prop(tilemap, "tilesets"));
    bool known_tilesets = !nil_value(prop(tilemap, "tilesets"));
    auto rulesets = terrain_stamp_rulesets(rulesets_value, terrain_sets);
    auto substitution_rules = materialize_tile_substitution_rules(tilemap, opts);

    std::vector<Roo::sptr_val> render_layers;
    for (const auto& layer : seq_children(source_layers_value))
    {
      auto layers = materialize_layer_with_rules(width,
                                                 height,
                                                 show_terrain_rules(tilemap, opts),
                                                 known_tilesets,
                                                 tile_ids,
                                                 layer,
                                                 terrain_sets,
                                                 rulesets);
      render_layers.insert(render_layers.end(), layers.begin(), layers.end());
    }

    if (!substitution_rules.empty())
    {
      for (auto& layer : render_layers)
      {
        layer = apply_substitution_rules_to_layer(substitution_rules, layer);
      }
    }

    auto result = map_value({keyword_value("width"),
                             Roo::Value::number(width),
                             keyword_value("height"),
                             Roo::Value::number(height),
                             keyword_value("tile-size"),
                             prop(tilemap, "tile-size"),
                             keyword_value("layers"),
                             Roo::Value::vector(render_layers)});
    return result;
  }

  namespace Function
  {
    FUNC(RenderLayersBang, render_layers);
    FUNC(MaterializeRenderMap, native_materialize_render_map);

    FUNC_IMPL(RenderLayersBang,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&RenderLayersBang::exec_render_layers))));

    FUNC_IMPL(MaterializeRenderMap,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&MaterializeRenderMap::exec_native_materialize_render_map))));

    EXEC_BODY(RenderLayersBang, exec_render_layers)
    {
      Pixils::RenderContext& rc = Roo::obj<Pixils::RenderContext>(
        *ctx.lookup(Pixils::Script::ID__PIXILS__RENDER_CONTEXT));
      return Roo::Value::boolean(render_layers(rc, args[0], args[1]));
    }

    EXEC_BODY(MaterializeRenderMap, exec_native_materialize_render_map)
    {
      return native_materialize_render_map(args[0], args[1]);
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

  class MaterializeImplNamespace : public Roo::Namespace
  {
   public:
    MaterializeImplNamespace()
      : Roo::Namespace("pixils.tilemap.materialize-impl")
    {
      values.emplace("materialize-render-map", Function::MaterializeRenderMap::make());
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
      auto materialize_ns = std::make_unique<MaterializeImplNamespace>();
      materialize_ns->set_origin(Roo::Namespace::Origin::native());
      if (host->register_namespace(host->user, materialize_ns.release()) != 0)
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
