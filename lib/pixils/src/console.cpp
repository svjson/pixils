#include <pixils/console.h>
#include <pixils/context.h>
#include <pixils/geom.h>
#include <pixils/keyboard.h>
#include <pixils/pretty_printer.h>
#include <pixils/text.h>
#include <pixils/timer.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <lisple/exception.h>
#include <lisple/namespace.h>
#include <lisple/runtime.h>
#include <lisple/type.h>
#include <memory>
#include <sstream>
#include <vector>

namespace Pixils
{
  /*!
   * @brief The amount of frames the animation of sliding in the console
   * will take up.
   */
  float anim_frames = 15;

  const int BUFFER_MAX_SIZE = 500;
  const int HISTORY_MAX_SIZE = 100;
  const std::string HISTORY_FILE_NAME = ".console_history";

  /*!
   * @brief Color configuration for the ConsoleOverlay.
   * Specifices the color of the prompt.
   */
  Color PROMPT_COLOR = {0x2b, 0x83, 0x14, 0xff};
  /*!
   * @brief Color configuration for the ConsoleOverlay.
   * Specifices the color of the user input.
   */
  Color INPUT_COLOR = {0xff, 0xff, 0xff, 0xff};
  /*!
   * @brief Color configuration for the ConsoleOverlay.
   * Specifices the color of error messages.
   */
  Color ERROR_COLOR = Color{0xf6, 0x40, 0x23};
  TextLine EMPTY_LINE = TextLine{{}};

  ConsoleOverlay::ConsoleOverlay(RenderContext& rc,
                                 Lisple::Runtime& runtime,
                                 SDL_Texture* font_map_texture)
    : rc(rc)
    , runtime(runtime)
    , text_renderer(font_map_texture, console_font_map, 1, rc.pixel_size)
    , tc(text_renderer, {0xff, 0xff, 0xff, 0xff}, {0x2b, 0x83, 0x14, 0xff}, 10)
  {
    update_prompt();
    restore_history();
  }

  void ConsoleOverlay::tick()
  {
    if (open_state == OPENING || open_state == CLOSING)
    {
      if (timer.is_elapsed(5))
      {
        timer.reset();
        if (open_state == OPENING)
        {
          open_fraction += 1.0 / anim_frames;
        }
        else if (open_state == CLOSING)
        {
          open_fraction -= 1.0 / anim_frames;
        }

        if (open_fraction >= 1.0)
        {
          open_fraction = 1.0;
          this->open_state = OPEN;
        }
        else if (open_fraction <= 0.0)
        {
          open_fraction = 0.0;
          this->open_state = CLOSED;
        }
      }
    }
    bounds.y = window_size.h - ((window_size.h / 2.0) * open_fraction);
  }

  bool ConsoleOverlay::on_keydown(SDL_KeyboardEvent& event)
  {
    if (event.keysym.sym == SDLK_BACKSPACE)
    {
      if (cursor_pos.x > 0)
      {
        if (cursor_pos.x == static_cast<int>(input.size()))
        {
          input = input.substr(0, input.size() - 1);
        }
        else
        {
          input = input.substr(0, cursor_pos.x - 1) + input.substr(cursor_pos.x);
        }
        cursor_pos.x--;
      }
    }
    if (event.keysym.sym == SDLK_DELETE)
    {
      if (input.size() && cursor_pos.x < static_cast<int>(input.size()))
      {
        input = input.substr(0, cursor_pos.x) + input.substr(cursor_pos.x + 1);
      }
    }
    if (event.keysym.sym == SDLK_k && event.keysym.mod & KMOD_CTRL)
    {
      if (input.size() && cursor_pos.x < static_cast<int>(input.size()))
      {
        input = input.substr(0, cursor_pos.x);
      }
    }
    else if (event.keysym.sym == SDLK_LEFT)
    {
      if (cursor_pos.x > 0) cursor_pos.x--;
    }
    else if (event.keysym.sym == SDLK_RIGHT)
    {
      if (cursor_pos.x < static_cast<int>(input.size())) cursor_pos.x++;
    }
    else if (event.keysym.sym == SDLK_PAGEUP && event.keysym.mod & KMOD_SHIFT)
    {
      scroll(-get_visible_lines() / 2);
    }
    else if (event.keysym.sym == SDLK_PAGEDOWN && event.keysym.mod & KMOD_SHIFT)
    {
      scroll(get_visible_lines() / 2);
    }
    else if (event.keysym.sym == SDLK_UP)
    {
      if (history_position == 0)
      {
        return false;
      }

      if (history_position == -1)
      {
        deferred_input = input;
      }

      if (history_position == -1 && history.size() > 0)
      {
        history_position = history.size() - 1;
      }
      else if (history_position > 0)
      {
        history_position--;
      }

      if (history_position == -1)
      {
        input = deferred_input;
      }
      else
      {
        auto it = history.begin();
        std::advance(it, history_position);
        input = *it;
      }
      cursor_pos.x = input.size();
    }
    else if (event.keysym.sym == SDLK_DOWN)
    {
      if (history_position == -1)
      {
        return false;
      }

      if (history_position == static_cast<int>(history.size()) - 1)
      {
        history_position = -1;
      }
      else if (history_position < static_cast<int>(history.size()))
      {
        history_position++;
      }

      if (history_position == -1)
      {
        input = deferred_input;
      }
      else
      {
        auto it = history.begin();
        std::advance(it, history_position);
        input = *it;
      }

      cursor_pos.x = input.size();
    }
    else if (event.keysym.sym == SDLK_PLUS && event.keysym.mod & KMOD_CTRL)
    {
      this->rc.pixel_size = std::min(10, rc.pixel_size + 1);
      this->text_renderer.set_scale(this->rc.pixel_size);
    }
    else if (event.keysym.sym == SDLK_MINUS && event.keysym.mod & KMOD_CTRL)
    {
      this->rc.pixel_size = std::max(1, rc.pixel_size - 1);
      this->text_renderer.set_scale(this->rc.pixel_size);
    }
    else if (event.keysym.sym == SDLK_HOME)
    {
      cursor_pos.x = 0;
    }
    else if (event.keysym.sym == SDLK_END)
    {
      cursor_pos.x = input.size();
    }
    else if (event.keysym.sym == SDLK_RETURN || event.keysym.sym == SDLK_KP_ENTER)
    {
      if (input.empty()) return false;
      execute_command(input);
      input = "";
      cursor_pos = {0, 0};
      history_position = -1;
      scroll_offset = 0;
      update_prompt();
    }
    else
    {
      auto input_char = Keyboard::key_to_char(event);

      if (input_char && text_renderer.supports_char(*input_char))
      {
        if (cursor_pos.x == static_cast<int>(input.size()))
        {
          input += std::string{*input_char};
        }
        else
        {
          input = input.substr(0, cursor_pos.x) + std::string{*input_char} +
                  input.substr(cursor_pos.x);
        }
        cursor_pos.x++;
      }
    }

    return false;
  }

  void ConsoleOverlay::update_prompt()
  {
    this->prompt = "@" + runtime.get_current_namespace().get_name() + "=>@";
  }

  void ConsoleOverlay::execute_command(const std::string& cmd)
  {
    add_to_history(cmd);

    TextLine prompt_line;
    prompt_line.segments.push_back(TextSegment{prompt, PROMPT_COLOR});
    prompt_line.segments.push_back(TextSegment{cmd, INPUT_COLOR});
    for (auto& line : prompt_line.split_lines(line_width))
    {
      buffer_append(line);
    }

    std::vector<TextLine> output;

    try
    {
      Lisple::sptr_sobject result = runtime.eval_ast(input);
      output = pretty_printer.pretty_print(result);
    }
    catch (Lisple::LispleException& e)
    {
      std::stringstream ss(e.what());
      std::string line;
      while (std::getline(ss, line, '\n'))
      {
        output.push_back(TextLine{{TextSegment{line, ERROR_COLOR}}});
      }
    }

    for (auto& line : output)
    {
      std::vector<TextLine> lines = line.split_lines(line_width);
      for (auto& line : lines)
      {
        buffer_append(line);
      }
    }
    buffer_append(EMPTY_LINE);
  }

  void ConsoleOverlay::add_to_history(const std::string& input)
  {
    if (!history.empty() && input == history.back()) return;

    history.push_back(input);
    while (history.size() >= HISTORY_MAX_SIZE)
    {
      history.erase(history.begin());
    }
    write_history();
  }

  void ConsoleOverlay::buffer_append(const TextLine& input)
  {
    buffer.push_back(input);
    while (buffer.size() >= BUFFER_MAX_SIZE)
    {
      buffer.erase(buffer.begin());
    }
  }

  void ConsoleOverlay::render(RenderContext&)
  {
    int char_width = text_renderer.get_scale() * 6;
    this->line_width = (bounds.w - 10 * rc.pixel_size) / char_width;

    SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rc.renderer, 0x80, 0x80, 0x80, 0x90);
    SDL_RenderFillRect(rc.renderer, &bounds);

    int margin_left = 5 * rc.pixel_size;
    // Prompt contains color code control chars, so need to subtract 2 when checking for
    // size
    int prompt_w = 6 * (prompt.size() - 2) * text_renderer.get_scale();

    int lines = get_visible_lines();

    int buffer_start = static_cast<int>(buffer.size()) - lines + scroll_offset;

    tc.move_to(bounds.x + 5 * text_renderer.get_scale(),
               bounds.y + 2 * text_renderer.get_scale());
    for (int i = 0; i < lines; i++)
    {
      if (buffer_start + i >= 0)
      {
        auto it = buffer.begin();
        std::advance(it, buffer_start + i);
        if (it != buffer.end())
        {
          TextLine& line = *it;
          for (auto& segment : line.segments)
          {
            tc.set_color(
              SDL_Color{segment.color.r, segment.color.g, segment.color.b, segment.color.a});
            tc.print(rc, segment.text);
          }
        }
      }
      tc.println(rc, "");
    }

    tc.move_to(
      bounds.x + 5 * rc.pixel_size,
      (bounds.y + 2 * rc.pixel_size) + (lines * (tc.get_line_height() * rc.pixel_size)));
    tc.set_color({0xff, 0xff, 0xff, 0xff});
    tc.print(rc, prompt + input);

    SDL_Rect cursor_rect = {
      (bounds.x + margin_left + prompt_w) + (6 * rc.pixel_size * cursor_pos.round_x()),
      tc.get_position().round_y(),
      6 * rc.pixel_size,
      9 * rc.pixel_size};
    SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rc.renderer, 0x60, 0x78, 0xc4, 0x80);
    SDL_RenderFillRect(rc.renderer, &cursor_rect);
  }

  void ConsoleOverlay::scroll(int amount)
  {
    if (amount < 0)
    {
      scroll_offset = std::max(scroll_offset + amount, get_max_scrollback());
    }
    else
    {
      scroll_offset = std::min(0, scroll_offset + amount);
    }
  }

  int ConsoleOverlay::get_visible_lines() const
  {
    int lines = -1 + (bounds.h + 2 * rc.pixel_size) / (10 * rc.pixel_size);
    if ((bounds.h + 2 * rc.pixel_size) % (10 * rc.pixel_size > 28)) lines--;
    return lines;
  }

  int ConsoleOverlay::get_max_scrollback() const
  {
    return -(buffer.size() - get_visible_lines());
  }

  void ConsoleOverlay::set_window_size(const SDL_Rect& rect)
  {
    this->window_size = rect;
    if (open_state == State::OPEN) this->bounds.y = rect.h / 2;
    this->bounds.h = rect.h / 2;
    this->bounds.w = rect.w;
  }

  ConsoleOverlay::State ConsoleOverlay::get_open_state()
  {
    return open_state;
  }

  void ConsoleOverlay::open()
  {
    this->open_state = State::OPENING;
    timer.reset();
  }

  void ConsoleOverlay::close()
  {
    this->open_state = State::CLOSING;
    timer.reset();
  }

  void ConsoleOverlay::write_history()
  {
    std::ofstream file(HISTORY_FILE_NAME);

    for (const std::string& item : history)
    {
      file << item << std::endl;
    }

    file.close();
  }

  void ConsoleOverlay::restore_history()
  {
    std::ifstream file(HISTORY_FILE_NAME);

    if (file.is_open())
    {
      std::string line;
      while (std::getline(file, line))
      {
        history.push_back(line);
      }
    }
  }
} // namespace Pixils
