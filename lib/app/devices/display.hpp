#pragma once

#include "configuration/hardware.hpp"
#include "esp_err.h"
#include "lvgl.h"
#include <cstdint>

namespace teslasynth::app::devices::display {

lv_display_t *
init(const configuration::hardware::MinimalDisplayPanelConfig &config);
lv_display_t *
init(const configuration::hardware::FullDisplayPanelConfig &config);
esp_err_t brightness_set(uint8_t brightness_percent);
esp_err_t backlight_off(void);
esp_err_t backlight_on(void);

} // namespace teslasynth::app::devices::display
