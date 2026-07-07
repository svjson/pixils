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
    bool set_succeeds = true;

    std::string get_text() override { return text; }
    bool has_text() override { return !text.empty(); }
    bool set_text(const std::string& next_text, std::string* error) override
    {
      if (!set_succeeds)
      {
        if (error) *error = "test clipboard failure";
        return false;
      }
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

TEST_F(ClipboardTest, clipboard_set_text_returns_false_when_backend_fails)
{
  ScopedClipboardBackend clipboard;
  clipboard.backend.text = "sentinel";
  clipboard.backend.set_succeeds = false;

  auto set_result = runtime.eval("(pixils.clipboard/set-text! \"new text\")");
  ASSERT_NE(set_result, nullptr);
  EXPECT_EQ(set_result->to_string(), "false");
  EXPECT_EQ(clipboard.backend.text, "sentinel");
}
