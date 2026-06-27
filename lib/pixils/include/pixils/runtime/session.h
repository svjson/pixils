
#ifndef __PIXILS__RUNTIME__SESSION_H_
#define __PIXILS__RUNTIME__SESSION_H_

#include <pixils/geom.h>
#include <pixils/runtime/hook_arguments.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/mode_stack.h>
#include <pixils/ui/focus_state.h>
#include <pixils/ui/mouse_state.h>
#include <pixils/ui/theme.h>

#include <optional>
#include <string>
#include <vector>

namespace Pixils
{
  struct RenderContext;
}

namespace Pixils::Asset
{
  class Registry;
}

namespace Pixils::Runtime
{
  struct View;

  struct Session
  {
    struct ModeFrameMetadata
    {
      View* origin_view = nullptr;
      Roo::sptr_val origin_event = Roo::Constant::NIL;
      UI::FocusState restore_focus;
    };

    Roo::Runtime& roo_runtime;
    Asset::Registry& assets;
    RenderContext& render_ctx;
    ModeStack mode_stack;
    Roo::sptr_val modes;
    std::shared_ptr<View> active_mode;
    std::vector<std::shared_ptr<View>> ctx_stack;
    std::vector<ModeFrameMetadata> frame_metadata;
    HookArguments hook_args;
    UI::FocusState focus_state;
    UI::MouseState mouse_state;
    bool quit_requested = false;
    std::optional<std::vector<std::string>> application_theme = std::nullopt;
    std::optional<std::string> application_theme_variant = std::nullopt;
    std::optional<UI::Theme> resolved_application_theme = std::nullopt;

    Session(Roo::Runtime& roo_runtime,
            Asset::Registry& assets,
            RenderContext& render_ctx,
            const HookArguments& hook_args);

    void pop_mode(const Roo::sptr_val& payload = Roo::Constant::NIL);
    void push_mode(const Roo::sptr_val& mode,
                   const Roo::sptr_val& state,
                   const Roo::sptr_val& overrides = Roo::Constant::NIL);
    void push_mode(const std::string& mode_name,
                   const Roo::sptr_val& state,
                   const Roo::sptr_val& overrides = Roo::Constant::NIL);
    void set_application_theme(const std::optional<std::vector<std::string>>& theme,
                               const std::optional<std::string>& variant = std::nullopt);
    bool process_messages();
    void update_mode();
    void render_mode();
  };

} // namespace Pixils::Runtime

#endif /* __PIXILS__RUNTIME__SESSION_H_ */
