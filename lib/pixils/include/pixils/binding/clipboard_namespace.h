#ifndef PIXILS__CLIPBOARD_NAMESPACE_H
#define PIXILS__CLIPBOARD_NAMESPACE_H

#include <roo/exec.h>
#include <roo/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__CLIPBOARD = "pixils.clipboard";

  namespace Function
  {
    FUNC(ClipboardGetText, get_text);
    FUNC(ClipboardHasText, has_text);
    FUNC(ClipboardSetTextBang, set_text);
  } // namespace Function

  class ClipboardNamespace : public Roo::Namespace
  {
   public:
    ClipboardNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__CLIPBOARD_NAMESPACE_H */
