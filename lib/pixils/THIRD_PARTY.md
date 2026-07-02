# Third-party code

Pixils vendors a small amount of third-party source code where doing so keeps
the Pixils build self-contained and avoids a system-level runtime dependency.

## Clipper2

- Upstream: https://github.com/AngusJohnson/Clipper2
- Vendored version: `Clipper2_2.0.1`
- Local path: `lib/pixils/third_party/clipper2`
- License: Boost Software License 1.0
- License text: `lib/pixils/third_party/clipper2/LICENSE`

Pixils builds the vendored Clipper2 C++ sources into its own native artifacts.
Pixils does not require Clipper2 to be installed on the host system and does
not dynamically link against a system `libclipper2`.

Clipper2 is used internally to implement polygon boolean operations exposed
through `pixils.polygon`, such as `intersection` and `union`.
