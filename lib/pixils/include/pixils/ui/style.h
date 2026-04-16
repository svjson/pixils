
#ifndef PIXILS__UI__STYLE_H
#define PIXILS__UI__STYLE_H

#include <pixils/geom.h>

#include <SDL2/SDL_render.h>
#include <memory>
#include <optional>

namespace Lisple
{
  class RTValue;
  using sptr_rtval = std::shared_ptr<RTValue>;
} // namespace Lisple

namespace Pixils::UI
{
  struct Style
  {
    struct Background
    {
      Background();
      Background(const std::pair<std::string, std::string>& resource_id);
      Background(const Color& color);

      std::optional<std::pair<std::string, std::string>> image;
      std::optional<Color> color;
    };

    /*! @brief Defines four-directional padding */
    struct Padding
    {
      /*! Top Padding */
      int t = 0;
      /*! Right Padding */
      int r = 0;
      /*! Bottom Padding */
      int b = 0;
      /*! Left  Padding */
      int l = 0;

      Padding() = default;
      /*! @brief Copy-constructor */
      Padding(const Padding& other);
      /*!
       * @brief Constructs a Padding instance with individual padding for
       * top, right, bottom and left edges.
       */
      Padding(int t, int r, int b, int l);
      /*!
       * @brief Constructs a Padding instance with equals padding for top
       * and bottom(vertical) and left and right(horizontal), respectively.
       *
       * @param h Horizontal padding
       * @param v Vertical padding
       */
      Padding(int h, int v);

      /*!
       * @brief Multiply-operator, multiplying each directional value
       * by @a amount..
       */
      Padding operator*(int amount) const;
      /*!
       * @brief Assignment-operator, copies the values of @a other
       * to this instance.
       */
      Padding& operator=(const Padding& other) = default;

      /*!
       * @brief Comparison-operator. Equal if all directional values
       * are identical to @a other.
       */
      bool operator==(const Padding& other) const;

      /*!
       * @brief Applies the padding to a Dimension, effectively
       * expanding it by t+b vertically and l+r horizontally.
       */
      void apply_to(Dimension& dimension);

      /*!
       * @brief Subtracts the padding from a Dimension, effectively
       * shrinking it by t+b vertically and l+r horizontally
       */
      Dimension remove_from(const Dimension& dimension);

      Rect apply_to(const Rect& rect) const;
    };

    Style();
    Style(const Style& other);

    void operator=(const Style& other);

    std::optional<Background> background = std::nullopt;
    std::optional<Padding> padding = std::nullopt;
    std::unique_ptr<Style> hover_style = nullptr;
  };

  /**
   * Merge properties from a style variant map into a ResolvedStyle.
   * Only non-NIL properties in the variant override existing values.
   */
  void apply_style_variant(Style& out, const Style& variant);

  /**
   * Resolve the effective style from a component's declared style map
   * and current state. Applies the base style first, then overlays
   * hover or active variant properties if the corresponding flags are
   * set in state.
   */
  Style resolve_style(const std::optional<Style>& style, const Lisple::sptr_rtval& state);

} // namespace Pixils::UI

#endif /* PIXILS__UI__STYLE_H */
