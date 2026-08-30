
#ifndef PIXILS__RENDER_NAMESPACE_H
#define PIXILS__RENDER_NAMESPACE_H

#include <pixils/binding/shkey.h>

#include <roo/exec.h>
#include <roo/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__RENDER = "pixils.render";

  inline constexpr std::string_view FN__ONTO_IMAGE_BANG = "onto-image!";
  inline constexpr std::string_view FN__DRAW_IMAGE_BANG = "image!";
  inline constexpr std::string_view FN__DRAW_IMAGES_BANG = "images!";
  inline constexpr std::string_view FN__DRAW_CIRCLE_BANG = "circle!";
  inline constexpr std::string_view FN__DRAW_ELLIPSE_BANG = "ellipse!";
  inline constexpr std::string_view FN__DRAW_LINE_BANG = "line!";
  inline constexpr std::string_view FN__DRAW_POLYGON_BANG = "polygon!";
  inline constexpr std::string_view FN__DRAW_RECT_BANG = "rect!";
  inline constexpr std::string_view FN__RENDER_TEXT_BANG = "text!";
  inline constexpr std::string_view FN__TEXT_METRICS = "text-metrics";
  inline constexpr std::string_view FN__TEXT_SIZE = "text-size";
  inline constexpr std::string_view FN__USE_COLOR_BANG = "use-color!";
  inline constexpr std::string_view FN__WITH_CLIP_RECT = "with-clip-rect";

  namespace MapKey
  {
    DECL_SHKEY(CLOSE);
    DECL_SHKEY(OFFSET);
    DECL_SHKEY(ROTATION);
  } // namespace MapKey

  namespace Function
  {
    /*!
     * @brief Render onto an existing generated image.
     * @since 0.1.0
     * @see pixils.resource/create-image!
     * @see pixils.render/image!
     *
     * The resource must identify a generated image in a dynamic bundle. The
     * callback is invoked without arguments while that image's existing texture
     * is the current render target. Normal `pixils.render` operations inside the
     * callback therefore add to the image, and the previous render target is
     * restored afterward.
     *
     * Existing pixels are preserved when `:clear` is omitted. Supplying a color
     * clears the complete image to that color before invoking the callback.
     * Omitting `:readback?` preserves the image's current readback policy;
     * supplying it enables or disables CPU-side pixel access after this pass.
     *
     * Usage:
     * @code
     * (pixils.render/onto-image!
     *   :project-assets/canvas
     *   (fn []
     *     (pixils.render/rect! {:x 2 :y 2 :w 4 :h 4}
     *                          {:fill true :color "#ffffff"})))
     *
     * (pixils.render/onto-image!
     *   :project-assets/canvas
     *   {:clear "#00000000" :readback? false}
     *   (fn []
     *     (pixils.render/image! :sprites/background {:pos {:x 0 :y 0}})))
     * @endcode
     *
     * | Arg      | Description                                                |
     * | -------- | ---------------------------------------------------------- |
     * | resource | Qualified keyword identifying an existing generated image. |
     * | options  | Optional map with `:clear` and `:readback?` values.        |
     * | render   | Zero-argument function that renders onto the image.        |
     *
     * @return The supplied resource keyword.
     */
    FUNC(RenderOntoImageBang, render_onto_image, render_onto_image_with_opts);

    /*!
     * @brief Draw an image resource.
     *
     * Usage:
     * @code
     * (pixils.render/image! :sprites/ship {:pos {:x 12 :y 18}})
     *
     * (pixils.render/image! :sprites/ship
     *   {:target {:x 12 :y 18 :w 32 :h 16}})
     *
     * (pixils.render/image! :terrain/water
     *   {:target {:x 14 :y 10}
     *    :clip-rect {:x 10 :y 10 :w 320 :h 200}
     *    :repeat-x? true
     *    :repeat-y? true})
     * @endcode
     *
     * The second argument can be a point, rect, or options map. Point targets
     * draw one image copy at natural/source size, affected by :scale. Rect
     * targets scale one image copy into that rect. When :repeat-x? or
     * :repeat-y? is true, copies repeat from the target anchor across
     * :clip-rect, or across the active renderer clip if :clip-rect is omitted.
     *
     * | Arg # | Description                                                      |
     * |-------|------------------------------------------------------------------|
     * | 0     | Qualified image keyword, e.g. :bundle/image                      |
     * | 1     | Point, rect, or options map                                      |
     *
     * Options:
     *
     * | Key          | Description                                                   |
     * |--------------|---------------------------------------------------------------|
     * | :pos         | Point placement alias for the common natural-size target case |
     * | :target      | Point or rect target for one image copy                       |
     * | :clip-rect   | Rect that clips drawing and bounds repeated drawing           |
     * | :source      | Optional source crop rect in image pixels                     |
     * | :scale       | Scale multiplier for point targets                            |
     * | :repeat-x?   | Repeat copies horizontally                                    |
     * | :repeat-y?   | Repeat copies vertically                                      |
     * | :opacity     | Alpha multiplier from 0.0 to 1.0                              |
     * | :blend-mode  | :blend, :none, or :erase-alpha                                |
     * | :rotation    | Rotation in radians                                           |
     * | :flip-x?     | Flip each copy horizontally                                   |
     * | :flip-y?     | Flip each copy vertically                                     |
     *
     * @return nil
     */
    FUNC(DrawImageBang, draw_img);
    /**
     * @brief Draw many copies of one image resource.
     *
     * Usage:
     * @code
     * (pixils.render/images! :sprites/tiles
     *   [{:pos {:x 12 :y 18}
     *     :source {:x 0 :y 0 :w 16 :h 16}}
     *    {:target {:x 40 :y 18 :w 32 :h 32}
     *     :source {:x 16 :y 0 :w 16 :h 16}}])
     * @endcode
     *
     * Entries accept the same placement keys as image!. Entries that can be
     * represented as simple textured quads are grouped internally; entries that
     * need behavior such as rotation or repetition fall back to the image! path.
     *
     * @return number of entries rendered
     */
    FUNC(DrawImagesBang, draw_imgs, draw_imgs_with_opts);
    /*! @brief draw-circle! function */
    FUNC(DrawCircleBang, draw_circle);
    /*! @brief draw-ellipse! function */
    FUNC(DrawEllipseBang, draw_ellipse);
    /*! @brief draw-line! function */
    FUNC(DrawLineBang, draw_line);
    /*! @brief draw-polygon! function */
    FUNC(DrawPolygonBang, polygon, polygon_with_opts);
    /*! @brief draw-rect! function */
    FUNC(DrawRectBang, draw_rect, draw_rect_from_points);
    /*! @brief text! function */
    FUNC(RenderTextBang, text_no_opts, text);
    /**
     * @brief Measure text and its insertion-point positions.
     *
     * Usage:
     * @code
     * (pixils.render/text-metrics "abc" {:font :font/console})
     * @endcode
     *
     * | Arg # | Description                                   |
     * |-------|-----------------------------------------------|
     * | 0     | Text to measure                               |
     * | 1     | Optional text options accepted by `text-size` |
     *
     * @return map containing dimensions, line height, and cumulative x positions
     *
     * @since 0.1.0
     */
    FUNC(TextMetrics, metrics_no_opts, metrics);
    /*! @brief text-size function */
    FUNC(TextSize, size_no_opts, size);
    /*! @brief use-color! function */
    FUNC(UseColorBang, use_color, use_color_num);
    /*! @brief with-clip-rect special form */
    SPECIAL_FORM_DECL(WithClipRectForm, with_clip_rect);
  } // namespace Function

  class RenderNamespace : public Roo::Namespace
  {
   public:
    RenderNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__RENDER_NAMESPACE_H */
