#include "pixils/binding/clipboard_namespace.h"

#include <pixils/clipboard.h>

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <roo/runtime/value.h>

#include <memory>
#include <string>

namespace Pixils::Clipboard
{
  namespace
  {
    struct SdlTextDeleter
    {
      void operator()(char* text) const { SDL_free(text); }
    };

    class SdlBackend : public Backend
    {
     public:
      std::string get_text() override
      {
        std::unique_ptr<char, SdlTextDeleter> text(SDL_GetClipboardText());
        if (!text) return "";
        return std::string(text.get());
      }

      bool has_text() override { return SDL_HasClipboardText() == true; }

      bool set_text(const std::string& text, std::string* error) override
      {
        if (SDL_SetClipboardText(text.c_str()) == 0) return true;
        if (error) *error = SDL_GetError();
        return false;
      }
    };

    SdlBackend sdl_backend;
    Backend* test_backend = nullptr;
  } // namespace

  void set_backend_for_testing(Backend* backend)
  {
    test_backend = backend;
  }

  Backend& backend()
  {
    if (test_backend) return *test_backend;
    return sdl_backend;
  }
} // namespace Pixils::Clipboard

namespace Pixils::Script
{

  namespace Function
  {
    FUNC_IMPL(ClipboardGetText,
              SIG((NO_ARGS, EXEC_DISPATCH(&ClipboardGetText::exec_get_text))));

    EXEC_BODY(ClipboardGetText, exec_get_text)
    {
      return Roo::string(Clipboard::backend().get_text());
    }

    FUNC_IMPL(ClipboardHasText,
              SIG((NO_ARGS, EXEC_DISPATCH(&ClipboardHasText::exec_has_text))));

    EXEC_BODY(ClipboardHasText, exec_has_text)
    {
      return Clipboard::backend().has_text() ? Roo::Constant::BOOL_TRUE
                                             : Roo::Constant::BOOL_FALSE;
    }

    FUNC_IMPL(ClipboardSetTextBang,
              SIG((FN_ARGS((&Roo::Type::STRING)),
                   EXEC_DISPATCH(&ClipboardSetTextBang::exec_set_text))));

    EXEC_BODY(ClipboardSetTextBang, exec_set_text)
    {
      std::string error;
      if (!Clipboard::backend().set_text(args[0]->str(), &error))
      {
        throw Roo::InvocationException(std::string("Could not set clipboard text: ") +
                                       error);
      }

      return Roo::Constant::BOOL_TRUE;
    }
  } // namespace Function

  ClipboardNamespace::ClipboardNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__CLIPBOARD))
  {
    values.emplace("get-text", Function::ClipboardGetText::make());
    values.emplace("has-text?", Function::ClipboardHasText::make());
    values.emplace("set-text!", Function::ClipboardSetTextBang::make());
  }
} // namespace Pixils::Script
