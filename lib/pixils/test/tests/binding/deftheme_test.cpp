#include "../fixture.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/program.h>
#include <pixils/ui/theme.h>

#include <gtest/gtest.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>

using DefThemeTest = BaseFixture;

namespace
{
  Pixils::UI::Theme& get_theme(Lisple::Runtime& rt, const std::string& name)
  {
    auto val = rt.eval("(get pixils/themes '" + name + ")");
    return Lisple::obj<Pixils::UI::Theme>(*val);
  }
} // namespace

TEST_F(DefThemeTest, deftheme_with_component_and_class_selectors_is_created)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {'text-node {:text {:scale 2}}
                :menu/item {:text {:font :font/console}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");
  EXPECT_EQ(theme.name, "test-theme");

  auto text_node = theme.get_style(Pixils::UI::ThemeSelector::component_type("text-node"));
  ASSERT_NE(text_node, nullptr);
  ASSERT_TRUE(text_node->text.has_value());
  ASSERT_TRUE(text_node->text->scale.has_value());
  EXPECT_EQ(*text_node->text->scale, 2);

  auto menu_item = theme.get_style(Pixils::UI::ThemeSelector::class_name("menu/item"));
  ASSERT_NE(menu_item, nullptr);
  ASSERT_TRUE(menu_item->text.has_value());
  ASSERT_TRUE(menu_item->text->font.has_value());
  EXPECT_EQ(*menu_item->text->font, "font/console");
}

TEST_F(DefThemeTest, deftheme_extend_merges_parent_styles_and_overrides_them)
{
  runtime.eval(R"(
    (pixils/deftheme base-theme
      {:styles {'text-node {:text {:font :font/console
                                   :scale 1}}
                :menu/item {:text {:color {:r 0 :g 0 :b 0 :a 255}}}}})

    (pixils/deftheme child-theme
      {:extend 'base-theme
       :styles {'text-node {:text {:scale 3}}
                :status/panel {:text {:font :font/status}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "child-theme");
  ASSERT_EQ(theme.extend.size(), 1u);
  EXPECT_EQ(theme.extend[0], "base-theme");

  auto text_node = theme.get_style(Pixils::UI::ThemeSelector::component_type("text-node"));
  ASSERT_NE(text_node, nullptr);
  ASSERT_TRUE(text_node->text.has_value());
  ASSERT_TRUE(text_node->text->font.has_value());
  ASSERT_TRUE(text_node->text->scale.has_value());
  EXPECT_EQ(*text_node->text->font, "font/console");
  EXPECT_EQ(*text_node->text->scale, 3);

  auto menu_item = theme.get_style(Pixils::UI::ThemeSelector::class_name("menu/item"));
  ASSERT_NE(menu_item, nullptr);
  ASSERT_TRUE(menu_item->text.has_value());
  ASSERT_TRUE(menu_item->text->color.has_value());
  EXPECT_EQ(menu_item->text->color->r, 0);

  auto status_panel = theme.get_style(Pixils::UI::ThemeSelector::class_name("status/panel"));
  ASSERT_NE(status_panel, nullptr);
  ASSERT_TRUE(status_panel->text.has_value());
  ASSERT_TRUE(status_panel->text->font.has_value());
  EXPECT_EQ(*status_panel->text->font, "font/status");
}

TEST_F(DefThemeTest, defprogram_with_theme_is_created)
{
  runtime.eval(R"(
    (pixils/deftheme app-theme {:styles {}})
    (pixils/defprogram app {:initial-mode 'root-mode
                            :theme 'app-theme})
  )");

  auto program_val = runtime.eval("(get pixils/programs 'app)");
  Pixils::Program& program = Lisple::obj<Pixils::Program>(*program_val);
  ASSERT_TRUE(program.theme.has_value());
  EXPECT_EQ(*program.theme, "app-theme");
}
