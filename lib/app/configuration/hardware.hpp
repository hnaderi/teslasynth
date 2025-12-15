#pragma once

#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <array>
#include <cstdint>
#include <optional>

namespace teslasynth::app::configuration::hardware {
struct OutputChannelConfig {
  gpio_num_t pin = gpio_num_t::GPIO_NUM_NC;
};

enum LogicType : bool {
  active_high = true,
  active_low = false,
};

struct SPIBus {
  gpio_num_t clk = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t mosi = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t miso = gpio_num_t::GPIO_NUM_NC;
};

struct TouchPanelConfig {
  bool enabled = false;
  enum TouchType : uint8_t {
    XPT2046 = 0,
    STMPE610 = 1,
  } type;
  gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t irq = gpio_num_t::GPIO_NUM_NC;
  std::optional<SPIBus> spi;
};

struct FullDisplayPanelConfig {
  enum DisplayType : uint8_t {
    ILI9341 = 0,
    ST7789 = 1,
  } type;
  gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t backlight = gpio_num_t::GPIO_NUM_NC;
  LogicType backlight_logic = LogicType::active_high;
  uint16_t width = 320, height = 240;
  bool mirror_x = true, mirror_y = false;
  SPIBus spi;

  TouchPanelConfig touch;
};

struct MinimalDisplayPanelConfig {
  enum DisplayType {
    SSD1306,
  } type = SSD1306;
  gpio_num_t sda = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t scl = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  uint8_t width = 128, height = 64;
};

enum DisplayType {
  none,
  minimal,
  full,
};

struct NoDisplay {};
struct DisplayConfig {
  DisplayType type = none;
  union {
    NoDisplay none;
    MinimalDisplayPanelConfig minimal;
    FullDisplayPanelConfig full;
  } config = {.none = NoDisplay{}};

  constexpr bool enabled() const { return type != none; }
  constexpr bool is_minimal() const { return type != minimal; }
  constexpr bool is_full() const { return type != full; }
};

struct OutputConfig {
  constexpr static uint8_t size = CONFIG_TESLASYNTH_OUTPUT_COUNT;
  std::array<OutputChannelConfig, size> channels;
};

struct HardwareConfig {
  uint32_t version = 0;
  DisplayConfig display;
  OutputConfig outputs{};
};

#if CONFIG_IDF_TARGET_ESP32

const HardwareConfig CYD = {
    .display =
        {
            .type = DisplayType::full,
            .config =
                {
                    .full =
                        {
                            .type = FullDisplayPanelConfig::ILI9341,
                            .cs = gpio_num_t::GPIO_NUM_15,
                            .dc = gpio_num_t::GPIO_NUM_2,
                            .rs = gpio_num_t::GPIO_NUM_4,
                            .backlight = gpio_num_t::GPIO_NUM_21,
                            .backlight_logic = active_high,
                            .width = 320,
                            .height = 240,
                            .mirror_x = true,
                            .mirror_y = false,
                            .spi =
                                {
                                    .clk = gpio_num_t::GPIO_NUM_14,
                                    .mosi = gpio_num_t::GPIO_NUM_13,
                                    .miso = gpio_num_t::GPIO_NUM_12,
                                },
                            .touch{
                                .enabled = true,
                                .type = TouchPanelConfig::XPT2046,
                                .cs = gpio_num_t::GPIO_NUM_33,
                                .irq = gpio_num_t::GPIO_NUM_36,
                                .spi = {{
                                    .clk = gpio_num_t::GPIO_NUM_25,
                                    .mosi = gpio_num_t::GPIO_NUM_32,
                                    .miso = gpio_num_t::GPIO_NUM_39,
                                }},
                            },

                        } // namespace teslasynth::app::configuration::hardware
                },
        },
};

const HardwareConfig lilygo_display = {
    .display =
        {
            .type = minimal,
            .config =
                {
                    .minimal =
                        {
                            .sda = gpio_num_t::GPIO_NUM_21,
                            .scl = gpio_num_t::GPIO_NUM_22,
                        },
                },
        },
};

#endif

} // namespace teslasynth::app::configuration::hardware
