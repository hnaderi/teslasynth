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

enum class LogicType : bool {
  active_high = true,
  active_low = false,
};

struct SPIBus {
  gpio_num_t clk = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t mosi = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t miso = gpio_num_t::GPIO_NUM_NC;
};

struct PanelFlags {
  bool mirror_x = false, mirror_y = false, swap_xy = false;
};

struct TouchPanelConfig {
  bool enabled = false;
  enum class TouchType : uint8_t {
    XPT2046 = 0,
    STMPE610 = 1,
  } type;

  gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t irq = gpio_num_t::GPIO_NUM_NC;
  PanelFlags flags;
  std::optional<SPIBus> spi;
};

struct FullDisplayPanelConfig {
  enum class FullDisplayType : uint8_t {
    ILI9341 = 0,
    ST7789 = 1,
  } type;

  gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t backlight = gpio_num_t::GPIO_NUM_NC;
  LogicType backlight_logic = LogicType::active_high;
  uint16_t width = 320, height = 240;
  PanelFlags flags;
  SPIBus spi;

  TouchPanelConfig touch;
};

struct MinimalDisplayPanelConfig {
  enum class DisplayType {
    SSD1306 = 0,
  } type = DisplayType::SSD1306;

  gpio_num_t sda = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t scl = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
  uint8_t width = 128, height = 64;
};

enum class DisplayType {
  none,
  minimal,
  full,
};

struct NoDisplay {};
struct DisplayConfig {
  constexpr static bool supported =
#ifdef CONFIG_TESLASYNTH_GUI_ENABLED
      true;
#else
      false;
#endif
  DisplayType type = DisplayType::none;
  union PanelConfig {
    NoDisplay none;
    MinimalDisplayPanelConfig minimal;
    FullDisplayPanelConfig full;
  } config = {.none = NoDisplay{}};

  constexpr bool enabled() const { return type != DisplayType::none; }
  constexpr bool is_minimal() const { return type == DisplayType::minimal; }
  constexpr bool is_full() const { return type == DisplayType::full; }
};

struct LEDConfig {
  gpio_num_t pin;
  LogicType logic;

  LEDConfig();
};

struct OutputConfig {
  constexpr static uint8_t size = CONFIG_TESLASYNTH_OUTPUT_COUNT;
  std::array<OutputChannelConfig, size> channels;
};

struct InputConfig {
  gpio_num_t maintenance = gpio_num_t::GPIO_NUM_0;
};

struct HardwareConfig {
  constexpr static uint32_t current_version = 0;
  uint32_t version = current_version;
  OutputConfig output{};
  InputConfig input;
  LEDConfig led;

  HardwareConfig();
};

} // namespace teslasynth::app::configuration::hardware
