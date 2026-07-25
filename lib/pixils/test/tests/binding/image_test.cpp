#include "../fixture.h"
#include "../render_fixture.h"

#include <gtest/gtest.h>
#include <sdl3_mock/mock_resources.h>

class ImageTest : public BaseFixture
{
 protected:
  void TearDown() override { SDL3Mock::reset_mocks(); }
};

class GeneratedImageTest : public RenderFixture
{
};

TEST_F(ImageTest, image_metadata_functions_load_declared_images_on_demand)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
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

TEST_F(ImageTest, loaded_images_use_nearest_scale_mode)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval("(pixils/defbundle sprites {:images {:ship \"ship.png\"}})");

  // When
  SDL_Texture* texture = render_ctx.asset_registry->get_image("sprites", "ship");

  // Then
  ASSERT_NE(texture, nullptr);
  EXPECT_EQ(texture->scale_mode, SDL_SCALEMODE_NEAREST);
}

TEST_F(ImageTest, image_dependencies_accept_map_with_transparency_color)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
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

TEST_F(ImageTest, transparency_color_is_baked_into_source_alpha)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites
      {:images {:ship {:file-name "ship.png"
                       :transparency-color "#000000"}}})
  )");

  // When
  auto alpha = runtime.eval("(:a (pixils.image/color-at :sprites/ship {:x 0 :y 0}))");

  // Then
  ASSERT_NE(alpha, nullptr);
  EXPECT_EQ(alpha->num().get_int(), 0);
}

TEST_F(ImageTest, resource_dependency_adapter_exposes_images_as_a_map)
{
  // When
  auto file_name = runtime.eval(R"(
    (:file-name
      (:ship
        (:images
          (pixils.resource/make-resource-dependencies
            {:images {:ship {:file-name "ship.png"
                             :transparency-color "#5a5268"}}}))))
  )");
  auto red = runtime.eval(R"(
    (:r
      (:transparency-color
        (:ship
          (:images
            (pixils.resource/make-resource-dependencies
              {:images {:ship {:file-name "ship.png"
                               :transparency-color "#5a5268"}}})))))
  )");

  // Then
  ASSERT_NE(file_name, nullptr);
  ASSERT_NE(red, nullptr);
  EXPECT_EQ(file_name->str(), "ship.png");
  EXPECT_EQ(red->num().get_int(), 0x5a);
}

TEST_F(ImageTest, image_rect_uses_optional_point_offset_for_position)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
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

TEST_F(ImageTest, trace_polygons_is_exposed_in_image_namespace)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval("(pixils/defbundle sprites {:images {:ship \"ship.png\"}})");

  // When
  auto count = runtime.eval("(count (pixils.image/trace-polygons :sprites/ship))");

  // Then
  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 0);
}

TEST_F(ImageTest, trace_polygons_accepts_omit_straight_edges_option)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval("(pixils/defbundle sprites {:images {:ship \"ship.png\"}})");

  auto count = runtime.eval("(count (pixils.image/trace-polygons :sprites/ship "
                            "{:omit-straight-edges [:north :east]}))");

  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 0);
}

TEST_F(ImageTest, dynamic_bundle_images_can_be_added_before_first_lookup)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {32, 12};
  runtime.eval("(pixils/defbundle-dynamic project-assets)");

  // When
  auto resource =
    runtime.eval("(pixils.resource/add-image! :project-assets/ship \"ship.png\")");
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
  SDL3Mock::prepared_surfaces["./ship.png"] = {24, 10};
  runtime.eval("(pixils/defbundle-dynamic project-assets)");
  auto missing = runtime.eval("(pixils.image/width :project-assets/ship)");
  EXPECT_EQ(missing->type, Roo::Value::Type::NIL);

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

TEST_F(ImageTest, dynamic_bundle_images_can_be_listed)
{
  // Given
  runtime.eval("(pixils/defbundle-dynamic project-assets)");

  // When
  runtime.eval("(pixils.resource/add-image! :project-assets/ship \"ship.png\")");
  auto id = runtime.eval("(:id (head (pixils.resource/list-images :project-assets)))");
  auto file_name =
    runtime.eval("(:file-name (head (pixils.resource/list-images :project-assets)))");

  // Then
  ASSERT_NE(id, nullptr);
  ASSERT_NE(file_name, nullptr);
  EXPECT_EQ(id->to_string(), ":project-assets/ship");
  EXPECT_EQ(file_name->str(), "ship.png");
}

TEST_F(ImageTest, dynamic_bundle_can_be_created_at_runtime)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {20, 14};

  // When
  auto bundle = runtime.eval("(pixils.resource/create-bundle! :project-assets)");
  runtime.eval("(pixils.resource/add-image! :project-assets/ship \"ship.png\")");
  auto width = runtime.eval("(pixils.image/width :project-assets/ship)");

  // Then
  ASSERT_NE(bundle, nullptr);
  EXPECT_EQ(bundle->to_string(), ":project-assets");
  ASSERT_NE(width, nullptr);
  EXPECT_EQ(width->num().get_int(), 20);
}

TEST_F(ImageTest, dynamic_bundle_can_be_created_with_images)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {18, 9};

  // When
  runtime.eval(
    "(pixils.resource/create-bundle! :project-assets {:images {:ship \"ship.png\"}})");
  auto id = runtime.eval("(:id (head (pixils.resource/list-images :project-assets)))");
  auto width = runtime.eval("(pixils.image/width :project-assets/ship)");

  // Then
  ASSERT_NE(id, nullptr);
  ASSERT_NE(width, nullptr);
  EXPECT_EQ(id->to_string(), ":project-assets/ship");
  EXPECT_EQ(width->num().get_int(), 18);
}

TEST_F(ImageTest, dynamic_bundle_images_can_be_removed)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {24, 10};
  runtime.eval("(pixils/defbundle-dynamic project-assets)");
  runtime.eval("(pixils.resource/add-image! :project-assets/ship \"ship.png\")");
  auto width = runtime.eval("(pixils.image/width :project-assets/ship)");
  ASSERT_EQ(width->num().get_int(), 24);

  // When
  auto resource = runtime.eval("(pixils.resource/remove-image! :project-assets/ship)");
  auto missing_width = runtime.eval("(pixils.image/width :project-assets/ship)");
  auto first = runtime.eval("(head (pixils.resource/list-images :project-assets))");

  // Then
  ASSERT_NE(resource, nullptr);
  EXPECT_EQ(resource->to_string(), ":project-assets/ship");
  EXPECT_EQ(missing_width->type, Roo::Value::Type::NIL);
  EXPECT_EQ(first->type, Roo::Value::Type::NIL);
}

TEST_F(ImageTest, add_image_requires_dynamic_bundle)
{
  // Given
  runtime.eval("(pixils/defbundle static-assets {:images {}})");

  // Then
  EXPECT_THROW(runtime.eval("(pixils.resource/add-image! :static-assets/ship \"ship.png\")"),
               std::runtime_error);
}

TEST_F(GeneratedImageTest, dynamic_bundle_images_can_be_created_from_lisp_drawing)
{
  // Given
  runtime.eval("(pixils/defbundle-dynamic project-assets)");

  // When
  auto resource = runtime.eval(R"(
    (pixils.resource/create-image!
      :project-assets/brush
      {:size {:w 12 :h 7}}
      (fn []
        (pixils.render/rect!
          {:x 1 :y 2 :w 3 :h 4}
          {:fill true
           :color {:r 255 :g 0 :b 0}})))
  )");
  auto width = runtime.eval("(pixils.image/width :project-assets/brush)");
  auto height = runtime.eval("(pixils.image/height :project-assets/brush)");
  runtime.eval("(pixils.render/image! :project-assets/brush {:pos {:x 5 :y 6}})");

  // Then
  ASSERT_NE(resource, nullptr);
  EXPECT_EQ(resource->to_string(), ":project-assets/brush");
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  EXPECT_EQ(width->num().get_int(), 12);
  EXPECT_EQ(height->num().get_int(), 7);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 5);
  EXPECT_EQ(ops[0].rendered_rect.y, 6);
  EXPECT_EQ(ops[0].rendered_rect.w, 12);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
  ASSERT_EQ(ops[0].sub_ops.size(), 1u);
  EXPECT_EQ(ops[0].sub_ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.y, 2);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.w, 3);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.h, 4);
}

TEST_F(GeneratedImageTest, generated_images_use_nearest_scale_mode)
{
  // Given
  runtime.eval("(pixils/defbundle-dynamic project-assets)");

  // When
  runtime.eval(R"(
    (pixils.resource/create-image!
      :project-assets/brush
      {:size {:w 8 :h 8}}
      (fn [] nil))
  )");

  // Then
  SDL_Texture* texture = render_ctx.asset_registry->get_image("project-assets", "brush");
  ASSERT_NE(texture, nullptr);
  EXPECT_EQ(texture->scale_mode, SDL_SCALEMODE_NEAREST);
}

TEST_F(GeneratedImageTest, generated_dynamic_bundle_images_can_be_listed)
{
  // Given
  runtime.eval("(pixils/defbundle-dynamic project-assets)");

  // When
  runtime.eval(R"(
    (pixils.resource/create-image!
      :project-assets/brush
      {:size {:w 9 :h 11}}
      (fn [] nil))
  )");
  auto id = runtime.eval("(:id (head (pixils.resource/list-images :project-assets)))");
  auto source =
    runtime.eval("(:source (head (pixils.resource/list-images :project-assets)))");
  auto width =
    runtime.eval("(:w (:size (head (pixils.resource/list-images :project-assets))))");
  auto height =
    runtime.eval("(:h (:size (head (pixils.resource/list-images :project-assets))))");

  // Then
  ASSERT_NE(id, nullptr);
  ASSERT_NE(source, nullptr);
  ASSERT_NE(width, nullptr);
  ASSERT_NE(height, nullptr);
  EXPECT_EQ(id->to_string(), ":project-assets/brush");
  EXPECT_EQ(source->to_string(), ":generated");
  EXPECT_EQ(width->num().get_int(), 9);
  EXPECT_EQ(height->num().get_int(), 11);
}

TEST_F(GeneratedImageTest, generated_dynamic_bundle_images_can_be_removed)
{
  // Given
  runtime.eval("(pixils/defbundle-dynamic project-assets)");
  runtime.eval(R"(
    (pixils.resource/create-image!
      :project-assets/brush
      {:size {:w 8 :h 8}}
      (fn [] nil))
  )");
  auto width = runtime.eval("(pixils.image/width :project-assets/brush)");
  ASSERT_EQ(width->num().get_int(), 8);

  // When
  auto resource = runtime.eval("(pixils.resource/remove-image! :project-assets/brush)");
  auto missing_width = runtime.eval("(pixils.image/width :project-assets/brush)");
  auto first = runtime.eval("(head (pixils.resource/list-images :project-assets))");

  // Then
  ASSERT_NE(resource, nullptr);
  EXPECT_EQ(resource->to_string(), ":project-assets/brush");
  EXPECT_EQ(missing_width->type, Roo::Value::Type::NIL);
  EXPECT_EQ(first->type, Roo::Value::Type::NIL);
}

TEST_F(GeneratedImageTest, create_image_restores_existing_render_target)
{
  // Given
  runtime.eval("(pixils/defbundle-dynamic project-assets)");
  SDL_Texture* previous_target = SDL_CreateTexture(render_ctx.renderer,
                                                   SDL_PIXELFORMAT_RGBA8888,
                                                   SDL_TEXTUREACCESS_TARGET,
                                                   20,
                                                   20);
  render_ctx.set_render_target(previous_target);

  // When
  runtime.eval(R"(
    (pixils.resource/create-image!
      :project-assets/brush
      {:size {:w 4 :h 4}}
      (fn [] nil))
  )");

  // Then
  EXPECT_EQ(render_target(), previous_target);
  EXPECT_EQ(render_ctx.current_render_target, previous_target);
}

TEST_F(GeneratedImageTest, create_image_requires_dynamic_bundle)
{
  // Given
  runtime.eval("(pixils/defbundle static-assets {:images {}})");

  // Then
  EXPECT_THROW(runtime.eval(R"(
    (pixils.resource/create-image!
      :static-assets/brush
      {:size {:w 4 :h 4}}
      (fn [] nil))
  )"),
               std::runtime_error);
}
