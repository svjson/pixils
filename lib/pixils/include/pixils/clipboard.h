#ifndef PIXILS__CLIPBOARD_H
#define PIXILS__CLIPBOARD_H

#include <string>

namespace Pixils::Clipboard
{
  class Backend
  {
   public:
    virtual ~Backend() = default;

    virtual std::string get_text() = 0;
    virtual bool has_text() = 0;
    virtual bool set_text(const std::string& text, std::string* error) = 0;
  };

  void set_backend_for_testing(Backend* backend);
  Backend& backend();
} // namespace Pixils::Clipboard

#endif /* PIXILS__CLIPBOARD_H */
