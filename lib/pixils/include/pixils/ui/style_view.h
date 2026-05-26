#ifndef PIXILS__UI__STYLE_VIEW_H
#define PIXILS__UI__STYLE_VIEW_H

#include <pixils/benchmark/counters.h>
#include <pixils/ui/interaction.h>
#include <pixils/ui/theme.h>

#include <cstdint>
#include <optional>
#include <string>

namespace Pixils::UI
{
  class StyleView
  {
   public:
    void set_parent(const StyleView* parent)
    {
      if (parent_view == parent) return;
      parent_view = parent;
      invalidate();
    }

    const StyleView* parent() const { return parent_view; }

    void invalidate()
    {
      PIXILS_BENCHMARK_COUNT(style_view_invalidations);
      cache_valid = false;
      local_generation++;
    }

    bool valid_for(const void* mode,
                   const void* state,
                   const InteractionState& interaction,
                   const Theme* inherited_theme,
                   std::uint64_t parent_generation) const
    {
      return cache_valid && mode_key == mode && state_key == state &&
             inherited_theme_key == inherited_theme &&
             parent_generation_key == parent_generation &&
             hovered_key == interaction.hovered && focused_key == interaction.focused &&
             focus_within_key == interaction.focus_within;
    }

    void mark_resolved(const void* mode,
                       const void* state,
                       const InteractionState& interaction,
                       const Theme* inherited_theme,
                       std::uint64_t parent_generation)
    {
      mode_key = mode;
      state_key = state;
      inherited_theme_key = inherited_theme;
      parent_generation_key = parent_generation;
      hovered_key = interaction.hovered;
      focused_key = interaction.focused;
      focus_within_key = interaction.focus_within;
      cache_valid = true;
      resolved_generation++;
      PIXILS_BENCHMARK_COUNT(style_view_resolutions);
    }

    bool theme_valid_for(const void* mode,
                         const Theme* inherited_theme,
                         const Theme* view_inherited_theme,
                         std::uint64_t inherited_theme_generation,
                         const std::optional<std::string>& selected_variant) const
    {
      return theme_cache_valid && theme_mode_key == mode &&
             theme_inherited_theme_key == inherited_theme &&
             theme_view_inherited_theme_key == view_inherited_theme &&
             theme_inherited_theme_generation_key == inherited_theme_generation &&
             theme_selected_variant_key == selected_variant;
    }

    void mark_theme_resolved(const void* mode,
                             const Theme* inherited_theme,
                             const Theme* view_inherited_theme,
                             std::uint64_t inherited_theme_generation,
                             const std::optional<std::string>& selected_variant)
    {
      theme_mode_key = mode;
      theme_inherited_theme_key = inherited_theme;
      theme_view_inherited_theme_key = view_inherited_theme;
      theme_inherited_theme_generation_key = inherited_theme_generation;
      theme_selected_variant_key = selected_variant;
      theme_cache_valid = true;
      theme_resolved_generation++;
    }

    std::uint64_t generation() const { return resolved_generation + local_generation; }
    std::uint64_t theme_generation() const { return theme_resolved_generation; }

   private:
    const StyleView* parent_view = nullptr;
    bool cache_valid = false;
    const void* mode_key = nullptr;
    const void* state_key = nullptr;
    const Theme* inherited_theme_key = nullptr;
    std::uint64_t parent_generation_key = 0;
    bool hovered_key = false;
    bool focused_key = false;
    bool focus_within_key = false;
    std::uint64_t local_generation = 1;
    std::uint64_t resolved_generation = 1;
    bool theme_cache_valid = false;
    const void* theme_mode_key = nullptr;
    const Theme* theme_inherited_theme_key = nullptr;
    const Theme* theme_view_inherited_theme_key = nullptr;
    std::uint64_t theme_inherited_theme_generation_key = 0;
    std::optional<std::string> theme_selected_variant_key = std::nullopt;
    std::uint64_t theme_resolved_generation = 1;
  };
} // namespace Pixils::UI

#endif /* PIXILS__UI__STYLE_VIEW_H */
