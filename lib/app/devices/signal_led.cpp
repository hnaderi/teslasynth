#include "signal_led.hpp"
#include "../helpers/maintenance.hpp"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "soc/gpio_num.h"

namespace teslasynth::app::devices::signal_led {
namespace {
constexpr char TAG[] = "LED";

typedef enum {
  LED_PATTERN_OFF,
  LED_PATTERN_SOLID,
  LED_PATTERN_BLINK_SLOW,
  LED_PATTERN_BLINK_FAST,
  LED_PATTERN_CONFIRM,
} led_pattern_t;

typedef struct {
  uint32_t on_ms;
  uint32_t off_ms;
} led_pattern_desc_t;

configuration::hardware::LEDConfig config_;
TaskHandle_t led_task = nullptr;
volatile led_pattern_t current_pattern;

constexpr led_pattern_desc_t pattern_table[] = {
    {0, 0}, {1000, 0}, {500, 500}, {150, 150}, {1000, 0},
};

void led_gpio_set(bool on) {
  gpio_set_level(config_.pin, on == static_cast<bool>(config_.logic));
}

void led_apply_pattern(led_pattern_t p) { current_pattern = p; }

void led_task_fn(void *) {
  while (true) {
    const led_pattern_desc_t &p = pattern_table[current_pattern];

    switch (current_pattern) {
    case LED_PATTERN_OFF:
      led_gpio_set(false);
      vTaskDelay(pdMS_TO_TICKS(250));
      break;

    case LED_PATTERN_SOLID:
      led_gpio_set(true);
      vTaskDelay(pdMS_TO_TICKS(250));
      break;

    case LED_PATTERN_CONFIRM:
      led_gpio_set(true);
      vTaskDelay(pdMS_TO_TICKS(p.on_ms));
      led_apply_pattern(LED_PATTERN_SOLID);
      break;

    default:
      led_gpio_set(true);
      vTaskDelay(pdMS_TO_TICKS(p.on_ms));
      led_gpio_set(false);
      vTaskDelay(pdMS_TO_TICKS(p.off_ms));
      break;
    }
  }
}

void maint_event_handler(void *, esp_event_base_t, int32_t id, void *) {
  switch (id) {
  case helpers::maintenance::MAINT_EVT_HOLD_STARTED:
    led_apply_pattern(LED_PATTERN_BLINK_SLOW);
    break;

  case helpers::maintenance::MAINT_EVT_RELEASE_PHASE:
    led_apply_pattern(LED_PATTERN_BLINK_FAST);
    break;

  case helpers::maintenance::MAINT_EVT_ABORTED:
    led_apply_pattern(LED_PATTERN_OFF);
    break;

  case helpers::maintenance::MAINT_EVT_TRIGGERED:
    led_apply_pattern(LED_PATTERN_CONFIRM);
    break;
  }
}
} // namespace

void init(const configuration::hardware::LEDConfig &config) {
  if (config.pin == gpio_num_t::GPIO_NUM_NC)
    return;

  config_ = config;

  gpio_config_t cfg = {};
  cfg.pin_bit_mask = (1ULL << config.pin);
  cfg.mode = GPIO_MODE_OUTPUT;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);

  led_apply_pattern(LED_PATTERN_OFF);

  xTaskCreate(led_task_fn, "signal_led", 1024, nullptr, 1, &led_task);

  ESP_ERROR_CHECK(esp_event_handler_register(
      helpers::maintenance::MAINT_EVENT_BASE, ESP_EVENT_ANY_ID,
      &maint_event_handler, nullptr));

  ESP_LOGI(TAG, "LED controller initialized on GPIO %d", config.pin);
}

} // namespace teslasynth::app::devices::signal_led
