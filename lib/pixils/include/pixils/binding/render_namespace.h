
#ifndef PIXILS__RENDER_NAMESPACE_H
#define PIXILS__RENDER_NAMESPACE_H

#include <pixils/binding/shkey.h>

#include <roo/exec.h>
#include <roo/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__RENDER = "pixils.render";

  inline constexpr std::string_view FN__DRAW_IMAGE_BANG = "image!";
  inline constexpr std::string_view FN__DRAW_CIRCLE_BANG = "circle!";
  inline constexpr std::string_view FN__DRAW_LINE_BANG = "line!";
  inline constexpr std::string_view FN__DRAW_POLYGON_BANG = "polygon!";
  inline constexpr std::string_view FN__DRAW_RECT_BANG = "rect!";
  inline constexpr std::string_view FN__RENDER_TEXT_BANG = "text!";
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
    /**
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
     * | :rotation    | Rotation in radians                                           |
     * | :flip-x?     | Flip each copy horizontally                                   |
     * | :flip-y?     | Flip each copy vertically                                     |
     *
     * @return nil
     */
    FUNC(DrawImageBang, draw_img);
    /*! @brief draw-circle! function */
    FUNC(DrawCircleBang, draw_circle);
    /*! @brief draw-line! function */
    FUNC(DrawLineBang, draw_line);
    /*! @brief draw-polygon! function */
    FUNC(DrawPolygonBang, polygon, polygon_with_opts);
    /*! @brief draw-rect! function */
    FUNC(DrawRectBang, draw_rect, draw_rect_from_points);
    /*! @brief text! function */
    FUNC(RenderTextBang, text_no_opts, text);
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
