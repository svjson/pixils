#include "input_simulator.h"

InputSimulator::InputSimulator(Pixils::FrameEvents& events)
  : _events(events)
{
}

void InputSimulator::mouse_move(Coord position)
{
  _mouse_pos = position;
  _events.do_mouse_motion(position.first, position.second);
}

void InputSimulator::mouse_move(int x, int y)
{
  mouse_move({x, y});
}

void InputSimulator::mouse_down(Uint8 button)
{
  SDL_MouseButtonEvent mouse_button_event{};
  mouse_button_event.button = button;
  _events.do_mouse_button_down(mouse_button_event);
}

void InputSimulator::mouse_down(Coord position, Uint8 button)
{
  mouse_move(position);
  mouse_down(button);
}

void InputSimulator::mouse_up(Uint8 button)
{
  SDL_MouseButtonEvent mouse_button_event{};
  mouse_button_event.button = button;
  _events.do_mouse_button_up(mouse_button_event);
}

void InputSimulator::mouse_up(Coord position, Uint8 button)
{
  mouse_move(position);
  mouse_up(button);
}

void InputSimulator::key_down(SDL_Keycode key)
{
  SDL_KeyboardEvent key_event{};
  key_event.keysym.sym = key;
  _events.do_key_down(key_event);
}

void InputSimulator::key_up(SDL_Keycode key)
{
  SDL_KeyboardEvent key_event{};
  key_event.keysym.sym = key;
  _events.do_key_up(key_event);
}

void InputSimulator::clear_transients()
{
  _events.mouse_button_down = Lisple::Constant::NIL;
  _events.mouse_button_up = Lisple::Constant::NIL;
  _events.mouse_moved = false;
  _events.key_down = Lisple::Constant::NIL;
  _events.key_up = Lisple::Constant::NIL;
}
