#include "pixils/ui/base_theme.h"

#include <pixils/binding/ui/style/theme_definition.h>

#include <lisple/context.h>
#include <lisple/runtime.h>
#include <optional>

namespace Pixils::UI
{
  namespace
  {
    /**
     * Keep the built-in base theme expressed as ordinary theme data so it can
     * grow into a real selector-based default stylesheet without C++ churn.
     */
    constexpr char DEFAULT_BASE_THEME_SOURCE[] = R"({
      :styles {
        :ui/panel {:background {:r 0xcc :g 0xcc :b 0xcc}
                   :padding 4}

        'ui/window {:border {:color {:r 0x00 :g 0x00 :b 0x00}
                                     :thickness 1}
                    :text {:color {:r 0 :g 0 :b 0}}}

        'ui/window-title-bar {:background {:r 0xff :g 0xff :b 0xff}
                              :text {:color {:r 0x88 :g 0x88 :b 0x88}}
                              :border {:bottom {:color {:r 0x00 :g 0x00 :b 0x00}
                                                :thickness 1}}
                              :width :fill
                              :height 19
                              :padding 0
                              :layout {:direction :row
                                       :gap :space-between}}

        ['ui/window:focus-within 'ui/window-title-bar]
        {:text {:color {:r 0 :g 0 :b 0}}}

        'ui/window-control-button {:hidden true
                                   :width 18
                                   :height 18}

        'ui/window-minimize-button {:hidden true
                                    :width 18
                                    :height 18}

        'ui/button-inner
        {:text {:color {:r 0x00 :g 0x00 :b 0x00}}
         :background {:r 0xff :g 0xff :b 0xff}
         :border {:color {:r 0x00 :g 0x00 :b 0x00}
                 :thickness 1}}

        'ui/button-inner:hover
        {:text {:color {:r 0xff :g 0xff :b 0xff}}
         :background {:r 0x00 :g 0x00 :b 0x00}
         :border {:color {:r 0x00 :g 0x00 :b 0x00}
                 :thickness 1}}


        'ui/text-input {:width 80
                        :border {:color {:r 0x00 :g 0x00 :b 0x00}
                        :thickness 1}}

        'ui/text-input-inner {:width :fill
                              :padding [2 4]}

        'ui/menu-bar {:background {:r 0xff :g 0xff :b 0xff}}

        'ui/popup-menu-item
        {:padding [2 16]}

        'ui/popup-menu-option-item
        {:padding [2 2]}

        :menu/shortcut-column
        {:margin {:l 16}}

        :menu/item
        {:text {:color {:r 0x00 :g 0x00 :b 0x00}
                :marked-style {:enabled true
                               :marker "@"
                               :font-styles :underline}}}

        :menu/item:hover
        {:text {:color {:r 0xff :g 0xff :b 0xff}}
         :background {:r 0x00 :g 0x00 :b 0x00}}

        'ui/menu-option-item {:padding {:t 2 :r 12 :b 2 :l 2}}

        'ui/menu-option-indicator {:margin {:l 2 :r 4}
                                :box-sizing :content-box
                                :width 8
                                :height 10}

        '(ui/menu-option-indicator {:selected true})
        {:background {:image :pixils/console-font}}

        'ui/menu-separator
        {:height 1
         :margin {:t 3 :r 0 :b 2 :l 0}
         :border {:thickness 1
                  :line-style :solid
                  :color {:r 0x00 :g 0x00 :b 0x00}
                  :right {:thickness 0}
                  :bottom {:thickness 0}
                  :left {:thickness 0}}}


        'ui/popup-menu-inner
        {:text {:color {:r 0x00 :g 0x00 :b 0x00}}
         :background {:r 0xff :g 0xff :b 0xff}
         :border {:color {:r 0x00 :g 0x00 :b 0x00}
                 :thickness 1}}
      }
    })";

    /**
     * Keep the built-in base theme expressed as ordinary theme data so it can
     * grow into a real selector-based default stylesheet without C++ churn.
     */
    [[maybe_unused]]
    constexpr char DEFAULT_CLASSIC_BLUE_THEME_SOURCE[] = R"({
      :styles {
        :ui/panel {:background {:r 0x20 :g 0x29 :g 0x36}
                   :padding 4}

        'window-title-bar {:background {:r 0x34 :g 0x42 :b 0x55}
                           :border {:top {:color {:r 0x58 :g 0x84 :b 0xc8}
                                          :thickness 1}
                                    :left {:color {:r 0x58 :g 0x84 :b 0xc8}
                                           :thickness 1}}
                           :width :fill
                           :height 19
                           :padding 0
                           :layout {:direction :row
                                    :gap :space-between}}

        ['window:focus-within 'window-title-bar]
        {:background {:r 0x26 :g 0x48 :b 0x8a}}

        'window-control-button {:width 18
                                :height 18}

        'window-minimize-button {:width 18
                                 :height 18}

        'text-input {:width 80}

        'text-input-inner {:width :fill
                           :padding [2 4]}

        'menu-item {:padding [2 16]}

        'menu-option-item {:padding {:t 2 :r 12 :b 2 :l 2}}

        'menu-option-indicator {:margin {:l 2 :r 4}
                                :box-sizing :content-box
                                :width 8
                                :height 10}
      }
    })";

  } // namespace

  const Theme& default_base_theme(Lisple::Runtime& runtime)
  {
    static std::optional<Theme> cached_theme = std::nullopt;

    if (!cached_theme)
    {
      Lisple::Context ctx(runtime);
      auto definition = ctx.eval(DEFAULT_BASE_THEME_SOURCE);
      cached_theme =
        Script::build_theme_from_definition(ctx, "pixils/base-theme", definition);
    }

    return *cached_theme;
  }
} // namespace Pixils::UI
