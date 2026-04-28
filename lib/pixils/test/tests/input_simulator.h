#ifndef PIXILS__TEST__INPUT_SIMULATOR_H
#define PIXILS__TEST__INPUT_SIMULATOR_H

#include <pixils/frame_events.h>

#include <SDL2/SDL_mouse.h>
#include <utility>

class InputSimulator
{
 public:
  using Coord = std::pair<int, int>;

  explicit InputSimulator(Pixils::FrameEvents& events);

  void mouse_move(Coord position);
  void mouse_move(int x, int y);
  void mouse_down(Uint8 button = SDL_BUTTON_LEFT);
  void mouse_down(Coord position, Uint8 button = SDL_BUTTON_LEFT);
  void mouse_up(Uint8 button = SDL_BUTTON_LEFT);
  void mouse_up(Coord position, Uint8 button = SDL_BUTTON_LEFT);

  /**
   * Clear per-frame transient events while preserving held buttons/keys.
   */
  void clear_transients();

 private:
  Pixils::FrameEvents& _events;
  Coord _mouse_pos = {0, 0};
};

#endif
