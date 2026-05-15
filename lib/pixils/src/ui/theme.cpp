#include <pixils/ui/theme.h>

#include <algorithm>
#include <lisple/runtime/dict.h>

namespace Pixils::UI
{
  namespace
  {
    bool interaction_matches_selector(const ThemeSelector& selector,
                                      const ThemeMatchContext& ctx)
    {
      if (selector.hovered && !ctx.interaction.hovered) return false;
      if (selector.focused && !ctx.interaction.focused) return false;
      if (selector.focus_within && !ctx.interaction.focus_within) return false;
      return true;
    }

    int interaction_specificity(const ThemeSelector& selector)
    {
      return static_cast<int>(selector.hovered) + static_cast<int>(selector.focused) +
             static_cast<int>(selector.focus_within);
    }

    bool rtval_equal(const Lisple::sptr_val& lhs, const Lisple::sptr_val& rhs)
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

    bool state_subset_matches(const Lisple::sptr_val& selector_state,
                              const Lisple::sptr_val& view_state)
    {
      if (!selector_state || selector_state->type != Lisple::Value::Type::MAP) return false;
      if (!view_state || view_state->type != Lisple::Value::Type::MAP) return false;

      for (const auto& key : Lisple::Dict::keys(*selector_state))
      {
        auto expected = Lisple::Dict::get_property(selector_state, key);
        auto actual = Lisple::Dict::get_property(view_state, *key);
        if (!actual || !rtval_equal(expected, actual)) return false;
      }

      return true;
    }

    bool state_maps_equal(const Lisple::sptr_val& lhs, const Lisple::sptr_val& rhs)
    {
      return state_subset_matches(lhs, rhs) && state_subset_matches(rhs, lhs);
    }

    bool matches_descendant_chain(const std::vector<ThemeSelector>& selectors,
                                  size_t selector_idx,
                                  const std::vector<ThemeMatchContext>& path,
                                  size_t path_idx)
    {
      if (!selectors[selector_idx].matches(path[path_idx]))
      {
        return false;
      }

      if (selector_idx == 0)
      {
        return true;
      }

      if (path_idx == 0)
      {
        return false;
      }

      for (size_t i = path_idx; i-- > 0;)
      {
        if (matches_descendant_chain(selectors, selector_idx - 1, path, i))
        {
          return true;
        }
      }

      return false;
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

  ThemeSelector ThemeSelector::state_match(const Lisple::sptr_val& value)
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
    if (!interaction_matches_selector(*this, ctx))
    {
      return false;
    }

    switch (type)
    {
    case Type::COMPONENT_TYPE:
      return std::find(ctx.mode_names.begin(), ctx.mode_names.end(), value) !=
             ctx.mode_names.end();
    case Type::CLASS_NAME:
      return std::find(ctx.class_names.begin(), ctx.class_names.end(), value) !=
             ctx.class_names.end();
    case Type::STATE:
      return state_subset_matches(state, ctx.state);
    case Type::COMPOUND:
      return std::all_of(children.begin(),
                         children.end(),
                         [&](const auto& child) { return child.matches(ctx); });
    case Type::DESCENDANT:
      return false;
    }

    return false;
  }

  bool ThemeSelector::matches_path(const std::vector<ThemeMatchContext>& path) const
  {
    if (path.empty())
    {
      return false;
    }

    if (type != Type::DESCENDANT)
    {
      return matches(path.back());
    }

    if (children.empty() || children.size() > path.size())
    {
      return false;
    }

    return matches_descendant_chain(children, children.size() - 1, path, path.size() - 1);
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
      return specificity + (state && state->type == Lisple::Value::Type::MAP
                              ? static_cast<int>(Lisple::Dict::keys(*state).size())
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

  void Theme::set_style(const ThemeSelector& selector, const Style& style)
  {
    auto it = std::find_if(rules.begin(),
                           rules.end(),
                           [&](const auto& rule) { return rule.selector == selector; });

    if (it == rules.end())
    {
      rules.push_back(ThemeRule{.selector = selector, .style = style});
    }
    else
    {
      apply_style_variant(it->style, style);
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

  std::vector<const Style*> Theme::get_matching_styles(const ThemeMatchContext& ctx) const
  {
    return get_matching_styles(std::vector<ThemeMatchContext>{ctx});
  }

  std::vector<const Style*> Theme::get_matching_styles(
    const std::vector<ThemeMatchContext>& path) const
  {
    struct MatchingRule
    {
      const ThemeRule* rule;
    };

    std::vector<MatchingRule> matching;
    matching.reserve(rules.size());

    for (size_t i = 0; i < rules.size(); i++)
    {
      if (rules[i].selector.matches_path(path))
      {
        matching.push_back(MatchingRule{.rule = &rules[i]});
      }
    }

    std::stable_sort(
      matching.begin(),
      matching.end(),
      [](const auto& lhs, const auto& rhs)
      { return lhs.rule->selector.specificity() < rhs.rule->selector.specificity(); });

    std::vector<const Style*> result;
    result.reserve(matching.size());
    for (const auto& match : matching)
    {
      result.push_back(&match.rule->style);
    }

    return result;
  }

  void overlay_theme(Theme& out, const Theme& overlay)
  {
    for (const auto& rule : overlay.rules)
    {
      out.set_style(rule.selector, rule.style);
    }
  }
} // namespace Pixils::UI
