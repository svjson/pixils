#include "../fixture.h"

#include <pixils/clipboard.h>

#include <gtest/gtest.h>

using ClipboardTest = BaseFixture;

namespace
{
  class InMemoryClipboardBackend : public Pixils::Clipboard::Backend
  {
   public:
    std::string text;

    std::string get_text() override { return text; }
    bool has_text() override { return !text.empty(); }
    bool set_text(const std::string& next_text, std::string*) override
    {
      text = next_text;
      return true;
    }
  };

  class ScopedClipboardBackend
  {
   public:
    ScopedClipboardBackend() { Pixils::Clipboard::set_backend_for_testing(&backend); }
    ~ScopedClipboardBackend() { Pixils::Clipboard::set_backend_for_testing(nullptr); }

    InMemoryClipboardBackend backend;
  };
} // namespace

TEST_F(ClipboardTest, clipboard_text_round_trips_through_native_namespace)
{
  ScopedClipboardBackend clipboard;

  auto set_result = runtime.eval("(pixils.clipboard/set-text! \"hello clipboard\")");
  ASSERT_NE(set_result, nullptr);
  EXPECT_EQ(set_result->to_string(), "true");

  auto has_text = runtime.eval("(pixils.clipboard/has-text?)");
  ASSERT_NE(has_text, nullptr);
  EXPECT_EQ(has_text->to_string(), "true");

  auto text = runtime.eval("(pixils.clipboard/get-text)");
  ASSERT_NE(text, nullptr);
  EXPECT_EQ(text->to_string(), "\"hello clipboard\"");
}
