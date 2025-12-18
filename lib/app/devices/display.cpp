#include "sdkconfig.h"

#ifndef CONFIG_TESLASYNTH_GUI_NONE

#include "display.hpp"
#include "driver/i2c_master.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_stmpe610.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lvgl_port_disp.h"
#include "esp_lvgl_port_touch.h"
#include "freertos/task.h"
#include "hal/ledc_types.h"
#include "hal/spi_types.h"
#include "soc/gpio_num.h"
#include <cstdint>
#include <driver/ledc.h>
#include <stdio.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

namespace teslasynth::app::devices::display {
using namespace configuration::hardware;

namespace {
constexpr char const *TAG = "DISPLAY";
constexpr ledc_channel_t ledc_channel = ledc_channel_t::LEDC_CHANNEL_1;
bool has_brightness = false;

esp_err_t brightness_init(const FullDisplayPanelConfig &config) {
  has_brightness = true;

  const ledc_channel_config_t LCD_backlight_channel = {
      .gpio_num = config.backlight,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = ledc_channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = ledc_timer_t::LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0,
      .flags = {
          .output_invert = (config.backlight_logic == LogicType::active_low),
      }};

  const ledc_timer_config_t LCD_backlight_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_10_BIT,
      .timer_num = ledc_timer_t::LEDC_TIMER_1,
      .freq_hz = 5000,
      .clk_cfg = LEDC_AUTO_CLK,
  };

  ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
  ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

  return ESP_OK;
}

esp_lcd_panel_io_handle_t
install_full_io(const FullDisplayPanelConfig &config) {
  constexpr spi_host_device_t LCD_HOST = SPI2_HOST;
  ESP_LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num = config.spi.mosi,
      .miso_io_num = config.spi.miso,
      .sclk_io_num = config.spi.clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz =
          static_cast<int>(config.height * 80 * sizeof(uint16_t)),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = config.cs,
      .dc_gpio_num = config.dc,
      .spi_mode = 0,
      .pclk_hz = 20 * 1000 * 1000, // 20MHz
      .trans_queue_depth = 10,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));
  return io_handle;
}
esp_lcd_panel_handle_t install_full_panel(const FullDisplayPanelConfig &config,
                                          esp_lcd_panel_io_handle_t io_handle) {
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = config.rs,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };
  switch (config.type) {
  case configuration::hardware::FullDisplayPanelConfig::ILI9341:
    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    break;
  case configuration::hardware::FullDisplayPanelConfig::ST7789:
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    break;
  };
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, config.flags.mirror_x,
                                       config.flags.mirror_y));

  // user can flush pre-defined pattern to the screen before we turn on the
  // screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  return panel_handle;
}
lv_display_t *full_display(const FullDisplayPanelConfig &config) {
  ESP_ERROR_CHECK(brightness_init(config));
  const auto io_handle = install_full_io(config);
  const auto panel_handle = install_full_panel(config, io_handle);
  ESP_LOGI(TAG, "Install the display");
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = static_cast<uint32_t>(config.height * 50),
      .double_buffer = false,
      .hres = config.width,
      .vres = config.height,
      .monochrome = false,
      .rotation =
          {
              .swap_xy = config.flags.swap_xy,
              .mirror_x = config.flags.mirror_x,
              .mirror_y = config.flags.mirror_y,
          },
      .color_format = LV_COLOR_FORMAT_RGB565,
      .flags =
          {
              .buff_dma = true,
              .swap_bytes = true,
          },
  };
  return lvgl_port_add_disp(&disp_cfg);
}

esp_lcd_panel_io_handle_t
install_minimal_io(const MinimalDisplayPanelConfig &config) {
  ESP_LOGI(TAG, "Initialize I2C bus");
  i2c_master_bus_handle_t i2c_bus = NULL;
  i2c_master_bus_config_t bus_config = {
      .i2c_port = 0,
      .sda_io_num = config.sda,
      .scl_io_num = config.scl,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags = {.enable_internal_pullup = true},
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t io_config = {
      .dev_addr = 0x3C,
      .control_phase_bytes = 1, // According to SSD1306 datasheet
      .dc_bit_offset = 6,       // According to SSD1306 datasheet
      .lcd_cmd_bits = 8,        // According to SSD1306 datasheet
      .lcd_param_bits = 8,      // According to SSD1306 datasheet
      .scl_speed_hz = 400 * 1000,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));
  return io_handle;
}

esp_lcd_panel_handle_t
install_minimal_panel(const MinimalDisplayPanelConfig &config,
                      esp_lcd_panel_io_handle_t io_handle) {
  ESP_LOGI(TAG, "Install SSD1306 panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = config.rs,
      .bits_per_pixel = 1,
  };
  esp_lcd_panel_ssd1306_config_t ssd1306_config = {.height = config.height};
  panel_config.vendor_config = &ssd1306_config;
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  return panel_handle;
}

lv_display_t *minimal_display(const MinimalDisplayPanelConfig &config) {
  auto io_handle = install_minimal_io(config);
  auto panel_handle = install_minimal_panel(config, io_handle);

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = static_cast<uint32_t>(config.width * config.height),
      .double_buffer = false,
      .hres = config.width,
      .vres = config.height,
      .monochrome = true,
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = false,
              .mirror_y = false,
          },
      .flags =
          {
              .buff_dma = true,
              .swap_bytes = false,
          },
  };
  ESP_LOGI(TAG, "Install display");
  lv_display_t *disp_handle = lvgl_port_add_disp(&disp_cfg);
  return disp_handle;
}

esp_lcd_panel_io_handle_t
install_touch_io(const FullDisplayPanelConfig &config) {
  static const int SPI_MAX_TRANSFER_SIZE = 32768;
  bool uses_secondary_spi = config.touch.spi.has_value();
  spi_host_device_t TOUCH_SPI = uses_secondary_spi ? SPI3_HOST : SPI2_HOST;
  if (uses_secondary_spi) {
    ESP_LOGI(TAG, "Initialize Touch SPI bus");
    const auto spi = *config.touch.spi;
    const spi_bus_config_t buscfg_touch = {
        .mosi_io_num = spi.mosi,
        .miso_io_num = spi.miso,
        .sclk_io_num = spi.clk,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO |
                 SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER |
                 SPICOMMON_BUSFLAG_GPIO_PINS,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM,
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(TOUCH_SPI, &buscfg_touch, SPI_DMA_CH_AUTO));
  }

  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  const esp_lcd_panel_io_spi_config_t tp_io_config = {
      .cs_gpio_num = config.touch.cs,
      .dc_gpio_num = config.touch.dc,
      .spi_mode = 0,
      .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,
      .trans_queue_depth = 3,
      .on_color_trans_done = NULL,
      .user_ctx = NULL,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .flags =
          {
              .dc_high_on_cmd = 0,
              .dc_low_on_data = 0,
              .dc_low_on_param = 0,
              .octal_mode = 0,
              .quad_mode = 0,
              .sio_mode = 0,
              .lsb_first = 0,
              .cs_high_active = 0,
          },
  };

  ESP_ERROR_CHECK(
      esp_lcd_new_panel_io_spi(TOUCH_SPI, &tp_io_config, &tp_io_handle));

  return tp_io_handle;
}

esp_lcd_touch_handle_t
install_touch_panel(const FullDisplayPanelConfig &config,
                    esp_lcd_panel_io_handle_t tp_io_handle) {
  esp_lcd_touch_handle_t tp = NULL;

  const esp_lcd_touch_config_t tp_cfg = {
      .x_max = config.height,
      .y_max = config.width,
      .rst_gpio_num = config.touch.rs,
      .int_gpio_num = config.touch.irq,
      .levels = {.reset = 0, .interrupt = 0},
      .flags =
          {
              .swap_xy = config.touch.flags.swap_xy,
              .mirror_x = config.touch.flags.mirror_x,
              .mirror_y = config.touch.flags.mirror_y,
          },
      .interrupt_callback = NULL,
  };

  switch (config.touch.type) {
  case TouchPanelConfig::XPT2046:
    ESP_LOGI(TAG, "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
    break;
  case TouchPanelConfig::STMPE610:
    ESP_LOGI(TAG, "Initialize touch controller STMPE610");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_stmpe610(tp_io_handle, &tp_cfg, &tp));
    break;
  }

  return tp;
}

lv_indev_t *install_touch(const FullDisplayPanelConfig &config,
                          lv_display_t *display) {
  auto io_handle = install_touch_io(config);
  auto tp = install_touch_panel(config, io_handle);

  const lvgl_port_touch_cfg_t touch_cfg = {.disp = display, .handle = tp};
  static lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
  return indev;
}
}; // namespace

lv_display_t *init(const MinimalDisplayPanelConfig &config) {
  return minimal_display(config);
}

lv_display_t *init(const FullDisplayPanelConfig &config) {
  lv_display_t *display = full_display(config);
  if (config.touch.enabled) {
    install_touch(config, display);
  }
  return display;
}

esp_err_t brightness_set(uint8_t brightness_percent) {
  if (!has_brightness)
    return ESP_OK;
  if (brightness_percent > 100) {
    brightness_percent = 100;
  }

  ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);

  uint32_t duty_cycle = (1023 * brightness_percent) / 100;

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel, duty_cycle));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel));
  return ESP_OK;
}
esp_err_t backlight_off(void) {
  ESP_LOGI(TAG, "Turn off LCD backlight");
  return brightness_set(0);
}
esp_err_t backlight_on(void) {
  ESP_LOGI(TAG, "Turn on LCD backlight");
  return brightness_set(100);
}
} // namespace teslasynth::app::devices::display

#endif
