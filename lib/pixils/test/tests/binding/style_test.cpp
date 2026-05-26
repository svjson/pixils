
#include "../fixture.h"
#include <pixils/runtime/mode.h>
#include <pixils/ui/style.h>

#include <gtest/gtest.h>
#include <lisple/form.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>
#include <optional>

using StyleTest = BaseFixture;

TEST_F(StyleTest, make_minimal_style)
{
  // When
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:width 40})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.width, std::nullopt);
  EXPECT_TRUE(style.width->is_fixed());
  EXPECT_EQ(style.width->fixed_value_or(0), 40);
}

TEST_F(StyleTest, make_uniform_border)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:border {:thickness 1 :line-style :solid}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.border, std::nullopt);
  EXPECT_EQ(style.border->thickness, 1);
  EXPECT_EQ(style.border->line_style, Pixils::UI::Style::LineStyle::SOLID);
}

TEST_F(StyleTest, make_style_with_box_sizing)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:box-sizing :content-box})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.box_sizing, std::nullopt);
  EXPECT_EQ(*style.box_sizing, Pixils::UI::Style::BoxSizing::CONTENT_BOX);
}

TEST_F(StyleTest, make_style_with_background_image_layout)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style "
                                         "{:background {:image :icons/brush "
                                         ":source {:x 1 :y 2 :w 3 :h 4} "
                                         ":fit :contain "
                                         ":align :center "
                                         ":offset {:x 5 :y 6} "
                                         ":opacity 0.5}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.background, std::nullopt);
  ASSERT_NE(style.background->image, std::nullopt);
  EXPECT_EQ(style.background->image->first, "icons");
  EXPECT_EQ(style.background->image->second, "brush");
  ASSERT_NE(style.background->source, std::nullopt);
  EXPECT_EQ(style.background->source->x, 1);
  EXPECT_EQ(style.background->source->y, 2);
  EXPECT_EQ(style.background->source->w, 3);
  EXPECT_EQ(style.background->source->h, 4);
  ASSERT_NE(style.background->fit, std::nullopt);
  EXPECT_EQ(*style.background->fit, Pixils::UI::Style::Background::Fit::CONTAIN);
  ASSERT_NE(style.background->align_x, std::nullopt);
  ASSERT_NE(style.background->align_y, std::nullopt);
  EXPECT_EQ(*style.background->align_x, Pixils::UI::Style::Background::Align::CENTER);
  EXPECT_EQ(*style.background->align_y, Pixils::UI::Style::Background::Align::CENTER);
  ASSERT_NE(style.background->offset, std::nullopt);
  EXPECT_EQ(style.background->offset->round_x(), 5);
  EXPECT_EQ(style.background->offset->round_y(), 6);
  ASSERT_NE(style.background->opacity, std::nullopt);
  EXPECT_FLOAT_EQ(*style.background->opacity, 0.5f);
}

TEST_F(StyleTest, make_style_with_opacity)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:opacity 0.25})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.opacity, std::nullopt);
  EXPECT_FLOAT_EQ(*style.opacity, 0.25f);
}

TEST_F(StyleTest, make_style_with_uniform_corner_radius)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:corner-radius 6})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.corner_radius, std::nullopt);
  EXPECT_EQ(*style.corner_radius, Pixils::UI::Style::CornerRadius(6));
}

TEST_F(StyleTest, make_style_with_directional_corner_radius)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:corner-radius {:tl 8 :tr 7 :br 2 :bl 1}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.corner_radius, std::nullopt);
  EXPECT_EQ(*style.corner_radius, (Pixils::UI::Style::CornerRadius{8, 7, 2, 1}));
}

TEST_F(StyleTest, style_adapter_exposes_corner_radius)
{
  Lisple::sptr_val result = runtime.eval(
    "(:br (:corner-radius "
    "(pixils.ui.style/make-style {:corner-radius {:tl 8 :tr 7 :br 2 :bl 1}})))");

  ASSERT_EQ(result->type, Lisple::Value::Type::NUMBER);
  EXPECT_EQ(result->num().get_int(), 2);
}

TEST_F(StyleTest, make_style_with_per_side_border_overrides)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:border {:thickness 2 :line-style :solid "
                 ":right {:thickness 1} :bottom {:thickness 0}}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.border, std::nullopt);
  EXPECT_EQ(style.border->top_thickness(), 2);
  EXPECT_EQ(style.border->right_thickness(), 1);
  EXPECT_EQ(style.border->bottom_thickness(), 0);
  EXPECT_EQ(style.border->left_thickness(), 2);
}

TEST_F(StyleTest, make_style_with_margin)
{
  // When
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:margin [2 4]})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.margin, std::nullopt);
  EXPECT_EQ(style.margin->t, 2);
  EXPECT_EQ(style.margin->r, 4);
  EXPECT_EQ(style.margin->b, 2);
  EXPECT_EQ(style.margin->l, 4);
}

TEST_F(StyleTest, make_style_with_four_value_margin)
{
  // When
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:margin [1 2 3 4]})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.margin, std::nullopt);
  EXPECT_EQ(style.margin->t, 1);
  EXPECT_EQ(style.margin->r, 2);
  EXPECT_EQ(style.margin->b, 3);
  EXPECT_EQ(style.margin->l, 4);
}

TEST_F(StyleTest, make_style_with_layout_direction)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:direction :row "
                 ":align-items :center}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->direction, std::nullopt);
  ASSERT_NE(style.layout->align_items, std::nullopt);
  EXPECT_EQ(*style.layout->direction, Pixils::UI::LayoutDirection::ROW);
  EXPECT_EQ(*style.layout->align_items, Pixils::UI::Style::Layout::AlignItems::CENTER);
}

TEST_F(StyleTest, make_style_with_layout_gap_mode)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap {:mode :space-between}}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::SPACE_BETWEEN);
}

TEST_F(StyleTest, make_style_with_layout_gap_keyword)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap :space-between}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::SPACE_BETWEEN);
}

TEST_F(StyleTest, make_style_with_layout_gap_none_keyword)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap :none}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::NONE);
}

TEST_F(StyleTest, make_style_with_layout_gap_number)
{
  // When
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:layout {:gap 8}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  ASSERT_NE(style.layout->gap->size, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::FIXED);
  EXPECT_EQ(*style.layout->gap->size, 8);
}

TEST_F(StyleTest, make_style_with_fill_and_shrink_sizes)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:width :fill :height :shrink})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.width, std::nullopt);
  ASSERT_NE(style.height, std::nullopt);
  EXPECT_TRUE(style.width->is_fill());
  EXPECT_TRUE(style.height->is_shrink());
}

TEST_F(StyleTest, make_style_with_text)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:text {:color {:r 255 :g 255 :b 255} "
                 ":font :font/console :scale 2 :align :center :wrap :word}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.text, std::nullopt);
  ASSERT_NE(style.text->color, std::nullopt);
  ASSERT_NE(style.text->font, std::nullopt);
  ASSERT_NE(style.text->scale, std::nullopt);
  ASSERT_NE(style.text->align, std::nullopt);
  ASSERT_NE(style.text->wrap, std::nullopt);
  EXPECT_EQ(*style.text->color, (Pixils::Color{255, 255, 255, 255}));
  EXPECT_EQ(*style.text->font, "font/console");
  EXPECT_EQ(*style.text->scale, 2);
  EXPECT_EQ(*style.text->align, Pixils::Text::Alignment::CENTER);
  EXPECT_EQ(*style.text->wrap, Pixils::UI::Style::Text::Wrap::WORD);
}

TEST_F(StyleTest, make_style_with_text_font_styles_shadows_and_marked_style)
{
  Lisple::sptr_val result = runtime.eval(R"(
    (pixils.ui.style/make-style
      {:text {:font :font/console
              :font-styles :underline
              :shadow {:offset {:x 1 :y 2}
                       :color {:r 3 :g 4 :b 5}}
              :marked-style {:enabled true
                             :marker "@"
                             :scale 2
                             :font-styles [:underline]}}})
  )");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.text, std::nullopt);
  ASSERT_NE(style.text->font_styles, std::nullopt);
  ASSERT_NE(style.text->shadows, std::nullopt);
  ASSERT_NE(style.text->marked_style, std::nullopt);
  ASSERT_EQ(style.text->font_styles->size(), 1u);
  EXPECT_EQ(style.text->font_styles->at(0), Pixils::Text::FontStyle::UNDERLINE);
  ASSERT_EQ(style.text->shadows->size(), 1u);
  EXPECT_EQ(style.text->shadows->at(0).offset, (Pixils::Point{1, 2}));
  EXPECT_EQ(style.text->shadows->at(0).color, (Pixils::Color{3, 4, 5, 255}));
  ASSERT_NE(style.text->marked_style->enabled, std::nullopt);
  ASSERT_NE(style.text->marked_style->marker, std::nullopt);
  EXPECT_TRUE(*style.text->marked_style->enabled);
  EXPECT_EQ(*style.text->marked_style->marker, '@');
  ASSERT_NE(style.text->marked_style->scale, std::nullopt);
  EXPECT_EQ(*style.text->marked_style->scale, 2);
  ASSERT_NE(style.text->marked_style->font_styles, std::nullopt);
  ASSERT_EQ(style.text->marked_style->font_styles->size(), 1u);
  EXPECT_EQ(style.text->marked_style->font_styles->at(0),
            Pixils::Text::FontStyle::UNDERLINE);
}

TEST(StyleResolveTest, hover_marked_style_overrides_only_its_own_fields)
{
  Pixils::UI::Style child;
  child.text = Pixils::UI::Style::Text{};
  child.text->marked_style = Pixils::UI::Style::Text::MarkedStyle{};
  child.text->marked_style->enabled = true;
  child.text->marked_style->marker = '@';
  child.text->marked_style->font_styles =
    std::vector<Pixils::Text::FontStyle>{Pixils::Text::FontStyle::UNDERLINE};
  child.hover = std::make_unique<Pixils::UI::Style>();
  child.hover->text = Pixils::UI::Style::Text{};
  child.hover->text->marked_style = Pixils::UI::Style::Text::MarkedStyle{};
  child.hover->text->marked_style->color = Pixils::Color{255, 255, 255, 255};

  Pixils::UI::InteractionState interaction;
  interaction.hovered = true;

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            Lisple::Constant::NIL,
                                            interaction);

  ASSERT_NE(resolved.text, std::nullopt);
  ASSERT_NE(resolved.text->marked_style, std::nullopt);
  ASSERT_NE(resolved.text->marked_style->enabled, std::nullopt);
  ASSERT_NE(resolved.text->marked_style->marker, std::nullopt);
  ASSERT_NE(resolved.text->marked_style->font_styles, std::nullopt);
  ASSERT_NE(resolved.text->marked_style->color, std::nullopt);
  EXPECT_TRUE(*resolved.text->marked_style->enabled);
  EXPECT_EQ(*resolved.text->marked_style->marker, '@');
  EXPECT_EQ(resolved.text->marked_style->font_styles->size(), 1u);
  EXPECT_EQ(resolved.text->marked_style->font_styles->at(0),
            Pixils::Text::FontStyle::UNDERLINE);
  EXPECT_EQ(*resolved.text->marked_style->color, (Pixils::Color{255, 255, 255, 255}));
}

TEST_F(StyleTest, make_style_with_text_color_none)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:text {:color :none}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.text, std::nullopt);
  EXPECT_TRUE(style.text->use_font_color);
  EXPECT_EQ(style.text->color, std::nullopt);
}

TEST_F(StyleTest, make_style_with_cursor)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:cursor :pointer})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.cursor, std::nullopt);
  EXPECT_EQ(style.cursor->kind, Pixils::UI::CursorSpec::Kind::SYSTEM);
  EXPECT_EQ(style.cursor->system, Pixils::UI::SystemCursor::POINTER);
}

TEST_F(StyleTest, make_style_with_hit_test_disabled)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:hit-test false})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.hit_test, std::nullopt);
  EXPECT_FALSE(*style.hit_test);
}

TEST_F(StyleTest, make_style_with_visibility)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:visibility :hidden})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.visibility, std::nullopt);
  EXPECT_EQ(*style.visibility, Pixils::UI::Style::Visibility::HIDDEN);
}

TEST_F(StyleTest, make_style_maps_hidden_sugar_to_visibility)
{
  auto hidden_result = runtime.eval("(pixils.ui.style/make-style {:hidden true})");
  auto visible_result = runtime.eval("(pixils.ui.style/make-style {:hidden false})");

  auto hidden_style = Lisple::obj<Pixils::UI::Style>(*hidden_result);
  auto visible_style = Lisple::obj<Pixils::UI::Style>(*visible_result);
  ASSERT_NE(hidden_style.visibility, std::nullopt);
  ASSERT_NE(visible_style.visibility, std::nullopt);
  EXPECT_EQ(*hidden_style.visibility, Pixils::UI::Style::Visibility::NONE);
  EXPECT_EQ(*visible_style.visibility, Pixils::UI::Style::Visibility::VISIBLE);
}

TEST_F(StyleTest, make_style_with_named_pointer_cursor)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:cursor :workbench/pointer})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.cursor, std::nullopt);
  EXPECT_EQ(style.cursor->kind, Pixils::UI::CursorSpec::Kind::NAMED);
  EXPECT_EQ(style.cursor->name, "workbench/pointer");
}

TEST_F(StyleTest, make_style_with_inline_image_cursor)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style "
                                         "{:cursor {:image :workbench-assets/cursor "
                                         ":source {:x 1 :y 2 :w 3 :h 4} "
                                         ":hotspot {:x 5 :y 6} "
                                         ":scale 2}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.cursor, std::nullopt);
  EXPECT_EQ(style.cursor->kind, Pixils::UI::CursorSpec::Kind::IMAGE);
  ASSERT_NE(style.cursor->image.image, std::nullopt);
  EXPECT_EQ(style.cursor->image.image->first, "workbench-assets");
  EXPECT_EQ(style.cursor->image.image->second, "cursor");
  ASSERT_NE(style.cursor->image.source, std::nullopt);
  EXPECT_EQ(style.cursor->image.source->x, 1);
  EXPECT_EQ(style.cursor->image.source->y, 2);
  EXPECT_EQ(style.cursor->image.source->w, 3);
  EXPECT_EQ(style.cursor->image.source->h, 4);
  EXPECT_EQ(style.cursor->image.hotspot.round_x(), 5);
  EXPECT_EQ(style.cursor->image.hotspot.round_y(), 6);
  EXPECT_EQ(style.cursor->image.scale, 2);
  EXPECT_EQ(style.cursor->image.render_mode, Pixils::UI::ImageCursor::RenderMode::APP);
}

TEST_F(StyleTest, make_style_with_native_image_cursor)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style "
                                         "{:cursor {:image :workbench-assets/cursor "
                                         ":render :native}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.cursor, std::nullopt);
  EXPECT_EQ(style.cursor->kind, Pixils::UI::CursorSpec::Kind::IMAGE);
  EXPECT_EQ(style.cursor->image.render_mode, Pixils::UI::ImageCursor::RenderMode::NATIVE);
}

TEST_F(StyleTest, defpointer_registers_named_pointer_without_implicit_renaming)
{
  runtime.eval("(pixils/defpointer workbench-pointer "
               "{:image :workbench-assets/cursor "
               ":hotspot {:x 1 :y 2} "
               ":scale 2})");

  auto pointer = render_ctx.pointer_registry.find("workbench-pointer");
  ASSERT_NE(pointer, render_ctx.pointer_registry.end());
  ASSERT_NE(pointer->second.image, std::nullopt);
  EXPECT_EQ(pointer->second.image->first, "workbench-assets");
  EXPECT_EQ(pointer->second.image->second, "cursor");
  EXPECT_EQ(pointer->second.hotspot.round_x(), 1);
  EXPECT_EQ(pointer->second.hotspot.round_y(), 2);
  EXPECT_EQ(pointer->second.scale, 2);
  EXPECT_EQ(render_ctx.pointer_registry.find("workbench/pointer"),
            render_ctx.pointer_registry.end());
}

TEST_F(StyleTest, make_style_with_view_scale)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:scale 2})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.scale, std::nullopt);
  EXPECT_EQ(*style.scale, 2);
}

TEST_F(StyleTest, style_adapter_exposes_view_scale)
{
  Lisple::sptr_val result = runtime.eval("(:scale (pixils.ui.style/make-style {:scale 2}))");

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->to_string(), "2");
}

TEST_F(StyleTest, make_style_with_hover_cursor)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:cursor :default "
                                         ":hover {:cursor :text}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.cursor, std::nullopt);
  EXPECT_EQ(style.cursor->kind, Pixils::UI::CursorSpec::Kind::SYSTEM);
  EXPECT_EQ(style.cursor->system, Pixils::UI::SystemCursor::DEFAULT);
  ASSERT_NE(style.hover, nullptr);
  ASSERT_NE(style.hover->cursor, std::nullopt);
  EXPECT_EQ(style.hover->cursor->kind, Pixils::UI::CursorSpec::Kind::SYSTEM);
  EXPECT_EQ(style.hover->cursor->system, Pixils::UI::SystemCursor::TEXT);
}

TEST_F(StyleTest, make_style_with_text_color_from_host_color_value)
{
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style "
                 "{:text {:color (pixils.color/make-color {:r 255 :g 255 :b 255})}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.text, std::nullopt);
  ASSERT_NE(style.text->color, std::nullopt);
  EXPECT_EQ(*style.text->color, (Pixils::Color{255, 255, 255, 255}));
}

TEST_F(StyleTest, make_style_with_focus_variants)
{
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-style {:focus {:width 80} "
                                         ":focus-within {:height 24}})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.focus, nullptr);
  ASSERT_NE(style.focus->width, std::nullopt);
  EXPECT_TRUE(style.focus->width->is_fixed());
  EXPECT_EQ(style.focus->width->fixed_value_or(0), 80);

  ASSERT_NE(style.focus_within, nullptr);
  ASSERT_NE(style.focus_within->height, std::nullopt);
  EXPECT_TRUE(style.focus_within->height->is_fixed());
  EXPECT_EQ(style.focus_within->height->fixed_value_or(0), 24);
}

TEST(StyleResolveTest, inherited_text_fields_are_applied_fieldwise)
{
  Pixils::UI::Style parent;
  parent.text = Pixils::UI::Style::Text{};
  parent.text->color = Pixils::Color{255, 255, 255, 255};
  parent.text->font = "font/ui";
  parent.text->scale = 2;
  parent.text->align = Pixils::Text::Alignment::RIGHT;
  parent.text->wrap = Pixils::UI::Style::Text::Wrap::WORD;

  Pixils::UI::Style child;
  child.text = Pixils::UI::Style::Text{};
  child.text->color = Pixils::Color{255, 0, 0, 255};

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            &parent,
                                            Lisple::Constant::NIL,
                                            {});

  ASSERT_NE(resolved.text, std::nullopt);
  ASSERT_NE(resolved.text->color, std::nullopt);
  ASSERT_NE(resolved.text->font, std::nullopt);
  ASSERT_NE(resolved.text->scale, std::nullopt);
  ASSERT_NE(resolved.text->align, std::nullopt);
  ASSERT_NE(resolved.text->wrap, std::nullopt);
  EXPECT_EQ(*resolved.text->color, (Pixils::Color{255, 0, 0, 255}));
  EXPECT_EQ(*resolved.text->font, "font/ui");
  EXPECT_EQ(*resolved.text->scale, 2);
  EXPECT_EQ(*resolved.text->align, Pixils::Text::Alignment::RIGHT);
  EXPECT_EQ(*resolved.text->wrap, Pixils::UI::Style::Text::Wrap::WORD);
}

TEST(StyleResolveTest, hover_text_variant_overrides_only_its_own_fields)
{
  Pixils::UI::Style parent;
  parent.text = Pixils::UI::Style::Text{};
  parent.text->font = "font/ui";
  parent.text->scale = 2;
  parent.text->align = Pixils::Text::Alignment::RIGHT;
  parent.text->wrap = Pixils::UI::Style::Text::Wrap::WORD;

  Pixils::UI::Style child;
  child.hover = std::make_unique<Pixils::UI::Style>();
  child.hover->text = Pixils::UI::Style::Text{};
  child.hover->text->color = Pixils::Color{0, 255, 0, 255};

  Pixils::UI::InteractionState interaction;
  interaction.hovered = true;

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            &parent,
                                            Lisple::Constant::NIL,
                                            interaction);

  ASSERT_NE(resolved.text, std::nullopt);
  ASSERT_NE(resolved.text->color, std::nullopt);
  ASSERT_NE(resolved.text->font, std::nullopt);
  ASSERT_NE(resolved.text->scale, std::nullopt);
  ASSERT_NE(resolved.text->align, std::nullopt);
  ASSERT_NE(resolved.text->wrap, std::nullopt);
  EXPECT_EQ(*resolved.text->color, (Pixils::Color{0, 255, 0, 255}));
  EXPECT_EQ(*resolved.text->font, "font/ui");
  EXPECT_EQ(*resolved.text->scale, 2);
  EXPECT_EQ(*resolved.text->align, Pixils::Text::Alignment::RIGHT);
  EXPECT_EQ(*resolved.text->wrap, Pixils::UI::Style::Text::Wrap::WORD);
}

TEST(StyleResolveTest, text_color_none_stops_inherited_text_tint)
{
  Pixils::UI::Style parent;
  parent.text = Pixils::UI::Style::Text{};
  parent.text->color = Pixils::Color{255, 255, 255, 255};

  Pixils::UI::Style child;
  child.text = Pixils::UI::Style::Text{};
  child.text->use_font_color = true;

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            &parent,
                                            Lisple::Constant::NIL,
                                            {});

  ASSERT_NE(resolved.text, std::nullopt);
  EXPECT_TRUE(resolved.text->use_font_color);
  EXPECT_EQ(resolved.text->color, std::nullopt);
}

TEST(StyleResolveTest, focus_variant_overrides_focus_within_on_focused_leaf)
{
  Pixils::UI::Style child;
  child.focus_within = std::make_unique<Pixils::UI::Style>();
  child.focus_within->width = Pixils::UI::Style::Size(120);
  child.focus = std::make_unique<Pixils::UI::Style>();
  child.focus->width = Pixils::UI::Style::Size(200);

  Pixils::UI::InteractionState interaction;
  interaction.focus_within = true;
  interaction.focused = true;

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            Lisple::Constant::NIL,
                                            interaction);

  ASSERT_NE(resolved.width, std::nullopt);
  EXPECT_TRUE(resolved.width->is_fixed());
  EXPECT_EQ(resolved.width->fixed_value_or(0), 200);
}

TEST(StyleVariantTest, apply_style_variant_preserves_hover_style)
{
  Pixils::UI::Style base;
  Pixils::UI::Style variant;
  variant.hover = std::make_unique<Pixils::UI::Style>();
  variant.hover->background = Pixils::UI::Style::Background{Pixils::Color{0, 11, 200, 255}};
  variant.hover->text = Pixils::UI::Style::Text{};
  variant.hover->text->color = Pixils::Color{255, 255, 255, 255};

  Pixils::UI::apply_style_variant(base, variant);

  ASSERT_NE(base.hover, nullptr);
  ASSERT_NE(base.hover->background, std::nullopt);
  ASSERT_NE(base.hover->background->color, std::nullopt);
  ASSERT_NE(base.hover->text, std::nullopt);
  ASSERT_NE(base.hover->text->color, std::nullopt);
  EXPECT_EQ(*base.hover->background->color, (Pixils::Color{0, 11, 200, 255}));
  EXPECT_EQ(*base.hover->text->color, (Pixils::Color{255, 255, 255, 255}));
}

TEST(StyleVariantTest, apply_style_variant_preserves_focus_styles)
{
  Pixils::UI::Style base;
  Pixils::UI::Style variant;
  variant.focus_within = std::make_unique<Pixils::UI::Style>();
  variant.focus_within->background =
    Pixils::UI::Style::Background{Pixils::Color{10, 20, 30, 255}};
  variant.focus = std::make_unique<Pixils::UI::Style>();
  variant.focus->text = Pixils::UI::Style::Text{};
  variant.focus->text->scale = 2;

  Pixils::UI::apply_style_variant(base, variant);

  ASSERT_NE(base.focus_within, nullptr);
  ASSERT_NE(base.focus_within->background, std::nullopt);
  ASSERT_NE(base.focus_within->background->color, std::nullopt);
  EXPECT_EQ(*base.focus_within->background->color, (Pixils::Color{10, 20, 30, 255}));

  ASSERT_NE(base.focus, nullptr);
  ASSERT_NE(base.focus->text, std::nullopt);
  ASSERT_NE(base.focus->text->scale, std::nullopt);
  EXPECT_EQ(*base.focus->text->scale, 2);
}

TEST(StyleVariantTest, apply_style_variant_overlays_corner_radius)
{
  Pixils::UI::Style base;
  base.corner_radius = Pixils::UI::Style::CornerRadius(2);
  Pixils::UI::Style variant;
  variant.corner_radius = Pixils::UI::Style::CornerRadius{8, 7, 2, 1};

  Pixils::UI::apply_style_variant(base, variant);

  ASSERT_NE(base.corner_radius, std::nullopt);
  EXPECT_EQ(*base.corner_radius, (Pixils::UI::Style::CornerRadius{8, 7, 2, 1}));
}

TEST_F(StyleTest, make_insets_with_four_value_vector)
{
  // When
  Lisple::sptr_val result = runtime.eval("(pixils.ui.style/make-insets [1 2 3 4])");

  // Then
  auto insets = Lisple::obj<Pixils::UI::Style::Insets>(*result);
  EXPECT_EQ(insets.t, 1);
  EXPECT_EQ(insets.r, 2);
  EXPECT_EQ(insets.b, 3);
  EXPECT_EQ(insets.l, 4);
}

TEST_F(StyleTest, make_bevel_border)
{
  // When
  Lisple::sptr_val result =
    runtime.eval("(pixils.ui.style/make-style {:border {:thickness 2 :line-style :bevel}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.border, std::nullopt);
  EXPECT_EQ(style.border->thickness, 2);
  EXPECT_EQ(style.border->line_style, Pixils::UI::Style::LineStyle::BEVEL);
}

TEST(StyleTotalDimensionsTest, total_width_includes_padding_and_border)
{
  // Given
  Pixils::UI::Style style;
  style.box_sizing = Pixils::UI::Style::BoxSizing::CONTENT_BOX;
  style.width = 100;
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;

  // Then: content=100, padding=4+4=8, border=2+2=4 -> total=112
  EXPECT_EQ(style.total_width(), 112);
}

TEST(StyleTotalDimensionsTest, total_width_includes_margin_padding_and_border)
{
  // Given
  Pixils::UI::Style style;
  style.box_sizing = Pixils::UI::Style::BoxSizing::CONTENT_BOX;
  style.width = 100;
  style.margin = Pixils::UI::Style::Insets(0, 3, 0, 5);
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;

  // Then: content=100, margin=5+3=8, padding=4+4=8, border=2+2=4 -> total=120
  EXPECT_EQ(style.total_width(), 120);
}

TEST(StyleTotalDimensionsTest, total_height_includes_padding_and_border)
{
  // Given
  Pixils::UI::Style style;
  style.box_sizing = Pixils::UI::Style::BoxSizing::CONTENT_BOX;
  style.height = 50;
  style.padding = Pixils::UI::Style::Insets(3, 0, 3, 0);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 1;

  // Then: content=50, padding=3+3=6, border=1+1=2 -> total=58
  EXPECT_EQ(style.total_height(), 58);
}

TEST(StyleTotalDimensionsTest, total_dimensions_use_effective_per_side_border_thicknesses)
{
  // Given
  Pixils::UI::Style style;
  style.box_sizing = Pixils::UI::Style::BoxSizing::CONTENT_BOX;
  style.width = 13;
  style.height = 13;
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;
  style.border->r = Pixils::UI::Style::Border{};
  style.border->r->thickness = 1;
  style.border->b = Pixils::UI::Style::Border{};
  style.border->b->thickness = 0;

  // Then: width=13 + left 2 + right 1, height=13 + top 2 + bottom 0
  EXPECT_EQ(style.total_width(), 16);
  EXPECT_EQ(style.total_height(), 15);
}

TEST(StyleTotalDimensionsTest, fixed_dimensions_default_to_border_box)
{
  Pixils::UI::Style style;
  style.width = 13;
  style.height = 13;
  style.padding = Pixils::UI::Style::Insets(1, 1, 1, 1);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;

  EXPECT_EQ(style.total_width(), 13);
  EXPECT_EQ(style.total_height(), 13);
}

TEST(StyleContentRectTest, content_rect_insets_by_border_then_padding)
{
  // Given
  Pixils::UI::Style style;
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);

  // When
  Pixils::Rect result = style.content_rect({0, 0, 100, 60});

  // Then: border inset -> {2,2,96,56}, padding inset -> {6,6,88,48}
  EXPECT_EQ(result.x, 6);
  EXPECT_EQ(result.y, 6);
  EXPECT_EQ(result.w, 88);
  EXPECT_EQ(result.h, 48);
}

TEST(StyleContentRectTest, content_rect_with_border_only)
{
  // Given
  Pixils::UI::Style style;
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 3;

  // When
  Pixils::Rect result = style.content_rect({10, 20, 100, 80});

  // Then
  EXPECT_EQ(result.x, 13);
  EXPECT_EQ(result.y, 23);
  EXPECT_EQ(result.w, 94);
  EXPECT_EQ(result.h, 74);
}

TEST(StyleContentRectTest, content_rect_uses_effective_per_side_border_thicknesses)
{
  // Given
  Pixils::UI::Style style;
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;
  style.border->r = Pixils::UI::Style::Border{};
  style.border->r->thickness = 1;
  style.border->b = Pixils::UI::Style::Border{};
  style.border->b->thickness = 0;

  // When
  Pixils::Rect result = style.content_rect({10, 20, 13, 13});

  // Then
  EXPECT_EQ(result.x, 12);
  EXPECT_EQ(result.y, 22);
  EXPECT_EQ(result.w, 10);
  EXPECT_EQ(result.h, 11);
}

TEST(StyleContentRectTest, content_rect_with_no_border_or_padding_returns_bounds)
{
  // Given
  Pixils::UI::Style style;

  // When
  Pixils::Rect result = style.content_rect({5, 10, 200, 100});

  // Then
  EXPECT_EQ(result.x, 5);
  EXPECT_EQ(result.y, 10);
  EXPECT_EQ(result.w, 200);
  EXPECT_EQ(result.h, 100);
}

TEST(StyleContentRectTest, corner_radius_does_not_affect_content_rect)
{
  Pixils::UI::Style style;
  style.corner_radius = Pixils::UI::Style::CornerRadius(8);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);

  Pixils::Rect result = style.content_rect({0, 0, 100, 60});

  EXPECT_EQ(result.x, 6);
  EXPECT_EQ(result.y, 6);
  EXPECT_EQ(result.w, 88);
  EXPECT_EQ(result.h, 48);
}
