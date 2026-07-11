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

void InputSimulator::mouse_down(Uint8 button, Uint8 clicks)
{
  SDL_MouseButtonEvent mouse_button_event{};
  mouse_button_event.button = button;
  mouse_button_event.clicks = clicks;
  _events.do_mouse_button_down(mouse_button_event);
}

void InputSimulator::mouse_down(Coord position, Uint8 button, Uint8 clicks)
{
  mouse_move(position);
  mouse_down(button, clicks);
}

void InputSimulator::mouse_up(Uint8 button, Uint8 clicks)
{
  SDL_MouseButtonEvent mouse_button_event{};
  mouse_button_event.button = button;
  mouse_button_event.clicks = clicks;
  _events.do_mouse_button_up(mouse_button_event);
}

void InputSimulator::mouse_up(Coord position, Uint8 button, Uint8 clicks)
{
  mouse_move(position);
  mouse_up(button, clicks);
}

void InputSimulator::mouse_wheel(float x, float y)
{
  SDL_MouseWheelEvent mouse_wheel_event{};
  mouse_wheel_event.x = x;
  mouse_wheel_event.y = y;
  mouse_wheel_event.mouse_x = static_cast<float>(_mouse_pos.first);
  mouse_wheel_event.mouse_y = static_cast<float>(_mouse_pos.second);
  _events.do_mouse_wheel(mouse_wheel_event);
}

void InputSimulator::mouse_wheel(Coord position, float x, float y)
{
  _mouse_pos = position;
  _events.set_mouse_position(position.first, position.second);
  mouse_wheel(x, y);
}

void InputSimulator::key_down(SDL_Keycode key)
{
  SDL_KeyboardEvent key_event{};
  key_event.key = key;
  _events.do_key_down(key_event);
}

void InputSimulator::key_up(SDL_Keycode key)
{
  SDL_KeyboardEvent key_event{};
  key_event.key = key;
  _events.do_key_up(key_event);
}

void InputSimulator::clear_transients()
{
  _events.clear_transients();
}
