#ifndef PIXILS__RESOURCE_NAMESPACE_H
#define PIXILS__RESOURCE_NAMESPACE_H

#include <pixils/runtime/mode.h>

#include <roo/exec.h>
#include <roo/namespace.h>

namespace Pixils::Script
{
  inline const std::string_view NS__PIXILS__RESOURCE = "pixils.resource";

  inline constexpr std::string_view FN__PIXILS__MAKE_RESOURCE_DEPS =
    "pixils.resource/make-resource-dependencies";
  inline const std::string FN__MAKE_RESOURCE_DEPS = "make-resource-dependencies";
  inline constexpr std::string_view FN__CREATE_BUNDLE_BANG = "create-bundle!";
  inline constexpr std::string_view FN__ADD_IMAGE_BANG = "add-image!";
  inline constexpr std::string_view FN__CREATE_IMAGE_BANG = "create-image!";
  inline constexpr std::string_view FN__REDRAW_IMAGE_BANG = "redraw-image!";
  inline constexpr std::string_view FN__REMOVE_IMAGE_BANG = "remove-image!";
  inline constexpr std::string_view FN__LIST_IMAGES = "list-images";
  inline constexpr std::string_view FN__CAN_CREATE_IMAGES = "can-create-images?";

  namespace HostType
  {
    HOST_TYPE(RESOURCE_DEPENDENCIES,
              "HResourceDependencies",
              std::string(FN__PIXILS__MAKE_RESOURCE_DEPS));
  }

  namespace Function
  {
    /*!
     * @brief Convert a resource declaration map to resource dependencies.
     * @since 0.1.0
     * @see pixils/defbundle
     * @see pixils/defbundle-dynamic
     * @see pixils.resource/create-bundle!
     *
     * The declaration may contain `:images`, `:music`, `:sounds`, and `:fonts`
     * maps. Image values are either file-name strings or maps containing the
     * required `:file-name` and optional `:transparency-color`. Music, sound,
     * and font values are file-name strings.
     *
     * The returned native value exposes the normalized dependency maps through
     * its `:images`, `:music`, `:sounds`, and `:fonts` properties. In the
     * normalized `:images` map every value is a map containing `:file-name` and,
     * when declared, `:transparency-color`.
     *
     * Usage:
     * @code
     * (pixils.resource/make-resource-dependencies
     *   {:images {:ship "images/ship.png"
     *             :cursor {:file-name "images/cursor.png"
     *                      :transparency-color "#ff00ff"}}
     *    :sounds {:click "audio/click.wav"}
     *    :music {:theme "audio/theme.ogg"}
     *    :fonts {:body "fonts/body.ttf"}})
     * => #<HResourceDependencies>
     * @endcode
     *
     * | Arg         | Description                                             |
     * | ----------- | ------------------------------------------------------- |
     * | declaration | Map describing image, music, sound, and font resources. |
     *
     * @return A normalized resource-dependencies native value.
     */
    FUNC(MakeResourceDependencies, make_deps);

    /*!
     * @brief Create or update a dynamic resource bundle at runtime.
     * @since 0.1.0
     * @see pixils/defbundle-dynamic
     * @see pixils.resource/add-image!
     * @see pixils.resource/create-image!
     *
     * With no declaration, creates an empty dynamic bundle. The optional
     * declaration has the same shape accepted by
     * `make-resource-dependencies`. Calling the function for an existing
     * dynamic bundle adds or replaces its declared resources. A statically
     * declared bundle cannot be converted to a dynamic bundle.
     *
     * Usage:
     * @code
     * (pixils.resource/create-bundle! :project-assets)
     * => :project-assets
     *
     * (pixils.resource/create-bundle!
     *   :project-assets
     *   {:images {:ship "images/ship.png"}})
     * => :project-assets
     * @endcode
     *
     * | Arg         | Description                                              |
     * | ----------- | -------------------------------------------------------- |
     * | bundle      | Unqualified keyword identifying the dynamic bundle.      |
     * | declaration | Optional resource declaration map used to populate it.   |
     *
     * @return The supplied bundle keyword.
     */
    FUNC(CreateBundleBang, create_bundle);

    /*!
     * @brief Add or replace a file-backed image in a dynamic bundle.
     * @since 0.1.0
     * @see pixils.resource/create-bundle!
     * @see pixils.resource/create-image!
     * @see pixils.resource/remove-image!
     *
     * The resource must be a qualified keyword whose qualifier names an
     * existing dynamic bundle. The image declaration is either a file-name
     * string or a map containing `:file-name` and optional
     * `:transparency-color`. Reusing an image identity replaces its existing
     * file-backed or generated image.
     *
     * Usage:
     * @code
     * (pixils.resource/add-image! :project-assets/ship "images/ship.png")
     * => :project-assets/ship
     *
     * (pixils.resource/add-image!
     *   :project-assets/cursor
     *   {:file-name "images/cursor.png"
     *    :transparency-color "#ff00ff"})
     * => :project-assets/cursor
     * @endcode
     *
     * | Arg      | Description                                                |
     * | -------- | ---------------------------------------------------------- |
     * | resource | Qualified keyword identifying the bundle and image.        |
     * | image    | File-name string or image declaration map.                 |
     *
     * @return The supplied resource keyword.
     */
    FUNC(AddImageBang, add_image);

    /*!
     * @brief Create a generated image by drawing into a render-target texture.
     * @since 0.1.0
     * @see pixils.resource/redraw-image!
     * @see pixils.resource/can-create-images?
     * @see pixils.render/image!
     *
     * The resource must belong to an existing dynamic bundle. The callback is
     * invoked without arguments while the new nearest-neighbor texture is the
     * current render target; render functions therefore draw into the image.
     * The previous render target is restored after the callback completes.
     *
     * `:size` is required and both dimensions must be positive. `:clear`
     * defaults to transparent black. `:readback?` defaults to `true`; set it to
     * `false` for a render-only buffer that does not need CPU-side pixel access.
     * Creating an image with an existing identity replaces that image.
     *
     * Usage:
     * @code
     * (pixils.resource/create-image!
     *   :project-assets/brush
     *   {:size {:w 16 :h 16}
     *    :clear "#00000000"
     *    :readback? true}
     *   (fn []
     *     (pixils.render/rect! {:x 2 :y 2 :w 12 :h 12}
     *                          {:fill true :color "#ffffff"})))
     * => :project-assets/brush
     * @endcode
     *
     * | Arg      | Description                                                |
     * | -------- | ---------------------------------------------------------- |
     * | resource | Qualified keyword identifying the bundle and image.        |
     * | options  | Map with required `:size` and optional `:clear` and         |
     * |          | `:readback?` values.                                       |
     * | draw     | Zero-argument function that draws the image contents.      |
     *
     * @return The supplied resource keyword.
     */
    FUNC(CreateImageBang, create_image);

    /*!
     * @brief Redraw an existing generated image in place.
     * @since 0.1.0
     * @see pixils.resource/create-image!
     * @see pixils.resource/can-create-images?
     *
     * The resource must identify a generated image in a dynamic bundle. Its
     * required `:size` must match the size used when the image was created. The
     * existing texture is reused, so references to it remain valid.
     *
     * The callback is invoked without arguments while the image texture is the
     * current render target. `:clear` defaults to transparent black. Omitting
     * `:readback?` preserves the image's current readback policy; supplying it
     * enables or disables CPU-side pixel access after this redraw. The previous
     * render target is restored after the callback completes.
     *
     * Usage:
     * @code
     * (pixils.resource/redraw-image!
     *   :project-assets/brush
     *   {:size {:w 16 :h 16}}
     *   (fn []
     *     (pixils.render/rect! {:x 0 :y 0 :w 16 :h 16}
     *                          {:fill true :color "#336699"})))
     * => :project-assets/brush
     * @endcode
     *
     * | Arg      | Description                                                |
     * | -------- | ---------------------------------------------------------- |
     * | resource | Qualified keyword identifying an existing generated image. |
     * | options  | Map with required `:size` and optional `:clear` and        |
     * |          | `:readback?` values.                                       |
     * | draw     | Zero-argument function that redraws the image contents.    |
     *
     * @return The supplied resource keyword.
     */
    FUNC(RedrawImageBang, redraw_image);

    /*!
     * @brief Remove an image from a dynamic resource bundle.
     * @since 0.1.0
     * @see pixils.resource/add-image!
     * @see pixils.resource/create-image!
     * @see pixils.resource/list-images
     *
     * Removes either a file-backed or generated image. Removing an identity
     * that is not present has no effect, but the bundle must exist and be
     * dynamic.
     *
     * Usage:
     * @code
     * (pixils.resource/remove-image! :project-assets/ship)
     * => :project-assets/ship
     * @endcode
     *
     * | Arg      | Description                                         |
     * | -------- | --------------------------------------------------- |
     * | resource | Qualified keyword identifying the image to remove.  |
     *
     * @return The supplied resource keyword.
     */
    FUNC(RemoveImageBang, remove_image);

    /*!
     * @brief Return the images registered in a resource bundle.
     * @since 0.1.0
     * @see pixils.resource/add-image!
     * @see pixils.resource/create-image!
     *
     * File-backed images are returned as maps with `:id` and `:file-name`.
     * Generated images are returned as maps with `:id`, `:source :generated`,
     * and `:size`. The bundle may be static or dynamic. The result is empty
     * when the bundle contains no images.
     *
     * Usage:
     * @code
     * (pixils.resource/list-images :project-assets)
     * => [{:id :project-assets/ship :file-name "images/ship.png"}
     *     {:id :project-assets/brush :source :generated :size {:w 16 :h 16}}]
     * @endcode
     *
     * | Arg    | Description                                         |
     * | ------ | --------------------------------------------------- |
     * | bundle | Unqualified keyword identifying an existing bundle. |
     *
     * @return A vector of image description maps.
     */
    FUNC(ListImages, list_images);

    /*!
     * @brief Test whether generated images can be created in this runtime.
     * @since 0.1.0
     * @see pixils.resource/create-image!
     * @see pixils.resource/redraw-image!
     *
     * Generated image creation requires both an asset registry and an active
     * SDL renderer. This predicate is useful when the same Roo code also runs
     * in a headless or non-rendering environment.
     *
     * Usage:
     * @code
     * (when (pixils.resource/can-create-images?)
     *   (create-render-resources!))
     * @endcode
     *
     * @return `true` when generated-image operations are available; otherwise
     * `false`.
     */
    FUNC(CanCreateImages, can_create_images);
  } // namespace Function

  NATIVE_ADAPTER(ResourceDependenciesAdapter,
                 Pixils::Runtime::ResourceDependencies,
                 (images, music, sounds, fonts));

  class ResourceNamespace : public Roo::Namespace
  {
   public:
    ResourceNamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__RESOURCE_NAMESPACE_H */
