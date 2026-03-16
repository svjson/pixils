
#ifndef PIXILS__CONSOLE_H
#define PIXILS__CONSOLE_H

#include <pixils/context.h>
#include <pixils/geom.h>
#include <pixils/pretty_printer.h>
#include <pixils/text.h>
#include <pixils/timer.h>

#include <SDL2/SDL_rect.h>
#include <list>
#include <map>
#include <string>

typedef union SDL_Event SDL_Event;
struct SDL_KeyboardEvent;

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{

  struct Textures;

  /*!
   * @brief Determines the size of the text scrollback buffer.
   *
   * When the limit is reach, the oldest entries will begin to be cycled
   * out as new lines are added.
   */
  extern const int BUFFER_MAX_SIZE;
  /*!
   * @brief Determines the size of the console history.
   *
   * When the limit is reached, the oldest commands will begin to be cycled
   * out as new commands are added.
   */
  extern const int HISTORY_MAX_SIZE;
  /*!
   * @brief Dotfile name where the console history is stored on disk.
   */
  extern const std::string HISTORY_FILE_NAME;

  /*!
   * @brief A graphical console overlay containing a Lisple REPL
   *
   * Previous commands are stored in a history dotfile and are accessible
   * between application executions.
   */
  class ConsoleOverlay
  {
   public:
    /*!
     * @brief The current state of the console overlay.
     * - OPEN - The console panel is open and visible
     * - CLOSED - The console panel is inactive and not visible.
     * - OPENING - The console panel is in the process of being opened
     * - CLOSING - The console panel is in the process of being closed
     */
    enum State
    {
      OPEN,
      CLOSED,
      OPENING,
      CLOSING
    };

   private:
    RenderContext rc;
    /*!
     * @brief The on-screen(or rendering buffer) coordinate and dimension
     * where this component will be rendered
     */
    SDL_Rect bounds = {0, 0, 0, 0};
    Lisple::Runtime& runtime;
    Text::Renderer text_renderer;
    Text::Cursor tc;
    SDL_Rect window_size = {0, 0, 0, 0};
    Timer timer;
    State open_state = CLOSED;
    float open_fraction = 0;
    Point cursor_pos = {0, 0};
    ObjectPrinter pretty_printer;

    std::string prompt;
    /*!
     * @brief The scrollback buffer of previous output
     */
    std::list<TextLine> buffer;
    /*!
     * @brief The command history containing previously evaluated commands
     */
    std::list<std::string> history;
    int history_position = -1;

    /*!
     * @brief The current offset in the scrollback history.
     */
    int scroll_offset = 0;
    unsigned int line_width;

    std::string deferred_input = "";
    std::string input;

   public:
    ConsoleOverlay(RenderContext rc, Lisple::Runtime& runtime, SDL_Texture* textures);

    /*!
     * @brief Must be called once every frame to forward the swipe in or out
     * graphical effect on opening/closing the overlay.
     */
    void tick();
    /*!
     * @brief Renders the current state of the ConsoleOverlay.
     */
    void render(RenderContext& rc);

    /*!
     * @brief Adds a line of input to the command history.
     */
    void add_to_history(const std::string& item);
    /*!
     * @brief Appends a new line to the scrollback buffer
     * and handles cycling out of old lines when the buffer
     * limit has been reached.
     */
    void buffer_append(const TextLine& line);
    /*!
     * @brief Evaluates and executes a REPL command.
     */
    void execute_command(const std::string& input);

    State get_open_state();
    /*!
     * @brief Set the console panel to an OPENING state
     */
    void open();
    /*!
     * @brief Set the console panel to an CLOSING state
     */
    void close();
    void set_window_size(const SDL_Rect& rect);

    /*!
     * @brief Updates the prompt with the current namespace of the runtime.
     */
    void update_prompt();

    /*!
     * @brief Scrolls the console text buffer an @a amount of lines.
     *
     * A negative value will scroll up, while a positive value will scroll
     * down.
     */
    void scroll(int amount);
    /*!
     * @brief Returns the number of visible text lines of the text buffer at
     * the current zoom level.
     */
    int get_visible_lines() const;
    /*!
     * @brief Get the scrollback buffer size.
     */
    int get_max_scrollback() const;

    /*!
     * @brief Reads the command history from disk and makes it available
     * to the console panel.
     */
    void restore_history();
    /*!
     * @brief Writes the in-memory command history to the history dot file.
     */
    void write_history();

    bool on_keydown(SDL_KeyboardEvent& event);
  };

  /*!
   * @brief Built-in font map for the fixed width console font
   */
  inline Text::FontMap console_font_map(
    {{'A', SDL_Rect{0, 0, 5, 9}},    {'B', SDL_Rect{6, 0, 5, 9}},
     {'C', SDL_Rect{12, 0, 5, 9}},   {'D', SDL_Rect{18, 0, 5, 9}},
     {'E', SDL_Rect{24, 0, 5, 9}},   {'F', SDL_Rect{30, 0, 5, 9}},
     {'G', SDL_Rect{36, 0, 5, 9}},   {'H', SDL_Rect{42, 0, 5, 9}},
     {'I', SDL_Rect{48, 0, 5, 9}},   {'J', SDL_Rect{54, 0, 5, 9}},
     {'K', SDL_Rect{60, 0, 5, 9}},   {'L', SDL_Rect{66, 0, 5, 9}},
     {'M', SDL_Rect{72, 0, 5, 9}},   {'N', SDL_Rect{0, 9, 5, 9}},
     {'O', SDL_Rect{6, 9, 5, 9}},    {'P', SDL_Rect{12, 9, 5, 9}},
     {'Q', SDL_Rect{18, 9, 5, 9}},   {'R', SDL_Rect{24, 9, 5, 9}},
     {'S', SDL_Rect{30, 9, 5, 9}},   {'T', SDL_Rect{36, 9, 5, 9}},
     {'U', SDL_Rect{42, 9, 5, 9}},   {'V', SDL_Rect{48, 9, 5, 9}},
     {'W', SDL_Rect{54, 9, 5, 9}},   {'X', SDL_Rect{60, 9, 5, 9}},
     {'Y', SDL_Rect{66, 9, 5, 9}},   {'Z', SDL_Rect{72, 9, 5, 9}},
     {'a', SDL_Rect{0, 18, 5, 9}},   {'b', SDL_Rect{6, 18, 5, 9}},
     {'c', SDL_Rect{12, 18, 5, 9}},  {'d', SDL_Rect{18, 18, 5, 9}},
     {'e', SDL_Rect{24, 18, 5, 9}},  {'f', SDL_Rect{30, 18, 5, 9}},
     {'g', SDL_Rect{36, 18, 5, 9}},  {'h', SDL_Rect{42, 18, 5, 9}},
     {'i', SDL_Rect{48, 18, 5, 9}},  {'j', SDL_Rect{54, 18, 5, 9}},
     {'k', SDL_Rect{60, 18, 5, 9}},  {'l', SDL_Rect{66, 18, 5, 9}},
     {'m', SDL_Rect{72, 18, 5, 9}},  {'n', SDL_Rect{0, 27, 5, 9}},
     {'o', SDL_Rect{6, 27, 5, 9}},   {'p', SDL_Rect{12, 27, 5, 9}},
     {'q', SDL_Rect{18, 27, 5, 9}},  {'r', SDL_Rect{24, 27, 5, 9}},
     {'s', SDL_Rect{30, 27, 5, 9}},  {'t', SDL_Rect{36, 27, 5, 9}},
     {'u', SDL_Rect{42, 27, 5, 9}},  {'v', SDL_Rect{48, 27, 5, 9}},
     {'w', SDL_Rect{54, 27, 5, 9}},  {'x', SDL_Rect{60, 27, 5, 9}},
     {'y', SDL_Rect{66, 27, 5, 9}},  {'z', SDL_Rect{72, 27, 5, 9}},
     {'+', SDL_Rect{0, 36, 5, 9}},   {'-', SDL_Rect{6, 36, 5, 9}},
     {'=', SDL_Rect{12, 36, 5, 9}},  {'/', SDL_Rect{18, 36, 5, 9}},
     {'\\', SDL_Rect{24, 36, 5, 9}}, {'*', SDL_Rect{30, 36, 5, 9}},
     {':', SDL_Rect{36, 36, 5, 9}},  {';', SDL_Rect{42, 36, 5, 9}},
     {'(', SDL_Rect{48, 36, 5, 9}},  {')', SDL_Rect{54, 36, 5, 9}},
     {'[', SDL_Rect{60, 36, 5, 9}},  {']', SDL_Rect{66, 36, 5, 9}},
     {'{', SDL_Rect{72, 36, 5, 9}},  {'}', SDL_Rect{0, 45, 5, 9}},
     {'<', SDL_Rect{6, 45, 5, 9}},   {'>', SDL_Rect{12, 45, 5, 9}},
     {'!', SDL_Rect{18, 45, 5, 9}},  {'?', SDL_Rect{24, 45, 5, 9}},
     {'.', SDL_Rect{30, 45, 5, 9}},  {',', SDL_Rect{36, 45, 5, 9}},
     {'\'', SDL_Rect{42, 45, 5, 9}}, {'"', SDL_Rect{48, 45, 5, 9}},
     {'&', SDL_Rect{54, 45, 5, 9}},  {L'¡', SDL_Rect{60, 45, 5, 9}},
     {'#', SDL_Rect{66, 45, 5, 9}},  {'%', SDL_Rect{72, 45, 5, 9}},
     {'^', SDL_Rect{0, 54, 5, 9}},   {'~', SDL_Rect{6, 54, 5, 9}},
     {'"', SDL_Rect{12, 54, 5, 9}},  {'`', SDL_Rect{18, 54, 5, 9}},
     {'|', SDL_Rect{24, 54, 5, 9}},  {L'¦', SDL_Rect{30, 54, 5, 9}},
     {'$', SDL_Rect{36, 54, 5, 9}},  {L'¢', SDL_Rect{42, 54, 5, 9}},
     {L'£', SDL_Rect{48, 54, 5, 9}}, {L'€', SDL_Rect{54, 54, 5, 9}},
     {L'¤', SDL_Rect{60, 54, 5, 9}}, {L'¥', SDL_Rect{66, 54, 5, 9}},
     {'@', SDL_Rect{72, 54, 5, 9}},  {L'§', SDL_Rect{0, 63, 5, 9}},
     {'1', SDL_Rect{6, 63, 5, 9}},   {'2', SDL_Rect{12, 63, 5, 9}},
     {'3', SDL_Rect{18, 63, 5, 9}},  {'4', SDL_Rect{24, 63, 5, 9}},
     {'5', SDL_Rect{30, 63, 5, 9}},  {'6', SDL_Rect{36, 63, 5, 9}},
     {'7', SDL_Rect{42, 63, 5, 9}},  {'8', SDL_Rect{48, 63, 5, 9}},
     {'9', SDL_Rect{54, 63, 5, 9}},  {'0', SDL_Rect{60, 63, 5, 9}},
     {'_', SDL_Rect{66, 63, 5, 9}},  {' ', SDL_Rect{72, 63, 5, 9}}});
} // namespace Pixils

#endif
