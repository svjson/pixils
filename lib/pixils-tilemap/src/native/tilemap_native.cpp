#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/color.h>
#include <pixils/context.h>
#include <pixils/geom.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_version.h>
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

  void draw_tile(Pixils::RenderContext& rc,
                 const Roo::sptr_val& tile,
                 const Pixils::Rect& rect,
                 double zoom);

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

  struct FloatRect
  {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;

    SDL_FRect to_SDL_frect() const
    {
      return SDL_FRect{static_cast<float>(x),
                       static_cast<float>(y),
                       static_cast<float>(w),
                       static_cast<float>(h)};
    }
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

  double scaled_tile_width(const RenderInput& input)
  {
    return static_cast<double>(input.tile_size.w) * input.zoom;
  }

  double scaled_tile_height(const RenderInput& input)
  {
    return static_cast<double>(input.tile_size.h) * input.zoom;
  }

  bool effectively_integer(double value)
  {
    return std::abs(value - std::round(value)) < 0.000001;
  }

  bool has_fractional_scaled_tile_size(const RenderInput& input)
  {
    return !effectively_integer(scaled_tile_width(input)) ||
           !effectively_integer(scaled_tile_height(input));
  }

  double integer_scaled_tile_zoom(const RenderInput& input)
  {
    double width_zoom =
      std::ceil(scaled_tile_width(input)) / static_cast<double>(input.tile_size.w);
    double height_zoom =
      std::ceil(scaled_tile_height(input)) / static_cast<double>(input.tile_size.h);
    return std::max(width_zoom, height_zoom);
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

  std::pair<std::string, std::string> resource_key(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return {"", ""};
    auto [bundle, asset] = value->qual();
    return {bundle, asset};
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

  struct MaskGrid
  {
    int left_w = 0;
    int middle_w = 0;
    int right_w = 0;
    int top_h = 0;
    int middle_h = 0;
    int bottom_h = 0;
    int middle_x = 0;
    int right_x = 0;
    int middle_y = 0;
    int bottom_y = 0;
  };

  MaskGrid mask_grid(const Pixils::Rect& rect)
  {
    MaskGrid grid;
    grid.left_w = rect.w / 3;
    grid.right_w = rect.w / 3;
    grid.middle_w = rect.w - grid.left_w - grid.right_w;
    grid.top_h = rect.h / 3;
    grid.bottom_h = rect.h / 3;
    grid.middle_h = rect.h - grid.top_h - grid.bottom_h;
    grid.middle_x = rect.x + grid.left_w;
    grid.right_x = grid.middle_x + grid.middle_w;
    grid.middle_y = rect.y + grid.top_h;
    grid.bottom_y = grid.middle_y + grid.middle_h;
    return grid;
  }

  Pixils::Rect edge_mask_rect(const std::string& part, const Pixils::Rect& rect)
  {
    MaskGrid grid = mask_grid(rect);

    if (part == "w") return Pixils::Rect{rect.x, grid.middle_y, grid.left_w, grid.middle_h};
    if (part == "e")
    {
      return Pixils::Rect{grid.right_x, grid.middle_y, grid.right_w, grid.middle_h};
    }
    if (part == "n") return Pixils::Rect{grid.middle_x, rect.y, grid.middle_w, grid.top_h};
    if (part == "s")
    {
      return Pixils::Rect{grid.middle_x, grid.bottom_y, grid.middle_w, grid.bottom_h};
    }

    return rect;
  }

  Pixils::Rect corner_mask_rect(const std::string& part, const Pixils::Rect& rect)
  {
    MaskGrid grid = mask_grid(rect);

    if (part == "nw") return Pixils::Rect{rect.x, rect.y, grid.left_w, grid.top_h};
    if (part == "ne") return Pixils::Rect{grid.right_x, rect.y, grid.right_w, grid.top_h};
    if (part == "sw") return Pixils::Rect{rect.x, grid.bottom_y, grid.left_w, grid.bottom_h};
    if (part == "se")
    {
      return Pixils::Rect{grid.right_x, grid.bottom_y, grid.right_w, grid.bottom_h};
    }

    return rect;
  }

  Pixils::Rect center_mask_rect(const std::string& part, const Pixils::Rect& rect)
  {
    MaskGrid grid = mask_grid(rect);

    if (part == "center")
    {
      return Pixils::Rect{grid.middle_x, grid.middle_y, grid.middle_w, grid.middle_h};
    }

    return rect;
  }

  Pixils::Rect quadrant_mask_rect(const std::string& part, const Pixils::Rect& rect)
  {
    int left_w = rect.w / 2;
    int right_w = rect.w - left_w;
    int top_h = rect.h / 2;
    int bottom_h = rect.h - top_h;
    int right_x = rect.x + left_w;
    int bottom_y = rect.y + top_h;

    if (part == "nw") return Pixils::Rect{rect.x, rect.y, left_w, top_h};
    if (part == "ne") return Pixils::Rect{right_x, rect.y, right_w, top_h};
    if (part == "sw") return Pixils::Rect{rect.x, bottom_y, left_w, bottom_h};
    if (part == "se") return Pixils::Rect{right_x, bottom_y, right_w, bottom_h};

    return rect;
  }

  Pixils::Rect tile_mask_rect(const Roo::sptr_val& tile, const Pixils::Rect& rect)
  {
    auto mask = prop(tile, "mask");
    if (nil_value(mask)) return rect;

    std::string kind = value_name(prop(mask, "kind"));
    std::string part = value_name(prop(mask, "part"));

    if (kind == "edges") return edge_mask_rect(part, rect);
    if (kind == "corners") return corner_mask_rect(part, rect);
    if (kind == "center") return center_mask_rect(part, rect);
    if (kind == "quadrants") return quadrant_mask_rect(part, rect);

    return rect;
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
                tile_mask_rect(tile, rect),
                native_color.r,
                native_color.g,
                native_color.b,
                native_color.a);
      return;
    }

    fill_rect(rc.renderer,
              tile_mask_rect(tile, rect),
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

  SDL_BlendMode erase_alpha_blend_mode()
  {
    static SDL_BlendMode mode =
      SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO,
                                 SDL_BLENDFACTOR_ONE,
                                 SDL_BLENDOPERATION_ADD,
                                 SDL_BLENDFACTOR_ZERO,
                                 SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                 SDL_BLENDOPERATION_ADD);
    return mode;
  }

  void draw_color_tile_with_blend(Pixils::RenderContext& rc,
                                  const Roo::sptr_val& tile,
                                  const Pixils::Rect& rect,
                                  SDL_BlendMode blend_mode)
  {
    SDL_BlendMode previous = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(rc.renderer, &previous);
    SDL_SetRenderDrawBlendMode(rc.renderer, blend_mode);
    draw_color_tile(rc, tile, rect);
    SDL_SetRenderDrawBlendMode(rc.renderer, previous);
  }

  void draw_texture_source_to(Pixils::RenderContext& rc,
                              const Roo::sptr_val& tile,
                              const Pixils::Rect& rect,
                              SDL_BlendMode blend_mode)
  {
    std::string bundle;
    std::string asset;
    if (!image_key(tile, &bundle, &asset) || !rc.asset_registry) return;
    SDL_Texture* texture = rc.asset_registry->get_image(bundle, asset);
    if (!texture) return;

    std::optional<SDL_Rect> source = source_rect(tile);
    const SDL_Rect dest = rect.to_SDL_rect();
    const SDL_Rect* source_ptr = source ? &*source : nullptr;

    SDL_BlendMode previous = SDL_BLENDMODE_NONE;
    SDL_GetTextureBlendMode(texture, &previous);
    SDL_SetTextureBlendMode(texture, blend_mode);
    SDL_RenderCopy(rc.renderer, texture, source_ptr, &dest);
    SDL_SetTextureBlendMode(texture, previous);
  }

  TileDim transition_source_tile_dim(Pixils::RenderContext& rc,
                                     const Roo::sptr_val& tile,
                                     const TileDim& fallback)
  {
    std::string type = value_name(prop(tile, "type"));
    if (type == "sprite" || type == "image")
    {
      int w = 0;
      int h = 0;
      if (tile_source_size(rc, tile, &w, &h)) return TileDim{w, h};
    }
    return fallback;
  }

  void draw_transition_mask_source_item(Pixils::RenderContext& rc,
                                        const Roo::sptr_val& item,
                                        const TileDim& size)
  {
    auto tile = prop(item, "tile-definition");
    if (nil_value(tile) || tile->type != Roo::Value::Type::MAP) return;

    auto offset = prop(item, "offset");
    int offset_x = 0;
    int offset_y = 0;
    if (!nil_value(offset) && offset->type == Roo::Value::Type::MAP)
    {
      offset_x = int_prop(offset, "x", 0);
      offset_y = int_prop(offset, "y", 0);
    }

    TileDim dim = transition_source_tile_dim(rc, tile, size);
    Pixils::Rect target{offset_x, offset_y, dim.w, dim.h};
    std::string type = value_name(prop(tile, "type"));
    if (type == "color")
    {
      draw_color_tile_with_blend(rc, tile, target, erase_alpha_blend_mode());
    }
    else if (type == "sprite" || type == "image")
    {
      draw_texture_source_to(rc, tile, target, erase_alpha_blend_mode());
    }
  }

  void draw_transition_overlay_image(Pixils::RenderContext& rc,
                                     const Roo::sptr_val& tile,
                                     const TileDim& size)
  {
    auto overlay = prop(prop(tile, "overlay"), "tile-definition");
    if (!nil_value(overlay))
    {
      draw_tile(rc, overlay, Pixils::Rect{0, 0, size.w, size.h}, 1.0);
    }

    auto mask_items = prop(prop(tile, "mask-source"), "tiles");
    if (seq_value(mask_items))
    {
      for (const auto& item : mask_items->elements())
      {
        draw_transition_mask_source_item(rc, item, size);
      }
    }
  }

  SDL_Texture* ensure_transition_overlay_texture(Pixils::RenderContext& rc,
                                                 const Roo::sptr_val& tile,
                                                 const TileDim& size)
  {
    if (!rc.renderer || !rc.asset_registry || size.w <= 0 || size.h <= 0) return nullptr;

    auto [bundle, asset] = resource_key(prop(tile, "image"));
    if (bundle.empty() || asset.empty()) return nullptr;

    if (SDL_Texture* existing = rc.asset_registry->get_image(bundle, asset))
    {
      return existing;
    }

    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
      SDL_CreateTexture(rc.renderer,
                        SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET,
                        size.w,
                        size.h),
      SDL_DestroyTexture);
    if (!texture) return nullptr;

    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);

    SDL_Texture* previous_target = rc.current_render_target;
    auto previous_clip = rc.current_clip_rect;
    rc.set_render_target(texture.get());
    rc.set_clip_rect(std::nullopt);
    SDL_SetRenderDrawColor(rc.renderer, 0, 0, 0, 0);
    SDL_RenderClear(rc.renderer);
    SDL_SetRenderDrawColor(rc.renderer, 0xff, 0xff, 0xff, 0xff);

    draw_transition_overlay_image(rc, tile, size);

    rc.set_render_target(previous_target);
    rc.set_clip_rect(previous_clip);

    try
    {
      rc.asset_registry->create_dynamic_bundle(bundle);
      SDL_Texture* committed = texture.get();
      rc.asset_registry->add_generated_image(bundle,
                                             asset,
                                             committed,
                                             nullptr,
                                             Pixils::Dimension{size.w, size.h});
      texture.release();
      return committed;
    }
    catch (...)
    {
      return nullptr;
    }
  }

  TileDim transition_tile_size(const Pixils::Rect& rect, double zoom)
  {
    return TileDim{std::max(1, static_cast<int>(std::round(rect.w / zoom))),
                   std::max(1, static_cast<int>(std::round(rect.h / zoom)))};
  }

  void draw_transition_mask_tile(Pixils::RenderContext& rc,
                                 const Roo::sptr_val& tile,
                                 const Pixils::Rect& rect,
                                 double zoom)
  {
    auto base = prop(prop(tile, "base"), "tile-definition");
    auto overlay = prop(prop(tile, "overlay"), "tile-definition");
    if (!nil_value(base))
    {
      draw_tile(rc, base, rect, zoom);
    }

    SDL_Texture* image = ensure_transition_overlay_texture(
      rc, tile, transition_tile_size(rect, zoom <= 0.0 ? 1.0 : zoom));
    if (image)
    {
      SDL_Rect dest = rect.to_SDL_rect();
      SDL_RenderCopy(rc.renderer, image, nullptr, &dest);
    }
    else if (!nil_value(overlay))
    {
      draw_tile(rc, overlay, rect, zoom);
    }
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
    else if (type == "transition-mask")
    {
      draw_transition_mask_tile(rc, tile, rect, zoom);
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

  constexpr const char* FRACTIONAL_RENDER_CACHE_BUNDLE =
    "pixils-tilemap-fractional-render-cache";
  constexpr const char* FRACTIONAL_RENDER_CACHE_ASSET = "viewport";
  constexpr int FRACTIONAL_RENDER_CACHE_BUCKET_SIZE = 64;

  struct FractionalRenderCacheTexture
  {
    SDL_Texture* texture = nullptr;
    int w = 0;
    int h = 0;

    explicit operator bool() const { return texture != nullptr; }
  };

  struct FractionalRenderPlan
  {
    RenderInput source_input;
    TileDim source_tile_size;
    double scale_x = 1.0;
    double scale_y = 1.0;
    int source_start_x = 0;
    int source_start_y = 0;
    int source_w = 0;
    int source_h = 0;
  };

  int fractional_render_cache_capacity(int value)
  {
    if (value <= 0) return 0;
    int bucket = FRACTIONAL_RENDER_CACHE_BUCKET_SIZE;
    return ((value + bucket - 1) / bucket) * bucket;
  }

  int fractional_render_cache_grown_capacity(int required, int existing)
  {
    if (required <= existing) return existing;
    int rounded = fractional_render_cache_capacity(required);
    return existing > 0 ? std::max(rounded, existing * 2) : rounded;
  }

  std::optional<FractionalRenderPlan> fractional_render_plan(
    const RenderInput& input,
    double source_zoom)
  {
    FractionalRenderPlan plan;
    plan.source_input = input;
    plan.source_input.zoom = source_zoom;
    plan.source_tile_size = scaled_tile_size(plan.source_input);
    plan.scale_x = scaled_tile_width(input) /
                   static_cast<double>(plan.source_tile_size.w);
    plan.scale_y = scaled_tile_height(input) /
                   static_cast<double>(plan.source_tile_size.h);
    if (plan.scale_x <= 0.0 || plan.scale_y <= 0.0) return std::nullopt;

    double source_x = static_cast<double>(input.offset.x) / plan.scale_x;
    double source_y = static_cast<double>(input.offset.y) / plan.scale_y;
    plan.source_start_x = static_cast<int>(std::floor(source_x));
    plan.source_start_y = static_cast<int>(std::floor(source_y));
    double frac_x = source_x - static_cast<double>(plan.source_start_x);
    double frac_y = source_y - static_cast<double>(plan.source_start_y);
    plan.source_w = std::max(
      1,
      static_cast<int>(std::ceil(static_cast<double>(input.target_rect.w) /
                                   plan.scale_x +
                                 frac_x)));
    plan.source_h = std::max(
      1,
      static_cast<int>(std::ceil(static_cast<double>(input.target_rect.h) /
                                   plan.scale_y +
                                 frac_y)));
    plan.source_input.offset.x = plan.source_start_x;
    plan.source_input.offset.y = plan.source_start_y;
    plan.source_input.render_offset = plan.source_input.offset;
    plan.source_input.has_target_rect = true;
    plan.source_input.target_rect = Pixils::Rect{0, 0, plan.source_w, plan.source_h};
    plan.source_input.ranges = render_ranges(plan.source_input);
    return plan;
  }

  bool fractional_render_plan_fits(const FractionalRenderPlan& plan,
                                   const FractionalRenderCacheTexture& cache)
  {
    return plan.source_w <= cache.w && plan.source_h <= cache.h;
  }

  FractionalRenderPlan best_fractional_render_plan_for_cache(
    const RenderInput& input,
    const FractionalRenderPlan& minimum_plan,
    const FractionalRenderCacheTexture& cache)
  {
    double minimum_zoom = minimum_plan.source_input.zoom;
    if (minimum_zoom >= 1.0) return minimum_plan;

    TileDim maximum_tile_size = input.tile_size;
    for (int candidate_w = maximum_tile_size.w;
         candidate_w > minimum_plan.source_tile_size.w;
         candidate_w--)
    {
      double candidate_zoom =
        static_cast<double>(candidate_w) / static_cast<double>(input.tile_size.w);
      if (auto candidate = fractional_render_plan(input, candidate_zoom);
          candidate && fractional_render_plan_fits(*candidate, cache))
      {
        return *candidate;
      }
    }
    return minimum_plan;
  }

  FractionalRenderCacheTexture fractional_render_cache_texture(Pixils::RenderContext& rc,
                                                               int required_w,
                                                               int required_h)
  {
    if (!rc.renderer || !rc.asset_registry || required_w <= 0 || required_h <= 0)
    {
      return {};
    }

    try
    {
      rc.asset_registry->create_dynamic_bundle(FRACTIONAL_RENDER_CACHE_BUNDLE);
      auto generated_sizes =
        rc.asset_registry->generated_image_sizes(FRACTIONAL_RENDER_CACHE_BUNDLE);
      auto generated_size = generated_sizes.find(FRACTIONAL_RENDER_CACHE_ASSET);
      SDL_Texture* existing = rc.asset_registry->get_image(FRACTIONAL_RENDER_CACHE_BUNDLE,
                                                           FRACTIONAL_RENDER_CACHE_ASSET);
      if (existing && generated_size != generated_sizes.end() &&
          generated_size->second.w >= required_w && generated_size->second.h >= required_h)
      {
        return FractionalRenderCacheTexture{
          existing, generated_size->second.w, generated_size->second.h};
      }

      if (existing || generated_size != generated_sizes.end())
      {
        rc.asset_registry->remove_image(FRACTIONAL_RENDER_CACHE_BUNDLE,
                                        FRACTIONAL_RENDER_CACHE_ASSET);
      }

      int existing_w =
        generated_size != generated_sizes.end() ? generated_size->second.w : 0;
      int existing_h =
        generated_size != generated_sizes.end() ? generated_size->second.h : 0;
      int capacity_w = fractional_render_cache_grown_capacity(required_w, existing_w);
      int capacity_h = fractional_render_cache_grown_capacity(required_h, existing_h);
      std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
        SDL_CreateTexture(rc.renderer,
                          SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET,
                          capacity_w,
                          capacity_h),
        SDL_DestroyTexture);
      if (!texture) return {};

      SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2, 0, 12)
      SDL_SetTextureScaleMode(texture.get(), SDL_ScaleModeLinear);
#endif
      SDL_Texture* committed = texture.get();
      rc.asset_registry->add_generated_image(FRACTIONAL_RENDER_CACHE_BUNDLE,
                                             FRACTIONAL_RENDER_CACHE_ASSET,
                                             committed,
                                             nullptr,
                                             Pixils::Dimension{capacity_w, capacity_h});
      texture.release();
      return FractionalRenderCacheTexture{committed, capacity_w, capacity_h};
    }
    catch (...)
    {
      return {};
    }
  }

  bool render_fractional_scaled_layers(Pixils::RenderContext& rc,
                                       const Roo::sptr_val& layers,
                                       const Roo::sptr_val& hidden,
                                       const RenderInput& input)
  {
    if (!input.has_target_rect || input.zoom <= 0.0) return false;

    auto minimum_plan = fractional_render_plan(input, integer_scaled_tile_zoom(input));
    if (!minimum_plan) return false;

    FractionalRenderCacheTexture cache =
      fractional_render_cache_texture(rc, minimum_plan->source_w, minimum_plan->source_h);
    if (!cache) return false;
    FractionalRenderPlan plan =
      best_fractional_render_plan_for_cache(input, *minimum_plan, cache);

    SDL_Texture* previous_target = rc.current_render_target;
    auto previous_clip = rc.current_clip_rect;
    SDL_Rect previous_viewport{0, 0, 0, 0};
    SDL_RenderGetViewport(rc.renderer, &previous_viewport);

    pad_render_ranges(&plan.source_input,
                      draw_range_padding_for_layers(rc, layers, hidden, plan.source_input));

    try
    {
      rc.set_render_target(cache.texture);
      SDL_RenderSetViewport(rc.renderer, nullptr);
      rc.set_clip_rect(std::nullopt);
      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(rc.renderer, 0, 0, 0, 0);
      SDL_RenderClear(rc.renderer);

      int index = 0;
      for (const auto& layer : Roo::get_children(*layers))
      {
        if (!int_vector_contains(hidden, index))
        {
          render_layer(rc, plan.source_input, layer);
        }
        index++;
      }
    }
    catch (...)
    {
      rc.set_render_target(previous_target);
      SDL_RenderSetViewport(rc.renderer, &previous_viewport);
      rc.set_clip_rect(previous_clip);
      throw;
    }

    rc.set_render_target(previous_target);
    SDL_RenderSetViewport(rc.renderer, &previous_viewport);
    rc.set_clip_rect(previous_clip);

    Pixils::Rect effective_clip = intersect_clip_rect(previous_clip, input.target_rect);
    if (effective_clip.w <= 0 || effective_clip.h <= 0) return true;

    rc.set_clip_rect(effective_clip);
    FloatRect dest{(static_cast<double>(plan.source_start_x) * plan.scale_x) -
                     static_cast<double>(input.render_offset.x),
                   (static_cast<double>(plan.source_start_y) * plan.scale_y) -
                     static_cast<double>(input.render_offset.y),
                   static_cast<double>(plan.source_w) * plan.scale_x,
                   static_cast<double>(plan.source_h) * plan.scale_y};
    SDL_Rect source_rect{0, 0, plan.source_w, plan.source_h};
    SDL_FRect dest_rect = dest.to_SDL_frect();
    SDL_RenderCopyF(rc.renderer, cache.texture, &source_rect, &dest_rect);
    rc.set_clip_rect(previous_clip);
    return true;
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

    if (input.has_target_rect && has_fractional_scaled_tile_size(input) &&
        render_fractional_scaled_layers(rc, layers, hidden, input))
    {
      return true;
    }

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

  std::unordered_set<std::string> value_filter(const Roo::sptr_val& values)
  {
    std::unordered_set<std::string> out;
    if (nil_value(values)) return out;
    if (values->type == Roo::Value::Type::MAP)
    {
      for (const auto& key : Roo::Dict::keys(*values))
      {
        out.insert(value_key(key));
      }
      return out;
    }
    if (seq_value(values))
    {
      for (const auto& value : Roo::get_children(*values))
      {
        out.insert(value_key(value));
      }
      return out;
    }
    out.insert(value_key(values));
    return out;
  }

  bool filter_matches(const std::unordered_set<std::string>& filter,
                      const Roo::sptr_val& value)
  {
    return filter.empty() || filter.contains(value_key(value));
  }

  bool layer_selector_matches(const Roo::sptr_val& selector,
                              const Roo::sptr_val& layer,
                              int layer_index)
  {
    if (selector && selector->type == Roo::Value::Type::NUMBER)
    {
      return selector->num().get_int() == layer_index;
    }
    return same_value(selector, prop(layer, "id"));
  }

  bool layer_selected(const Roo::sptr_val& selectors,
                      const Roo::sptr_val& layer,
                      int layer_index)
  {
    if (nil_value(selectors)) return true;
    if (seq_value(selectors))
    {
      for (const auto& selector : Roo::get_children(*selectors))
      {
        if (layer_selector_matches(selector, layer, layer_index)) return true;
      }
      return false;
    }
    return layer_selector_matches(selectors, layer, layer_index);
  }

  Roo::sptr_val position_value(int x, int y)
  {
    return map_value({keyword_value("x"),
                      Roo::Value::number(x),
                      keyword_value("y"),
                      Roo::Value::number(y)});
  }

  int64_t packed_position_key(int x, int y)
  {
    return (static_cast<int64_t>(static_cast<uint32_t>(x)) << 32) |
           static_cast<uint32_t>(y);
  }

  std::unordered_set<int64_t> position_filter(const Roo::sptr_val& positions)
  {
    std::unordered_set<int64_t> out;
    for (const auto& position : seq_children(positions))
    {
      if (position && position->type == Roo::Value::Type::MAP)
      {
        out.insert(packed_position_key(int_prop(position, "x", 0),
                                       int_prop(position, "y", 0)));
      }
    }
    return out;
  }

  bool position_filter_matches(const std::unordered_set<int64_t>& filter, int x, int y)
  {
    return !filter.empty() && filter.contains(packed_position_key(x, y));
  }

  void apply_properties(Roo::sptr_val& target,
                        const Roo::sptr_val& properties,
                        bool overwrite)
  {
    if (!target || target->type != Roo::Value::Type::MAP || !properties ||
        properties->type != Roo::Value::Type::MAP)
    {
      return;
    }
    for (const auto& key : Roo::Dict::map_sptr_keys(properties))
    {
      if (!overwrite && !nil_value(Roo::Dict::get_property(target, key))) continue;
      Roo::Dict::set_property(target, key, Roo::Dict::get_property(properties, key));
    }
  }

  Roo::sptr_val layer_cell_match_value(const Roo::sptr_val& layer,
                                       int layer_index,
                                       int x,
                                       int y,
                                       const Roo::sptr_val& tile_ref)
  {
    return map_value({keyword_value("position"),
                      position_value(x, y),
                      keyword_value("x"),
                      Roo::Value::number(x),
                      keyword_value("y"),
                      Roo::Value::number(y),
                      keyword_value("layer-id"),
                      prop(layer, "id"),
                      keyword_value("layer-index"),
                      Roo::Value::number(layer_index),
                      keyword_value("tile-ref"),
                      tile_ref});
  }

  Roo::sptr_val find_layer_cells(const Roo::sptr_val& tilemap,
                                 const Roo::sptr_val& opts)
  {
    if (!tilemap || tilemap->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;

    auto layer_selectors = prop(opts, "layers");
    auto role = value_filter(prop(opts, "role"));
    auto data_kind = value_filter(prop(opts, "data-kind"));
    auto tile_refs = value_filter(prop(opts, "tile-refs"));
    bool include_empty = truthy_prop(opts, "include-empty?");

    Roo::sptr_val_v matches;
    int layer_index = 0;
    for (const auto& layer : seq_children(prop(tilemap, "layers")))
    {
      if (!layer || layer->type != Roo::Value::Type::MAP)
      {
        layer_index++;
        continue;
      }
      if (!layer_selected(layer_selectors, layer, layer_index) ||
          !filter_matches(role, prop(layer, "role")) ||
          !filter_matches(data_kind, prop(layer, "data-kind")))
      {
        layer_index++;
        continue;
      }

      auto rows = tile_rows(prop(layer, "tiles"));
      for (int y = 0; y < static_cast<int>(rows.size()); y++)
      {
        for (int x = 0; x < static_cast<int>(rows[y].size()); x++)
        {
          auto tile_ref = rows[y][x];
          if (!include_empty && nil_value(tile_ref)) continue;
          if (!filter_matches(tile_refs, tile_ref)) continue;
          matches.push_back(layer_cell_match_value(layer, layer_index, x, y, tile_ref));
        }
      }
      layer_index++;
    }

    return Roo::Value::vector(matches);
  }

  std::vector<std::vector<Roo::sptr_val>> masked_layer_rows(
    const Roo::sptr_val& layer,
    const std::unordered_set<std::string>& masked_tile_refs,
    const std::unordered_set<int64_t>& masked_positions)
  {
    auto rows = tile_rows(prop(layer, "tiles"));
    for (int y = 0; y < static_cast<int>(rows.size()); y++)
    {
      for (int x = 0; x < static_cast<int>(rows[y].size()); x++)
      {
        auto tile_ref = rows[y][x];
        if (position_filter_matches(masked_positions, x, y) ||
            (!nil_value(tile_ref) && filter_matches(masked_tile_refs, tile_ref)))
        {
          rows[y][x] = Roo::Constant::NIL;
        }
      }
    }
    return rows;
  }

  Roo::sptr_val layer_with_masked_rows(
    const Roo::sptr_val& source,
    const std::vector<std::vector<Roo::sptr_val>>& rows)
  {
    auto layer = Roo::Dict::shallow_copy(source);
    map_set(layer, "tiles", rows_value(rows));
    return layer;
  }

  Roo::sptr_val live_base_layer_value(
    const Roo::sptr_val& layer,
    const std::unordered_set<std::string>& masked_tile_refs,
    const std::unordered_set<int64_t>& masked_positions,
    const Roo::sptr_val& overlay_layer_defaults)
  {
    auto out = layer_with_masked_rows(layer,
                                      masked_layer_rows(layer,
                                                        masked_tile_refs,
                                                        masked_positions));
    apply_properties(out, overlay_layer_defaults, false);
    return out;
  }

  Roo::sptr_val live_base_tiled_layer_value(const Roo::sptr_val& layer,
                                            const Roo::sptr_val& base_layer_props)
  {
    auto out = Roo::Dict::shallow_copy(layer);
    apply_properties(out, base_layer_props, true);
    return out;
  }

  Roo::sptr_val live_base_tiled_layers(
    const Roo::sptr_val& layers,
    const Roo::sptr_val& base_layer_id,
    const std::unordered_set<std::string>& masked_tile_refs,
    const std::unordered_set<int64_t>& masked_positions,
    const Roo::sptr_val& base_layer_props,
    const Roo::sptr_val& overlay_layer_defaults)
  {
    Roo::sptr_val_v out;
    for (const auto& layer : seq_children(layers))
    {
      if (same_value(prop(layer, "id"), base_layer_id))
      {
        out.push_back(live_base_tiled_layer_value(layer, base_layer_props));
      }
      else
      {
        out.push_back(live_base_layer_value(layer,
                                            masked_tile_refs,
                                            masked_positions,
                                            overlay_layer_defaults));
      }
    }
    return Roo::Value::vector(out);
  }

  Roo::sptr_val live_base_tilemap(const Roo::sptr_val& tilemap,
                                  const Roo::sptr_val& opts)
  {
    if (!tilemap || tilemap->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;
    auto layers = prop(tilemap, "layers");
    auto base_layer_id = prop(opts, "base-layer-id");

    auto out = Roo::Dict::shallow_copy(tilemap);
    apply_properties(out, prop(opts, "tilemap-props"), true);
    map_set(out,
            "layers",
            live_base_tiled_layers(layers,
                                   base_layer_id,
                                   value_filter(prop(opts, "masked-tile-refs")),
                                   position_filter(prop(opts, "masked-positions")),
                                   prop(opts, "base-layer-props"),
                                   prop(opts, "overlay-layer-defaults")));
    return out;
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
    std::vector<std::vector<Roo::sptr_val>> transition_masks;
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

  struct OutputEntry
  {
    int x = 0;
    int y = 0;
    Roo::sptr_val tile = Roo::Constant::NIL;
    Roo::sptr_val mask_ref = Roo::Constant::NIL;
  };

  struct TerrainRuleMaterialization
  {
    std::vector<GeneratedLayer> generated_layers;
    std::vector<std::vector<Roo::sptr_val>> transition_replacements;
    std::vector<Roo::sptr_val> transition_tiles;
    std::unordered_set<std::string> transition_tile_ids;
  };

  struct LayerMaterialization
  {
    std::vector<Roo::sptr_val> layers;
    std::vector<Roo::sptr_val> transition_tiles;
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

  using TileDefinitionsByTileset =
    std::unordered_map<std::string, std::unordered_map<std::string, Roo::sptr_val>>;

  TileDefinitionsByTileset tile_definitions_by_tileset(const Roo::sptr_val& tilesets)
  {
    TileDefinitionsByTileset out;
    for (const auto& tileset : seq_children(tilesets))
    {
      auto tileset_id = prop(tileset, "id");
      auto& tile_defs = out[value_key(tileset_id)];
      for (const auto& tile : seq_children(prop(tileset, "tiles")))
      {
        tile_defs[value_key(prop(tile, "id"))] = tile;
      }
    }
    return out;
  }

  Roo::sptr_val tile_definition_in(const TileDefinitionsByTileset& tile_defs,
                                   const Roo::sptr_val& tileset,
                                   const Roo::sptr_val& tile)
  {
    auto tileset_found = tile_defs.find(value_key(tileset));
    if (tileset_found == tile_defs.end()) return Roo::Constant::NIL;
    auto tile_found = tileset_found->second.find(value_key(tile));
    if (tile_found == tileset_found->second.end()) return Roo::Constant::NIL;
    return tile_found->second;
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

  bool terrain_in_condition_value(const Roo::sptr_val& condition,
                                  const Roo::sptr_val& value)
  {
    for (const auto& terrain : seq_children(prop(condition, "terrain-in")))
    {
      if (same_value(value, terrain)) return true;
    }
    return false;
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
      if (!nil_value(prop(condition, "terrain-in")))
      {
        return terrain_in_condition_value(condition, value);
      }
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
    layer.transition_masks = tile_rows(prop(value, "transition-masks"));
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

  Roo::sptr_val terrain_preview_ref_value(const TerrainSet* terrain_set,
                                          const Roo::sptr_val& terrain_ref)
  {
    TerrainPreview preview = terrain_preview(terrain_set, terrain_ref);
    if (nil_value(preview.tileset) || nil_value(preview.tile)) return Roo::Constant::NIL;
    return map_value({keyword_value("tileset"),
                      preview.tileset,
                      keyword_value("tile"),
                      preview.tile});
  }

  std::vector<Roo::sptr_val> condition_terrains(const Roo::sptr_val& condition)
  {
    if (!condition || condition->type != Roo::Value::Type::MAP) return {};
    auto terrain_in = prop(condition, "terrain-in");
    if (!nil_value(terrain_in)) return seq_children(terrain_in);
    auto terrain = prop(condition, "terrain");
    if (!nil_value(terrain)) return {terrain};
    return {};
  }

  Roo::sptr_val first_opposing_terrain(const TerrainStampRule& rule)
  {
    static const std::vector<std::string> directions = {
      "nw", "n", "ne", "w", "e", "sw", "s", "se"};
    for (const auto& direction : directions)
    {
      auto condition = rule.match.find(direction);
      if (condition == rule.match.end()) continue;
      for (const auto& terrain : condition_terrains(condition->second))
      {
        if (!nil_value(terrain) && !same_value(terrain, rule.center)) return terrain;
      }
    }
    return Roo::Constant::NIL;
  }

  Roo::sptr_val first_opposing_terrain_at(
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const TerrainStampRuleset& ruleset,
    const TerrainStampRule& rule,
    int x,
    int y)
  {
    static const std::vector<std::string> directions = {
      "nw", "n", "ne", "w", "e", "sw", "s", "se"};
    for (const auto& direction : directions)
    {
      auto condition = rule.match.find(direction);
      if (condition == rule.match.end()) continue;
      if (keyword_named(condition->second, "ignore")) continue;
      auto terrain = terrain_value_at(source_rows, ruleset, direction, x, y);
      if (condition_matches(rule.center, condition->second, terrain) &&
          known_terrain_value(terrain) && !same_value(terrain, rule.center))
      {
        return terrain;
      }
    }
    return first_opposing_terrain(rule);
  }

  Roo::sptr_val mask_set_by_id(const Roo::sptr_val& mask_sets,
                               const Roo::sptr_val& mask_set_id)
  {
    for (const auto& mask_set : seq_children(mask_sets))
    {
      if (same_value(prop(mask_set, "id"), mask_set_id)) return mask_set;
    }
    return Roo::Constant::NIL;
  }

  Roo::sptr_val mask_by_id(const Roo::sptr_val& mask_set,
                           const Roo::sptr_val& mask_id)
  {
    for (const auto& mask : seq_children(prop(mask_set, "masks")))
    {
      if (same_value(prop(mask, "id"), mask_id)) return mask;
    }
    return Roo::Constant::NIL;
  }

  Roo::sptr_val resolved_mask_source_tiles(
    const TileDefinitionsByTileset& tile_defs,
    const Roo::sptr_val& source)
  {
    Roo::sptr_val_v out;
    auto source_tileset = prop(source, "tileset");
    for (const auto& item : seq_children(prop(source, "tiles")))
    {
      auto copy = Roo::Dict::shallow_copy(item);
      map_set(copy,
              "tile-definition",
              tile_definition_in(tile_defs, source_tileset, prop(item, "tile")));
      out.push_back(copy);
    }
    return Roo::Value::vector(out);
  }

  Roo::sptr_val resolved_transition_mask_source(
    const Roo::sptr_val& mask_sets,
    const TileDefinitionsByTileset& tile_defs,
    const Roo::sptr_val& mask_ref)
  {
    auto mask_set = mask_set_by_id(mask_sets, prop(mask_ref, "mask-set"));
    auto mask = mask_by_id(mask_set, prop(mask_ref, "mask"));
    if (nil_value(mask_set) || nil_value(mask)) return Roo::Constant::NIL;
    auto source = prop(mask, "source");
    if (!source || source->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;
    auto out = Roo::Dict::shallow_copy(source);
    map_set(out, "size", prop(mask_set, "mask-size"));
    map_set(out, "tiles", resolved_mask_source_tiles(tile_defs, source));
    return out;
  }

  Roo::sptr_val transition_mask_polarity(const Roo::sptr_val& mask_ref)
  {
    if (keyword_named(prop(mask_ref, "polarity"), "solid-is-opposing"))
    {
      return keyword_value("solid-is-opposing");
    }
    return keyword_value("solid-is-current");
  }

  std::string transition_tile_id_label(const Roo::sptr_val& base_ref,
                                       const Roo::sptr_val& overlay_ref,
                                       const Roo::sptr_val& mask_ref)
  {
    return "transition-" + id_label(prop(base_ref, "tileset")) + "-" +
           id_label(prop(base_ref, "tile")) + "-" +
           id_label(prop(overlay_ref, "tileset")) + "-" +
           id_label(prop(overlay_ref, "tile")) + "-" +
           id_label(prop(mask_ref, "mask-set")) + "-" +
           id_label(prop(mask_ref, "mask"));
  }

  Roo::sptr_val transition_cache_image(const Roo::sptr_val& tile_id)
  {
    return keyword_value("pixils-transition-cache/" + id_label(tile_id));
  }

  Roo::sptr_val transition_tile_ref(const Roo::sptr_val& tile)
  {
    return map_value({keyword_value("tileset"),
                      keyword_value("pixils-transition-tiles"),
                      keyword_value("tile"),
                      prop(tile, "id")});
  }

  Roo::sptr_val transition_ref_with_definition(
    const Roo::sptr_val& ref,
    const TileDefinitionsByTileset& tile_defs)
  {
    auto out = Roo::Dict::shallow_copy(ref);
    map_set(out,
            "tile-definition",
            tile_definition_in(tile_defs, prop(ref, "tileset"), prop(ref, "tile")));
    return out;
  }

  Roo::sptr_val transition_tile_definition(
    const Roo::sptr_val& tilemap,
    const Roo::sptr_val& mask_sets,
    const TileDefinitionsByTileset& tile_defs,
    const TerrainSet* terrain_set,
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const TerrainStampRuleset& ruleset,
    const TerrainStampRule& rule,
    const Roo::sptr_val& mask_ref,
    int source_x,
    int source_y)
  {
    auto base_ref = terrain_preview_ref_value(terrain_set, rule.center);
    auto opposing_ref =
      terrain_preview_ref_value(terrain_set,
                                first_opposing_terrain_at(source_rows,
                                                          ruleset,
                                                          rule,
                                                          source_x,
                                                          source_y));
    auto mask_source = resolved_transition_mask_source(mask_sets, tile_defs, mask_ref);
    if (nil_value(base_ref) || nil_value(opposing_ref) || nil_value(mask_source))
    {
      return Roo::Constant::NIL;
    }

    auto polarity = transition_mask_polarity(mask_ref);
    auto oriented_base =
      keyword_named(polarity, "solid-is-opposing") ? opposing_ref : base_ref;
    auto oriented_overlay =
      keyword_named(polarity, "solid-is-opposing") ? base_ref : opposing_ref;
    auto tile_id =
      keyword_value(transition_tile_id_label(oriented_base, oriented_overlay, mask_ref));

    return map_value({keyword_value("id"),
                      tile_id,
                      keyword_value("type"),
                      keyword_value("transition-mask"),
                      keyword_value("size"),
                      prop(tilemap, "tile-size"),
                      keyword_value("base"),
                      transition_ref_with_definition(oriented_base, tile_defs),
                      keyword_value("overlay"),
                      transition_ref_with_definition(oriented_overlay, tile_defs),
                      keyword_value("mask"),
                      mask_ref,
                      keyword_value("mask-polarity"),
                      polarity,
                      keyword_value("mask-source"),
                      mask_source,
                      keyword_value("image"),
                      transition_cache_image(tile_id)});
  }

  void add_unique_transition_tile(TerrainRuleMaterialization& result,
                                  const Roo::sptr_val& tile)
  {
    if (nil_value(tile)) return;
    std::string id = value_key(prop(tile, "id"));
    if (result.transition_tile_ids.insert(id).second)
    {
      result.transition_tiles.push_back(tile);
    }
  }

  void set_transition_replacement(TerrainRuleMaterialization& result,
                                  int x,
                                  int y,
                                  const Roo::sptr_val& tile_ref)
  {
    if (nil_value(tile_ref)) return;
    if (y < 0 || y >= static_cast<int>(result.transition_replacements.size())) return;
    if (x < 0 ||
        x >= static_cast<int>(result.transition_replacements[y].size()))
    {
      return;
    }
    result.transition_replacements[y][x] = tile_ref;
  }

  std::vector<OutputEntry> output_entries(const TerrainStampRuleset& ruleset,
                                          const TerrainStampRule& rule,
                                          const OutputLayer& output_layer)
  {
    if (ruleset.unit_w == 1 && ruleset.unit_h == 1)
    {
      return {OutputEntry{rule.anchor_x,
                          rule.anchor_y,
                          tile_at(output_layer.tiles, rule.anchor_x, rule.anchor_y),
                          tile_at(output_layer.transition_masks,
                                  rule.anchor_x,
                                  rule.anchor_y)}};
    }

    std::vector<OutputEntry> out;
    for (int tile_y = 0; tile_y < static_cast<int>(output_layer.tiles.size()); tile_y++)
    {
      for (int tile_x = 0;
           tile_x < static_cast<int>(output_layer.tiles[tile_y].size());
           tile_x++)
      {
        out.push_back(OutputEntry{tile_x,
                                  tile_y,
                                  output_layer.tiles[tile_y][tile_x],
                                  tile_at(output_layer.transition_masks,
                                          tile_x,
                                          tile_y)});
      }
    }
    return out;
  }

  bool output_layer_generates_layer(const TerrainStampRuleset& ruleset,
                                    const TerrainStampRule& rule,
                                    const OutputLayer& output_layer)
  {
    for (const auto& entry : output_entries(ruleset, rule, output_layer))
    {
      if (nil_value(entry.mask_ref)) return true;
    }
    return false;
  }

  void apply_output_entry(TerrainRuleMaterialization& result,
                          int width,
                          int height,
                          bool known_tilesets,
                          const std::unordered_map<std::string, std::unordered_set<std::string>>&
                            tile_ids,
                          const Roo::sptr_val& tilemap,
                          const Roo::sptr_val& mask_sets,
                          const TileDefinitionsByTileset& tile_defs,
                          const TerrainSet* terrain_set,
                          const std::vector<std::vector<Roo::sptr_val>>& source_rows,
                          const Roo::sptr_val& source_layer,
                          const TerrainStampRuleset& ruleset,
                          const TerrainStampRule& rule,
                          const OutputLayer& output_layer,
                          const OutputEntry& entry,
                          int source_x,
                          int source_y)
  {
    int x = source_x + entry.x - rule.anchor_x;
    int y = source_y + entry.y - rule.anchor_y;
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    if (!nil_value(entry.mask_ref))
    {
      auto tile = transition_tile_definition(tilemap,
                                             mask_sets,
                                             tile_defs,
                                             terrain_set,
                                             source_rows,
                                             ruleset,
                                             rule,
                                             entry.mask_ref,
                                             source_x,
                                             source_y);
      add_unique_transition_tile(result, tile);
      set_transition_replacement(
        result, x, y, nil_value(tile) ? entry.tile : transition_tile_ref(tile));
      return;
    }

    auto layer_id = terrain_stamp_output_layer_key(source_layer, ruleset, output_layer);
    int index = generated_layer_index(result.generated_layers, layer_id);
    if (index < 0) return;
    auto& layer = result.generated_layers[index];
    layer.tiles[y][x] = entry.tile;
    layer.source_mask[y][x] =
      output_entry_occupies_source(known_tilesets, tile_ids, ruleset, output_layer, entry.tile);
  }

  void apply_output_layer(TerrainRuleMaterialization& result,
                          int width,
                          int height,
                          bool known_tilesets,
                          const std::unordered_map<std::string, std::unordered_set<std::string>>&
                            tile_ids,
                          const Roo::sptr_val& tilemap,
                          const Roo::sptr_val& mask_sets,
                          const TileDefinitionsByTileset& tile_defs,
                          const TerrainSet* terrain_set,
                          const std::vector<std::vector<Roo::sptr_val>>& source_rows,
                          const Roo::sptr_val& source_layer,
                          const TerrainStampRuleset& ruleset,
                          const TerrainStampRule& rule,
                          const OutputLayer& output_layer,
                          int source_x,
                          int source_y)
  {
    if (output_layer_generates_layer(ruleset, rule, output_layer))
    {
      ensure_generated_layer(
        result.generated_layers, width, height, source_layer, ruleset, output_layer);
    }

    for (const auto& entry : output_entries(ruleset, rule, output_layer))
    {
      apply_output_entry(result,
                         width,
                         height,
                         known_tilesets,
                         tile_ids,
                         tilemap,
                         mask_sets,
                         tile_defs,
                         terrain_set,
                         source_rows,
                         source_layer,
                         ruleset,
                         rule,
                         output_layer,
                         entry,
                         source_x,
                         source_y);
    }
  }

  TerrainRuleMaterialization materialize_terrain_stamp_layers(
    int width,
    int height,
    bool known_tilesets,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& tile_ids,
    const Roo::sptr_val& tilemap,
    const Roo::sptr_val& mask_sets,
    const TileDefinitionsByTileset& tile_defs,
    const std::unordered_map<std::string, TerrainSet>& terrain_sets,
    const Roo::sptr_val& source_layer,
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const std::vector<TerrainStampRuleset>& rulesets)
  {
    TerrainRuleMaterialization result;
    result.transition_replacements = empty_rows(width, height);
    auto terrain_set_found = terrain_sets.find(value_key(prop(source_layer, "terrain-set")));
    const TerrainSet* terrain_set =
      terrain_set_found == terrain_sets.end() ? nullptr : &terrain_set_found->second;
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
              apply_output_layer(result,
                                 width,
                                 height,
                                 known_tilesets,
                                 tile_ids,
                                 tilemap,
                                 mask_sets,
                                 tile_defs,
                                 terrain_set,
                                 source_rows,
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
    return result;
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

  Roo::sptr_val transition_replacement_at(
    const std::vector<std::vector<Roo::sptr_val>>& transition_replacements,
    int x,
    int y)
  {
    return tile_at(transition_replacements, x, y);
  }

  bool has_transition_replacements(
    const std::vector<std::vector<Roo::sptr_val>>& transition_replacements)
  {
    for (const auto& row : transition_replacements)
    {
      for (const auto& cell : row)
      {
        if (!nil_value(cell)) return true;
      }
    }
    return false;
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
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const std::vector<std::vector<Roo::sptr_val>>& transition_replacements)
  {
    std::vector<std::vector<Roo::sptr_val>> rows;
    rows.reserve(source_rows.size());
    for (int y = 0; y < static_cast<int>(source_rows.size()); y++)
    {
      const auto& row = source_rows[y];
      std::vector<Roo::sptr_val> out_row;
      out_row.reserve(row.size());
      for (int x = 0; x < static_cast<int>(row.size()); x++)
      {
        auto replacement = transition_replacement_at(transition_replacements, x, y);
        if (!nil_value(replacement))
        {
          out_row.push_back(same_value(prop(replacement, "tileset"), tileset)
                              ? replacement
                              : Roo::Constant::NIL);
          continue;
        }

        TerrainPreview preview = terrain_preview(terrain_set, row[x]);
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
    const std::vector<std::vector<Roo::sptr_val>>& source_rows,
    const std::vector<std::vector<Roo::sptr_val>>& transition_replacements)
  {
    auto terrain_set_id = value_key(prop(source_layer, "terrain-set"));
    auto terrain_set_found = terrain_sets.find(terrain_set_id);
    const TerrainSet* terrain_set =
      terrain_set_found == terrain_sets.end() ? nullptr : &terrain_set_found->second;
    auto tilesets = terrain_definition_preview_tilesets(terrain_set);
    if (has_transition_replacements(transition_replacements))
    {
      tilesets.push_back(keyword_value("pixils-transition-tiles"));
    }
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
        source_layer, terrain_set, tileset, split, source_rows, transition_replacements));
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

  Roo::sptr_val generated_transition_tilesets_value(
    const std::vector<Roo::sptr_val>& transition_tiles)
  {
    if (transition_tiles.empty()) return Roo::Constant::NIL;
    return Roo::Value::vector(
      {map_value({keyword_value("id"),
                  keyword_value("pixils-transition-tiles"),
                  keyword_value("label"),
                  Roo::string("Generated transition tiles"),
                  keyword_value("tiles"),
                  Roo::Value::vector(transition_tiles)})});
  }

  LayerMaterialization materialize_layer_with_rules(
    int width,
    int height,
    bool show_rules,
    bool known_tilesets,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& tile_ids,
    const Roo::sptr_val& tilemap,
    const Roo::sptr_val& mask_sets,
    const TileDefinitionsByTileset& tile_defs,
    const Roo::sptr_val& layer,
    const std::unordered_map<std::string, TerrainSet>& terrain_sets,
    const std::vector<TerrainStampRuleset>& rulesets)
  {
    bool terrain_layer = keyword_named(prop(layer, "data-kind"), "terrain");
    auto source_rows = tile_rows(prop(layer, "tiles"));
    TerrainRuleMaterialization generated =
      (show_rules && terrain_layer)
        ? materialize_terrain_stamp_layers(width,
                                           height,
                                           known_tilesets,
                                           tile_ids,
                                           tilemap,
                                           mask_sets,
                                           tile_defs,
                                           terrain_sets,
                                           layer,
                                           source_rows,
                                           rulesets)
        : TerrainRuleMaterialization{{},
                                     empty_rows(width, height),
                                     {},
                                     {}};

    LayerMaterialization out;
    if (terrain_layer)
    {
      auto masked_rows = mask_terrain_layer(source_rows, generated.generated_layers);
      auto terrain_layers = materialize_terrain_layer(layer,
                                                      terrain_sets,
                                                      masked_rows,
                                                      generated.transition_replacements);
      out.layers.insert(out.layers.end(), terrain_layers.begin(), terrain_layers.end());
    }
    else
    {
      auto copied = Roo::Dict::shallow_copy(layer);
      if (nil_value(prop(copied, "data-kind")))
      {
        map_set(copied, "data-kind", keyword_value("tile-ref"));
      }
      out.layers.push_back(copied);
    }
    for (const auto& generated_layer : generated.generated_layers)
    {
      out.layers.push_back(generated_layer_value(generated_layer));
    }
    out.transition_tiles = generated.transition_tiles;
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
    auto tile_defs = tile_definitions_by_tileset(prop(tilemap, "tilesets"));
    bool known_tilesets = !nil_value(prop(tilemap, "tilesets"));
    auto rulesets = terrain_stamp_rulesets(rulesets_value, terrain_sets);
    auto substitution_rules = materialize_tile_substitution_rules(tilemap, opts);
    auto mask_sets = prop(opts, "mask-sets");
    if (nil_value(mask_sets)) mask_sets = prop(tilemap, "mask-sets");

    std::vector<Roo::sptr_val> render_layers;
    std::vector<Roo::sptr_val> transition_tiles;
    std::unordered_set<std::string> transition_tile_ids;
    for (const auto& layer : seq_children(source_layers_value))
    {
      auto result = materialize_layer_with_rules(width,
                                                 height,
                                                 show_terrain_rules(tilemap, opts),
                                                 known_tilesets,
                                                 tile_ids,
                                                 tilemap,
                                                 mask_sets,
                                                 tile_defs,
                                                 layer,
                                                 terrain_sets,
                                                 rulesets);
      render_layers.insert(render_layers.end(),
                           result.layers.begin(),
                           result.layers.end());
      for (const auto& tile : result.transition_tiles)
      {
        std::string id = value_key(prop(tile, "id"));
        if (transition_tile_ids.insert(id).second)
        {
          transition_tiles.push_back(tile);
        }
      }
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
    auto transition_tilesets = generated_transition_tilesets_value(transition_tiles);
    if (!nil_value(transition_tilesets))
    {
      map_set(result, "tilesets", transition_tilesets);
    }
    return result;
  }

  namespace Function
  {
    FUNC(RenderLayersBang, render_layers);
    FUNC(MaterializeRenderMap, native_materialize_render_map);
    FUNC(FindLayerCells, find_layer_cells);
    FUNC(LiveBaseTilemap, live_base_tilemap);

    FUNC_IMPL(RenderLayersBang,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&RenderLayersBang::exec_render_layers))));

    FUNC_IMPL(MaterializeRenderMap,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&MaterializeRenderMap::exec_native_materialize_render_map))));

    FUNC_IMPL(FindLayerCells,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&FindLayerCells::exec_find_layer_cells))));

    FUNC_IMPL(LiveBaseTilemap,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&LiveBaseTilemap::exec_live_base_tilemap))));

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

    EXEC_BODY(FindLayerCells, exec_find_layer_cells)
    {
      return find_layer_cells(args[0], args[1]);
    }

    EXEC_BODY(LiveBaseTilemap, exec_live_base_tilemap)
    {
      return live_base_tilemap(args[0], args[1]);
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

  class OpsImplNamespace : public Roo::Namespace
  {
   public:
    OpsImplNamespace()
      : Roo::Namespace("pixils.tilemap.ops-impl")
    {
      values.emplace("find-layer-cells", Function::FindLayerCells::make());
      values.emplace("live-base-tilemap", Function::LiveBaseTilemap::make());
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
      auto ops_ns = std::make_unique<OpsImplNamespace>();
      ops_ns->set_origin(Roo::Namespace::Origin::native());
      if (host->register_namespace(host->user, ops_ns.release()) != 0)
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
