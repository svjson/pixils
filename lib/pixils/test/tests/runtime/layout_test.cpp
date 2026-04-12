
#include "session_fixture.h"

#include <pixils/geom.h>
#include <pixils/runtime/mode.h>

#include <gtest/gtest.h>

using Pixils::Rect;
using Pixils::Runtime::ChildSlot;
using Pixils::Runtime::DimensionConstraint;

class LayoutTest : public SessionFixture {};

TEST_F(LayoutTest, layout_single_fill_child_takes_full_height)
{
  // Given
  std::vector<ChildSlot> slots = {{}};
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = session.layout_children(slots, parent);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 200);
}

TEST_F(LayoutTest, layout_fixed_then_fill_child_splits_height_correctly)
{
  // Given
  ChildSlot fixed_slot;
  fixed_slot.height = DimensionConstraint::fixed(40);

  std::vector<ChildSlot> slots = {fixed_slot, {}};
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = session.layout_children(slots, parent);

  // Then
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 40);
  EXPECT_EQ(rects[1].h, 160);
}

TEST_F(LayoutTest, layout_two_fill_children_split_height_evenly)
{
  // Given
  std::vector<ChildSlot> slots = {{}, {}};
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = session.layout_children(slots, parent);

  // Then
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 100);
  EXPECT_EQ(rects[1].y, 100);
  EXPECT_EQ(rects[1].h, 100);
}

TEST_F(LayoutTest, layout_children_always_inherit_full_parent_width)
{
  // Given
  ChildSlot fixed_slot;
  fixed_slot.height = DimensionConstraint::fixed(30);

  std::vector<ChildSlot> slots = {fixed_slot, {}};
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = session.layout_children(slots, parent);

  // Then
  for (const auto& r : rects)
  {
    EXPECT_EQ(r.w, 320);
    EXPECT_EQ(r.x, 0);
  }
}

TEST_F(LayoutTest, layout_children_respect_parent_origin)
{
  // Given
  std::vector<ChildSlot> slots = {{}};
  Rect parent = {10, 20, 100, 80};

  // When
  auto rects = session.layout_children(slots, parent);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 10);
  EXPECT_EQ(rects[0].y, 20);
  EXPECT_EQ(rects[0].w, 100);
  EXPECT_EQ(rects[0].h, 80);
}
