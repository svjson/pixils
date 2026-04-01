
#ifndef __PIXILS__RENDER_CONTEXT_H_
#define __PIXILS__RENDER_CONTEXT_H_

#include "geom.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

namespace Pixils
{
  struct Display;

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
    SDL_Window* window;
    /*!
     * @brief Pointer to the SDL renderer that does all rendering
     */
    SDL_Renderer* renderer;

    /*!
     * @brief The buffer texture that all in-game drawing happens against
     */
    SDL_Texture* buffer_texture = nullptr;
    /*!
     * @brief The current size/dimension of the in-memory buffer
     */
    Dimension buffer_dim{0, 0};

    /*
     * @brief The dimensions of the application window
     */
    Rect window_rect{0, 0, 0, 0};

    /*!
     * @brief The on-screen size of game pixels, effetively the scaling
     * factor to use when rendering the in-memory buffer to the screen.
     */
    int pixel_size = 5;

    /*!
     * @brief The size of map tiles
     */
    int tile_size = 16;

    Asset::Registry* asset_registry = nullptr;

    Dimension get_window_dimension();

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

   private:
    /*!
     * @brief Creates an initial in-memory buffer and sets the SDL_Renderer
     * to target it.
     */
    void create_and_target_buffer();
  };
} // namespace Pixils

#endif
