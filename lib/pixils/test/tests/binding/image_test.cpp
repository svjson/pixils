#include "../fixture.h"

#include <gtest/gtest.h>
#include <sdl2_mock/mock_resources.h>

class ImageTest : public BaseFixture
{
 protected:
  void TearDown() override { SDLMock::reset_mocks(); }
};

TEST_F(ImageTest, image_metadata_functions_load_declared_images_on_demand)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval("(pixils/defbundle sprites {:images {:ship \"ship.png\"}})");

  // When
  auto width = runtime.eval("(pixils.image/width :sprites/ship)");
  auto height = runtime.eval("(pixils.image/height :sprites/ship)");
  auto size_w = runtime.eval("(:w (pixils.image/size :sprites/ship))");
  auto size_h = runtime.eval("(:h (pixils.image/size :sprites/ship))");

  // Then
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  ASSERT_NE(size_w, nullptr);
  ASSERT_NE(size_h, nullptr);
  EXPECT_EQ(width->num().get_int(), 16);
  EXPECT_EQ(height->num().get_int(), 8);
  EXPECT_EQ(size_w->num().get_int(), 16);
  EXPECT_EQ(size_h->num().get_int(), 8);
}

TEST_F(ImageTest, image_dependencies_accept_map_with_transparency_color)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites
      {:images {:ship {:file-name "ship.png"
                       :transparency-color "#5a5268"}}})
  )");

  // When
  auto width = runtime.eval("(pixils.image/width :sprites/ship)");
  auto height = runtime.eval("(pixils.image/height :sprites/ship)");

  // Then
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  EXPECT_EQ(width->num().get_int(), 16);
  EXPECT_EQ(height->num().get_int(), 8);
}

TEST_F(ImageTest, image_rect_uses_optional_point_offset_for_position)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval("(pixils/defbundle sprites {:images {:ship \"ship.png\"}})");

  // When
  auto x = runtime.eval("(:x (pixils.image/rect :sprites/ship {:x 5 :y 7}))");
  auto y = runtime.eval("(:y (pixils.image/rect :sprites/ship {:x 5 :y 7}))");
  auto w = runtime.eval("(:w (pixils.image/rect :sprites/ship {:x 5 :y 7}))");
  auto h = runtime.eval("(:h (pixils.image/rect :sprites/ship {:x 5 :y 7}))");

  // Then
  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_NE(w, nullptr);
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(x->num().get_int(), 5);
  EXPECT_EQ(y->num().get_int(), 7);
  EXPECT_EQ(w->num().get_int(), 16);
  EXPECT_EQ(h->num().get_int(), 8);
}

TEST_F(ImageTest, dynamic_bundle_images_can_be_added_before_first_lookup)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {32, 12};
  runtime.eval("(pixils/defbundle-dynamic project-assets)");

  // When
  auto resource = runtime.eval(
    "(pixils.resource/add-image! :project-assets/ship \"ship.png\")");
  auto width = runtime.eval("(pixils.image/width :project-assets/ship)");
  auto height = runtime.eval("(pixils.image/height :project-assets/ship)");

  // Then
  ASSERT_NE(resource, nullptr);
  EXPECT_EQ(resource->to_string(), ":project-assets/ship");
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  EXPECT_EQ(width->num().get_int(), 32);
  EXPECT_EQ(height->num().get_int(), 12);
}

TEST_F(ImageTest, dynamic_bundle_images_can_be_added_after_bundle_is_loaded)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {24, 10};
  runtime.eval("(pixils/defbundle-dynamic project-assets)");
  auto missing = runtime.eval("(pixils.image/width :project-assets/ship)");
  EXPECT_EQ(missing->type, Lisple::Value::Type::NIL);

  // When
  runtime.eval("(pixils.resource/add-image! :project-assets/ship \"ship.png\")");
  auto width = runtime.eval("(pixils.image/width :project-assets/ship)");
  auto height = runtime.eval("(pixils.image/height :project-assets/ship)");

  // Then
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  EXPECT_EQ(width->num().get_int(), 24);
  EXPECT_EQ(height->num().get_int(), 10);
}

TEST_F(ImageTest, add_image_requires_dynamic_bundle)
{
  // Given
  runtime.eval("(pixils/defbundle static-assets {:images {}})");

  // Then
  EXPECT_THROW(runtime.eval(
                 "(pixils.resource/add-image! :static-assets/ship \"ship.png\")"),
               std::runtime_error);
}
