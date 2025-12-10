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

enum LogicType {
  active_high,
  active_low,
};

struct SPIBus {
  gpio_num_t clk = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t mosi = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t miso = gpio_num_t::GPIO_NUM_NC;
};

struct TouchPanelConfig {
  enum {
    XPT2046,
    STMPE610,
  } type;
  bool enabled = false;
  gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t irq = gpio_num_t::GPIO_NUM_NC;
  std::optional<SPIBus> spi;
};

struct FullDisplayPanelConfig {
  enum DisplayType {
    ILI9341,
    ST7789,
  } type;
  bool secondary_spi = false;
  gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t backlight = gpio_num_t::GPIO_NUM_NC;
  LogicType backlight_logic = LogicType::active_high;
  uint16_t width = 320, height = 240;
  SPIBus spi;
  bool mirror_x = true, mirror_y = false;

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
} // namespace teslasynth::app::configuration::hardware
