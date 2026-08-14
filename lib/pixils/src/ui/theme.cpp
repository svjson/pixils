#include <pixils/benchmark/counters.h>
#include <pixils/ui/theme.h>

#include <algorithm>
#include <roo/runtime/dict.h>

namespace Pixils::UI
{
  namespace
  {
    Roo::sptr_val state_property(const ThemeMatchContext& ctx, const Roo::sptr_val& key)
    {
      if (ctx.ui_state && ctx.ui_state->type == Roo::Value::Type::MAP &&
          Roo::Dict::contains_key(*ctx.ui_state, key->str()))
      {
        return Roo::Dict::get_property(ctx.ui_state, key);
      }
      return Roo::Dict::get_property(ctx.state, key);
    }

    bool interaction_matches_selector(const ThemeSelector& selector,
                                      const ThemeMatchContext& ctx)
    {
      if (selector.hovered && !ctx.interaction.hovered) return false;
      if (selector.focused && !ctx.interaction.focused) return false;
      if (selector.focus_within && !ctx.interaction.focus_within) return false;
      if (selector.disabled)
      {
        auto disabled = state_property(ctx, Roo::keyword("disabled?"));
        if (!disabled || !Roo::is_truthy(*disabled)) return false;
      }
      return true;
    }

    int interaction_specificity(const ThemeSelector& selector)
    {
      return static_cast<int>(selector.hovered) + static_cast<int>(selector.focused) +
             static_cast<int>(selector.focus_within) + static_cast<int>(selector.disabled);
    }

    bool rtval_equal(const Roo::sptr_val& lhs, const Roo::sptr_val& rhs)
    {
      if (lhs == rhs) return true;
      if (!lhs || !rhs) return false;
      if (lhs->type != rhs->type) return false;

      try
      {
        return *lhs == *rhs;
      }
      catch (...)
      {
        return lhs->to_string() == rhs->to_string();
      }
    }

    bool state_subset_matches(const Roo::sptr_val& selector_state,
                              const Roo::sptr_val& view_state)
    {
      if (!selector_state || selector_state->type != Roo::Value::Type::MAP) return false;
      if (!view_state || view_state->type != Roo::Value::Type::MAP) return false;

      for (const auto& key : Roo::Dict::keys(*selector_state))
      {
        auto expected = Roo::Dict::get_property(selector_state, key);
        auto actual = Roo::Dict::get_property(view_state, *key);
        if (!actual || !rtval_equal(expected, actual)) return false;
      }

      return true;
    }

    bool state_subset_matches(const Roo::sptr_val& selector_state,
                              const ThemeMatchContext& ctx)
    {
      if (!selector_state || selector_state->type != Roo::Value::Type::MAP) return false;

      for (const auto& key : Roo::Dict::keys(*selector_state))
      {
        auto expected = Roo::Dict::get_property(selector_state, key);
        auto actual = state_property(ctx, key);
        if (!actual || !rtval_equal(expected, actual)) return false;
      }

      return true;
    }

    bool state_maps_equal(const Roo::sptr_val& lhs, const Roo::sptr_val& rhs)
    {
      return state_subset_matches(lhs, rhs) && state_subset_matches(rhs, lhs);
    }

    struct SelectorMatch
    {
      size_t component_inheritance_distance = 0;
    };

    std::optional<SelectorMatch> match_selector(const ThemeSelector& selector,
                                                const ThemeMatchContext& ctx)
    {
      if (!interaction_matches_selector(selector, ctx))
      {
        return std::nullopt;
      }

      switch (selector.type)
      {
      case ThemeSelector::Type::COMPONENT_TYPE:
      {
        auto mode = std::find(ctx.mode_names.begin(), ctx.mode_names.end(), selector.value);
        if (mode == ctx.mode_names.end()) return std::nullopt;
        return SelectorMatch{.component_inheritance_distance =
                               static_cast<size_t>(mode - ctx.mode_names.begin())};
      }
      case ThemeSelector::Type::CLASS_NAME:
        if (std::find(ctx.class_names.begin(), ctx.class_names.end(), selector.value) ==
            ctx.class_names.end())
        {
          return std::nullopt;
        }
        return SelectorMatch{};
      case ThemeSelector::Type::STATE:
        if (!state_subset_matches(selector.state, ctx)) return std::nullopt;
        return SelectorMatch{};
      case ThemeSelector::Type::COMPOUND:
      {
        SelectorMatch compound_match;
        for (const auto& child : selector.children)
        {
          auto child_match = match_selector(child, ctx);
          if (!child_match) return std::nullopt;
          compound_match.component_inheritance_distance +=
            child_match->component_inheritance_distance;
        }
        return compound_match;
      }
      case ThemeSelector::Type::DESCENDANT:
        return std::nullopt;
      }

      return std::nullopt;
    }

    std::optional<SelectorMatch> match_descendant_chain(
      const std::vector<ThemeSelector>& selectors,
      size_t selector_idx,
      const std::vector<ThemeMatchContext>& path,
      size_t path_idx)
    {
      auto current_match = match_selector(selectors[selector_idx], path[path_idx]);
      if (!current_match) return std::nullopt;

      if (selector_idx == 0) return current_match;
      if (path_idx == 0) return std::nullopt;

      std::optional<SelectorMatch> best_match;
      for (size_t i = path_idx; i-- > 0;)
      {
        auto ancestor_match = match_descendant_chain(selectors, selector_idx - 1, path, i);
        if (!ancestor_match) continue;

        SelectorMatch match{.component_inheritance_distance =
                              ancestor_match->component_inheritance_distance +
                              current_match->component_inheritance_distance};
        if (!best_match || match.component_inheritance_distance <
                             best_match->component_inheritance_distance)
        {
          best_match = match;
        }
      }

      return best_match;
    }

    std::optional<SelectorMatch> match_selector_path(
      const ThemeSelector& selector,
      const std::vector<ThemeMatchContext>& path)
    {
      if (path.empty()) return std::nullopt;
      if (selector.type != ThemeSelector::Type::DESCENDANT)
      {
        return match_selector(selector, path.back());
      }
      if (selector.children.empty() || selector.children.size() > path.size())
      {
        return std::nullopt;
      }
      return match_descendant_chain(selector.children,
                                    selector.children.size() - 1,
                                    path,
                                    path.size() - 1);
    }

  } // namespace

  ThemeSelector ThemeSelector::component_type(const std::string& value)
  {
    ThemeSelector selector;
    selector.type = Type::COMPONENT_TYPE;
    selector.value = value;
    return selector;
  }

  ThemeSelector ThemeSelector::class_name(const std::string& value)
  {
    ThemeSelector selector;
    selector.type = Type::CLASS_NAME;
    selector.value = value;
    return selector;
  }

  ThemeSelector ThemeSelector::state_match(const Roo::sptr_val& value)
  {
    ThemeSelector selector;
    selector.type = Type::STATE;
    selector.state = value;
    return selector;
  }

  ThemeSelector ThemeSelector::compound(const std::vector<ThemeSelector>& children)
  {
    ThemeSelector selector;
    selector.type = Type::COMPOUND;
    selector.children = children;
    return selector;
  }

  ThemeSelector ThemeSelector::descendant(const std::vector<ThemeSelector>& children)
  {
    ThemeSelector selector;
    selector.type = Type::DESCENDANT;
    selector.children = children;
    return selector;
  }

  bool ThemeSelector::operator==(const ThemeSelector& other) const
  {
    if (type != other.type) return false;
    if (value != other.value) return false;
    if (hovered != other.hovered) return false;
    if (focused != other.focused) return false;
    if (focus_within != other.focus_within) return false;
    if (disabled != other.disabled) return false;
    if (type == Type::STATE)
    {
      if (!state_maps_equal(state, other.state)) return false;
    }
    else if (!rtval_equal(state, other.state))
    {
      return false;
    }
    if (children.size() != other.children.size()) return false;

    for (size_t i = 0; i < children.size(); i++)
    {
      if (!(children[i] == other.children[i])) return false;
    }

    return true;
  }

  bool ThemeSelector::matches(const ThemeMatchContext& ctx) const
  {
    return match_selector(*this, ctx).has_value();
  }

  bool ThemeSelector::matches_path(const std::vector<ThemeMatchContext>& path) const
  {
    return match_selector_path(*this, path).has_value();
  }

  int ThemeSelector::specificity() const
  {
    int specificity = interaction_specificity(*this);

    switch (type)
    {
    case Type::COMPONENT_TYPE:
      return 1 + specificity;
    case Type::CLASS_NAME:
      return 1 + specificity;
    case Type::STATE:
      return specificity + (state && state->type == Roo::Value::Type::MAP
                              ? static_cast<int>(Roo::Dict::keys(*state).size())
                              : 1);
    case Type::COMPOUND:
    case Type::DESCENDANT:
    {
      int sum = specificity;
      for (const auto& child : children)
      {
        sum += child.specificity();
      }
      return sum;
    }
    }

    return 0;
  }

  void Theme::set_style(const ThemeSelector& selector,
                        const Style& style,
                        const std::vector<Roo::sptr_val>& style_exprs)
  {
    declarations_resolved = false;
    auto it = std::find_if(rules.begin(),
                           rules.end(),
                           [&](const auto& rule) { return rule.selector == selector; });

    if (it == rules.end())
    {
      rules.push_back(
        ThemeRule{.selector = selector, .style = style, .style_exprs = style_exprs});
    }
    else
    {
      apply_style_variant(it->style, style);
      it->style_exprs.insert(it->style_exprs.end(), style_exprs.begin(), style_exprs.end());
    }
  }

  void Theme::set_variant_style(const std::string& variant,
                                const ThemeSelector& selector,
                                const Style& style,
                                const std::vector<Roo::sptr_val>& style_exprs)
  {
    declarations_resolved = false;
    auto& rules_for_variant = variant_rules[variant];
    auto it = std::find_if(rules_for_variant.begin(),
                           rules_for_variant.end(),
                           [&](const auto& rule) { return rule.selector == selector; });

    if (it == rules_for_variant.end())
    {
      rules_for_variant.push_back(
        ThemeRule{.selector = selector, .style = style, .style_exprs = style_exprs});
    }
    else
    {
      apply_style_variant(it->style, style);
      it->style_exprs.insert(it->style_exprs.end(), style_exprs.begin(), style_exprs.end());
    }
  }

  const Style* Theme::get_style(const ThemeSelector& selector) const
  {
    auto it = std::find_if(rules.begin(),
                           rules.end(),
                           [&](const auto& rule) { return rule.selector == selector; });
    if (it == rules.end()) return nullptr;
    return &it->style;
  }

  const Style* Theme::get_variant_style(const std::string& variant,
                                        const ThemeSelector& selector) const
  {
    auto variant_it = variant_rules.find(variant);
    const auto& source_rules =
      variant_it == variant_rules.end() ? rules : variant_it->second;
    auto it = std::find_if(source_rules.begin(),
                           source_rules.end(),
                           [&](const auto& rule) { return rule.selector == selector; });
    if (it == source_rules.end()) return nullptr;
    return &it->style;
  }

  std::vector<const Style*> Theme::get_matching_styles(const ThemeMatchContext& ctx) const
  {
    return get_matching_styles(std::vector<ThemeMatchContext>{ctx});
  }

  std::vector<const Style*> Theme::get_matching_styles(
    const std::vector<ThemeMatchContext>& path) const
  {
    PIXILS_BENCHMARK_COUNT(theme_matching_calls);

    struct MatchingRule
    {
      const ThemeRule* rule;
      SelectorMatch selector_match;
    };

    std::vector<MatchingRule> matching;
    const auto& source_rules = rules;
    matching.reserve(source_rules.size());
    PIXILS_BENCHMARK_ADD(theme_index_candidate_rules,
                         static_cast<std::int64_t>(source_rules.size()));
    PIXILS_BENCHMARK_ADD(theme_full_selector_match_checks,
                         static_cast<std::int64_t>(source_rules.size()));
    PIXILS_BENCHMARK_ADD(theme_rule_match_checks,
                         static_cast<std::int64_t>(source_rules.size()));

    for (size_t i = 0; i < source_rules.size(); i++)
    {
      auto selector_match = match_selector_path(source_rules[i].selector, path);
      if (selector_match)
      {
        matching.push_back(
          MatchingRule{.rule = &source_rules[i], .selector_match = *selector_match});
      }
    }
    PIXILS_BENCHMARK_ADD(theme_rules_rejected,
                         static_cast<std::int64_t>(source_rules.size() - matching.size()));
    PIXILS_BENCHMARK_ADD(theme_rules_matched, static_cast<std::int64_t>(matching.size()));

    std::stable_sort(matching.begin(),
                     matching.end(),
                     [](const auto& lhs, const auto& rhs)
                     {
                       int lhs_specificity = lhs.rule->selector.specificity();
                       int rhs_specificity = rhs.rule->selector.specificity();
                       if (lhs_specificity != rhs_specificity)
                       {
                         return lhs_specificity < rhs_specificity;
                       }
                       return lhs.selector_match.component_inheritance_distance >
                              rhs.selector_match.component_inheritance_distance;
                     });

    std::vector<const Style*> result;
    result.reserve(matching.size());
    for (const auto& match : matching)
    {
      result.push_back(&match.rule->style);
    }

    return result;
  }

  Theme Theme::resolved_for_variant(const std::optional<std::string>& variant) const
  {
    Theme resolved = *this;
    auto target_variant = variant;
    if (target_variant &&
        resolved.variant_rules.find(*target_variant) == resolved.variant_rules.end() &&
        resolved.vars.find(*target_variant) == resolved.vars.end())
    {
      target_variant = resolved.default_variant;
    }

    if (resolved.selected_variant == target_variant)
    {
      return resolved;
    }

    resolved.selected_variant = target_variant;
    if (resolved.selected_variant &&
        resolved.variant_rules.find(*resolved.selected_variant) ==
          resolved.variant_rules.end() &&
        resolved.vars.find(*resolved.selected_variant) == resolved.vars.end())
    {
      resolved.selected_variant = resolved.default_variant;
    }
    if (resolved.selected_variant)
    {
      auto it = resolved.variant_rules.find(*resolved.selected_variant);
      if (it != resolved.variant_rules.end())
      {
        const auto& variant_rules = it->second;
        for (const auto& rule : variant_rules)
        {
          resolved.set_style(rule.selector, rule.style, rule.style_exprs);
        }
      }

      auto defaults_it = resolved.variant_defaults.find(*resolved.selected_variant);
      if (defaults_it != resolved.variant_defaults.end())
      {
        if (!resolved.defaults) resolved.defaults = Style{};
        apply_style_variant(*resolved.defaults, defaults_it->second);
      }
      auto defaults_exprs_it =
        resolved.variant_defaults_exprs.find(*resolved.selected_variant);
      if (defaults_exprs_it != resolved.variant_defaults_exprs.end())
      {
        resolved.defaults_exprs.insert(resolved.defaults_exprs.end(),
                                       defaults_exprs_it->second.begin(),
                                       defaults_exprs_it->second.end());
      }
    }
    return resolved;
  }

  void overlay_theme(Theme& out, const Theme& overlay)
  {
    out.declarations_resolved = false;
    if (overlay.defaults)
    {
      if (!out.defaults) out.defaults = Style{};
      apply_style_variant(*out.defaults, *overlay.defaults);
    }
    out.defaults_exprs.insert(out.defaults_exprs.end(),
                              overlay.defaults_exprs.begin(),
                              overlay.defaults_exprs.end());
    for (const auto& rule : overlay.rules)
    {
      out.set_style(rule.selector, rule.style, rule.style_exprs);
    }
    for (const auto& [variant, defaults] : overlay.variant_defaults)
    {
      auto& out_defaults = out.variant_defaults[variant];
      apply_style_variant(out_defaults, defaults);
    }
    for (const auto& [variant, defaults_exprs] : overlay.variant_defaults_exprs)
    {
      auto& out_defaults_exprs = out.variant_defaults_exprs[variant];
      out_defaults_exprs.insert(out_defaults_exprs.end(),
                                defaults_exprs.begin(),
                                defaults_exprs.end());
    }
    for (const auto& [variant, rules] : overlay.variant_rules)
    {
      for (const auto& rule : rules)
      {
        out.set_variant_style(variant, rule.selector, rule.style, rule.style_exprs);
      }
    }
    for (const auto& [variant, vars] : overlay.vars)
    {
      auto& out_vars = out.vars[variant];
      for (const auto& [key, value] : vars)
      {
        out_vars[key] = value;
      }
    }
    if (!overlay.var_layers.empty())
    {
      out.var_layers.insert(out.var_layers.end(),
                            overlay.var_layers.begin(),
                            overlay.var_layers.end());
    }
    else if (!overlay.vars.empty())
    {
      out.var_layers.push_back(
        ThemeVarLayer{.default_variant = overlay.default_variant, .vars = overlay.vars});
    }
    if (overlay.default_variant) out.default_variant = overlay.default_variant;
    if (overlay.selected_variant) out.selected_variant = overlay.selected_variant;
  }
} // namespace Pixils::UI
