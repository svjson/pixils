
#ifndef __PIXILS__RENDER_CONTEXT_H_
#define __PIXILS__RENDER_CONTEXT_H_

#include "geom.h"
#include <pixils/ui/style.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

namespace Pixils
{
  struct Display;
  class FontRegistry;

  namespace Asset
  {
    class Registry;
  }

  /*!
   * @brief The "brush" of the UI. All rendering operations go through here.
   */
  struct RenderContext
  {
    /*!
     * @brief Pointer to the application window
     */
    SDL_Window* window = nullptr;
    /*!
     * @brief Pointer to the SDL renderer that does all rendering
     */
    SDL_Renderer* renderer = nullptr;

    /*!
     * @brief The buffer texture that all in-game drawing happens against
     */
    SDL_Texture* buffer_texture = nullptr;
    /*!
     * @brief The render target Pixils most recently selected on the SDL renderer.
     *
     * SDL exposes this via SDL_GetRenderTarget, but keeping it here makes target
     * restoration testable against the SDL mock as well.
     */
    SDL_Texture* current_render_target = nullptr;
    std::optional<Rect> current_clip_rect = std::nullopt;
    /*!
     * @brief The current size/dimension of the in-memory buffer
     */
    Dimension buffer_dim{0, 0};

    /*
     * @brief The dimensions of the application window
     */
    Rect window_rect{0, 0, 0, 0};
    Rect application_rect{0, 0, 0, 0};

    /*!
     * @brief The on-screen size of game pixels, effetively the scaling
     * factor to use when rendering the in-memory buffer to the screen.
     */
    int pixel_size = 5;

    /*!
     * @brief The size of map tiles
     */
    int tile_size = 16;

    std::unique_ptr<Asset::Registry> asset_registry;
    std::unique_ptr<FontRegistry> font_registry;
    std::unordered_map<std::string, UI::ImageCursor> pointer_registry;

    /*!
     * @brief Allow render primitives to use SDL_RenderGeometry when available.
     *
     * Tests using the SDL mock disable this because the test binary may still
     * load a real SDL library transitively, while mock renderer pointers are not
     * valid real SDL_Renderer objects.
     */
    bool enable_render_geometry = true;

    RenderContext();
    RenderContext(SDL_Window* window, SDL_Renderer* renderer);
    ~RenderContext();
    RenderContext(RenderContext&&) noexcept;
    RenderContext& operator=(RenderContext&&) noexcept;

    Dimension get_window_dimension();

    /*!
     * @brief Calculate where the logical application buffer is drawn in the
     * physical window for a display configuration.
     */
    Rect application_target_rect(Display& display) const;

    /*!
     * @brief Map a physical window coordinate to logical application buffer
     * coordinates, accounting for fit/stretch scaling and alignment.
     */
    Point window_to_buffer_point(Display& display, int x, int y) const;
    Point buffer_to_window_point(const Point& point) const;
    void warp_mouse_to_buffer_point(const Point& point);

    /*!
     * @brief Prepare a new frame for rendering. To be called at the beginning of every
     * frame, regardless of application state.
     */
    void begin_frame(Display& display);

    /*!
     * @brief Make sure the in memory buffer is initialized and of the correct
     * size.
     */
    void prepare_application_frame(Display& display);
    /*!
     * @brief Clear the in-memory buffer
     */
    void clear_buffer();

    /*!
     * @brief Renders the contents of the in-memory buffer to the screen,
     * scaled to a factor of @ref pixel_size
     */
    void flush_buffer(Display& display);

    /*!
     * @brief Updates the screen
     */
    void finalize_frame();

    void set_render_target(SDL_Texture* target);
    void set_clip_rect(std::optional<Rect> rect);

   private:
    /*!
     * @brief Creates an initial in-memory buffer and sets the SDL_Renderer
     * to target it.
     */
    void create_and_target_buffer();
  };
} // namespace Pixils

#endif
