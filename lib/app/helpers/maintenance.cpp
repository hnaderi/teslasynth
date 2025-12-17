#include "maintenance.hpp"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include <cstdint>

RTC_NOINIT_ATTR static uint32_t state = 0;
constexpr uint32_t magic_value = 0x5A5A5A5Au;

namespace teslasynth::app::helpers::maintenance {
ESP_EVENT_DEFINE_BASE(MAINT_EVENT_BASE);
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

namespace {
constexpr char const *TAG = "MAINTENANCE";
enum class MaintState { IDLE, HOLDING, WAIT_RELEASE };
gpio_num_t button = gpio_num_t::GPIO_NUM_NC;
#define MAINT_HOLD_MS 3000
#define MAINT_RELEASE_MS 1000

bool boot_button_init(const configuration::hardware::InputConfig &config) {
  button = config.maintenance;
  if (button == gpio_num_t::GPIO_NUM_NC)
    return false;
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << button),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&io_conf);
  return true;
}

bool check_boot_hold_for_maintenance(void) {
  static MaintState state = MaintState::IDLE;
  static int64_t t0 = 0;

  if (button == GPIO_NUM_NC)
    return false;

  int level = gpio_get_level(button);
  int64_t now = esp_timer_get_time() / 1000;

  switch (state) {

  case MaintState::IDLE:
    if (level == 0) {
      t0 = now;
      state = MaintState::HOLDING;
      ESP_LOGI(TAG, "hold button for %d seconds to enter maintenance",
               MAINT_HOLD_MS / 1000);
      esp_event_post(MAINT_EVENT_BASE, MAINT_EVT_HOLD_STARTED, nullptr, 0, 0);
    }
    break;

  case MaintState::HOLDING:
    if (level == 0) {
      if ((now - t0) >= MAINT_HOLD_MS) {
        t0 = now;
        state = MaintState::WAIT_RELEASE;
        ESP_LOGI(TAG, "release button to confirm maintenance reboot");
        esp_event_post(MAINT_EVENT_BASE, MAINT_EVT_RELEASE_PHASE, nullptr, 0,
                       0);
      }
    } else {
      state = MaintState::IDLE;
      ESP_LOGI(TAG, "aborted (released too early)");
      esp_event_post(MAINT_EVENT_BASE, MAINT_EVT_ABORTED, nullptr, 0, 0);
    }
    break;

  case MaintState::WAIT_RELEASE:
    if (level != 0) {
      if ((now - t0) >= MAINT_RELEASE_MS) {
        state = MaintState::IDLE;
        ESP_LOGI(TAG, "confirmed, entering mode");
        esp_event_post(MAINT_EVENT_BASE, MAINT_EVT_TRIGGERED, nullptr, 0, 0);
        return true;
      }
    } else {
      t0 = now;
    }
    break;
  }

  return false;
}

void task(void *) {
  while (1) {
    if (check_boot_hold_for_maintenance()) {
      reboot();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
} // namespace

void init(const configuration::hardware::InputConfig &config) {
  if (boot_button_init(config)) {
    xTaskCreate(task, "maintenance_check", 2048, nullptr, 1, nullptr);
  }
}

} // namespace teslasynth::app::helpers::maintenance
