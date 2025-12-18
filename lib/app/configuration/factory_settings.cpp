#include "hardware.hpp"
#include "sdkconfig.h"
#include "soc/gpio_num.h"

#define GPIO_CONFIG(sym) static_cast<gpio_num_t>(CONFIG_TESLASYNTH_##sym)
namespace teslasynth::app::configuration::hardware {

#if defined(CONFIG_TESLASYNTH_GUI_FULL)

#ifdef CONFIG_TESLASYNTH_TOUCH_ENABLED
constexpr static TouchPanelConfig default_touch_config{
    .enabled = true,
#if defined(CONFIG_TESLASYNTH_TOUCH_PANEL_XPT2046)
    .type = TouchPanelConfig::XPT2046,
#elif defined(CONFIG_TESLASYNTH_TOUCH_PANEL_STMPE610)
    .type = TouchPanelConfig::STMPE610,
#else
#error "No touch panel type selected"
#endif

    .cs = GPIO_CONFIG(TOUCH_INTERFACE_SPI_CS),
    .dc = GPIO_CONFIG(TOUCH_INTERFACE_SPI_DC),
    .rs = GPIO_CONFIG(TOUCH_INTERFACE_SPI_RS),
    .irq = GPIO_CONFIG(TOUCH_INTERFACE_SPI_IRQ),
    .flags =
        {
#ifdef CONFIG_TESLASYNTH_TOUCH_MIRROR_X
            .mirror_x = true,
#endif
#ifdef CONFIG_TESLASYNTH_TOUCH_MIRROR_Y
            .mirror_y = true,
#endif
#ifdef CONFIG_TESLASYNTH_TOUCH_SWAP_XY
            .swap_xy = true,
#endif
        },

#ifdef CONFIG_TESLASYNTH_TOUCH_SECONDARY_SPI
    .spi = {{
        .clk = GPIO_CONFIG(TOUCH_INTERFACE_SPI_CLK),
        .mosi = GPIO_CONFIG(TOUCH_INTERFACE_SPI_MOSI),
        .miso = GPIO_CONFIG(TOUCH_INTERFACE_SPI_MISO),
    }},
#else
    .spi = {},
#endif
};
#endif

constexpr static FullDisplayPanelConfig::FullDisplayType full_display_type =
#if defined(CONFIG_TESLASYNTH_DISPLAY_PANEL_ILI9341)
    FullDisplayPanelConfig::ILI9341;
#elif defined(CONFIG_TESLASYNTH_DISPLAY_PANEL_ST7789)
    FullDisplayPanelConfig::ST7789;
#else
#error "No display panel type selected"
#endif

constexpr static LogicType full_display_backlight_logic =
#ifdef CONFIG_TESLASYNTH_DISPLAY_BACKLIGHT_ACTIVE_HIGH
    active_high;
#else
    active_low;
#endif

constexpr static PanelFlags full_display_flags{
#ifdef CONFIG_TESLASYNTH_DISPLAY_MIRROR_X
    .mirror_x = true,
#endif
#ifdef CONFIG_TESLASYNTH_DISPLAY_MIRROR_Y
    .mirror_y = true,
#endif
#ifdef CONFIG_TESLASYNTH_DISPLAY_SWAP_XY
    .swap_xy = true,
#endif
};

constexpr static FullDisplayPanelConfig default_full_display_config{
    .type = full_display_type,
    .cs = GPIO_CONFIG(DISPLAY_INTERFACE_SPI_CS),
    .dc = GPIO_CONFIG(DISPLAY_INTERFACE_SPI_DC),
    .rs = GPIO_CONFIG(DISPLAY_INTERFACE_SPI_RS),

    .backlight = GPIO_CONFIG(DISPLAY_BACKLIGHT_PIN),
    .backlight_logic = full_display_backlight_logic,

#ifdef CONFIG_TESLASYNTH_DISPLAY_RESOLUTION_320x240
    .width = 320,
    .height = 240,
#endif

    .flags = full_display_flags,
    .spi =
        {
            .clk = GPIO_CONFIG(DISPLAY_INTERFACE_SPI_CLK),
            .mosi = GPIO_CONFIG(DISPLAY_INTERFACE_SPI_MOSI),
            .miso = GPIO_CONFIG(DISPLAY_INTERFACE_SPI_MISO),
        },

#ifdef CONFIG_TESLASYNTH_TOUCH_ENABLED
    .touch = default_touch_config
#endif
};

#endif

#if defined(CONFIG_TESLASYNTH_GUI_STATUS_PANEL)
constexpr static MinimalDisplayPanelConfig default_minimal_display_config{
    .type = MinimalDisplayPanelConfig::SSD1306,
    .sda = GPIO_CONFIG(DISPLAY_INTERFACE_I2C_SDA),
    .scl = GPIO_CONFIG(DISPLAY_INTERFACE_I2C_SCL),
    .rs = GPIO_CONFIG(DISPLAY_INTERFACE_I2C_RS),
    .width = 128,
    .height = 64,
};
#endif

constexpr DisplayType gui_type =
#if defined(CONFIG_TESLASYNTH_GUI_STATUS_PANEL)
    minimal;
#elif defined(CONFIG_TESLASYNTH_GUI_FULL)
    full;
#else
    none;
#endif

constexpr static DisplayConfig::PanelConfig default_panel_config{
#if defined(CONFIG_TESLASYNTH_GUI_STATUS_PANEL)
    .minimal = default_minimal_display_config},
#elif defined(CONFIG_TESLASYNTH_GUI_FULL)
    .full = default_full_display_config,
#endif
};

constexpr static DisplayConfig default_display{
    .type = gui_type,
    .config = default_panel_config,
};

constexpr static OutputConfig default_outputs{};

HardwareConfig::HardwareConfig()
    : display(default_display), outputs(default_outputs) {}

} // namespace teslasynth::app::configuration::hardware
