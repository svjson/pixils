#include "../fixture.h"
#include <pixils/color.h>

#include <gtest/gtest.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>

using ColorTest = BaseFixture;

TEST_F(ColorTest, make_color_from_map)
{
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.color/make-color {:r 12 :g 34 :b 56 :a 78})");

  auto color = Lisple::obj<Pixils::Color>(*result);
  EXPECT_EQ(color, (Pixils::Color{12, 34, 56, 78}));
}

TEST_F(ColorTest, make_color_from_hex_string)
{
  Lisple::sptr_rtval result = runtime.eval("(pixils.color/make-color \"#FaC988\")");

  auto color = Lisple::obj<Pixils::Color>(*result);
  EXPECT_EQ(color, (Pixils::Color{0xFA, 0xC9, 0x88, 0xFF}));
}

TEST_F(ColorTest, make_color_from_short_hex_string)
{
  Lisple::sptr_rtval result = runtime.eval("(pixils.color/make-color \"#AbC\")");

  auto color = Lisple::obj<Pixils::Color>(*result);
  EXPECT_EQ(color, (Pixils::Color{0xAA, 0xBB, 0xCC, 0xFF}));
}

TEST_F(ColorTest, exposes_named_colors)
{
  Lisple::sptr_rtval result = runtime.eval("pixils.color/REBECCA-PURPLE");

  auto color = Lisple::obj<Pixils::Color>(*result);
  EXPECT_EQ(color, (Pixils::Color{0x66, 0x33, 0x99, 0xFF}));
}

TEST_F(ColorTest, exposes_transparent)
{
  Lisple::sptr_rtval result = runtime.eval("pixils.color/TRANSPARENT");

  auto color = Lisple::obj<Pixils::Color>(*result);
  EXPECT_EQ(color, (Pixils::Color{0x00, 0x00, 0x00, 0x00}));
}
