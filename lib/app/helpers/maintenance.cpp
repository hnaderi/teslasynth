#include "maintenance.hpp"
#include "esp_attr.h"
#include "esp_system.h"
#include <cstdint>

RTC_NOINIT_ATTR static uint32_t state = 0;
constexpr uint32_t magic_value = 0x5A5A5A5Au;

namespace teslasynth::app::helpers::maintenance {
bool check() {
  if (state == magic_value) {
    state = 0;
    return true;
  } else {
    return false;
  }
}
void reboot() {
  state = magic_value;
  esp_restart();
}
} // namespace teslasynth::app::helpers::maintenance
