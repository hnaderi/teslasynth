#pragma once

#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <cstdint>
#include <optional>
#include <array>

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

struct LargeDisplayPanelConfig {
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

  struct TouchPanelConfig {
    enum {
      XPT2046,
      STMPE610,
    } type;
    bool enabled = false, secondary_spi = false;
    gpio_num_t cs = gpio_num_t::GPIO_NUM_NC;
    gpio_num_t dc = gpio_num_t::GPIO_NUM_NC;
    gpio_num_t rs = gpio_num_t::GPIO_NUM_NC;
    gpio_num_t irq = gpio_num_t::GPIO_NUM_NC;
    std::optional<SPIBus> spi;
  } touch;
};

struct SmallDisplayPanelConfig {
  enum DisplayType {
    SSD1306,
  } type;
  gpio_num_t sda = gpio_num_t::GPIO_NUM_NC;
  gpio_num_t scl = gpio_num_t::GPIO_NUM_NC;
  uint16_t width = 128, height = 64;
};

enum GUIType {
  none,
  minimal,
  full,
};

struct NoGUI {};
struct MinimalGUI {
  SmallDisplayPanelConfig display;
};
struct FullGUI {
  LargeDisplayPanelConfig display;
};
union GUI {
  NoGUI none;
  MinimalGUI minimal;
  FullGUI full;
};

template <std::uint8_t SIZE> struct OutputConfig {
  constexpr static auto size = SIZE;
  std::array<OutputChannelConfig, size> channels{};
};

template <std::uint8_t MAX_CHANNELS> struct HardwareConfig {
  uint32_t version = 0;
  uint8_t channels = MAX_CHANNELS;
  GUIType gui_type = none;
  GUI gui = {.none = NoGUI{}};
  OutputConfig<MAX_CHANNELS> outputs{};
};

typedef HardwareConfig<CONFIG_TESLASYNTH_OUTPUT_COUNT> AppHardwareConfig;
} // namespace teslasynth::app::configuration::hardware
