#include "display.hpp"
#include "configuration/synth.hpp"
#include "core/lv_obj.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/task.h"
#include "media/128x64.h"
#include "misc/lv_area.h"
#include "notes.hpp"
#include "widgets/label/lv_label.h"
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

static const char *TAG = "display";

static lv_display_t *display;
extern lv_display_t *install_display();

lv_obj_t *main_screen, *splash_screen;

void init_ui() {
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
}

void render_config(lv_obj_t *parent) {
  Config &config = get_config();

  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text_fmt(label, "Max on: %s",
                        std::string(config.max_on_time).c_str());
  lv_obj_set_width(label, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
}

void splash_load_cb(lv_event_t *e) {
  /* load main screen after 3000ms */
  lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 3000, false);
}

void init_splash_screen() {
  ESP_LOGI(TAG, "splash screen");
  splash_screen = lv_obj_create(nullptr);

  lv_obj_t *icon = lv_image_create(splash_screen);
  lv_image_set_src(icon, &logo);
  lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);
}
void init_main_screen() {
  ESP_LOGI(TAG, "main screen");
  main_screen = lv_obj_create(nullptr);

  lv_obj_t *icon = lv_image_create(main_screen);
  lv_image_set_src(icon, &bluetooth_icon);
  lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *label = lv_label_create(main_screen);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text(label, "TeslaSynth");
  lv_obj_set_width(label, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 20, 0);

  render_config(main_screen);
}

void setup_ui() {
  init_ui();
  display = install_display();
  ESP_LOGI(TAG, "starting the UI");
  if (lvgl_port_lock(0)) {
    /* Rotation of the screen */
    // lv_disp_set_rotation(display, LV_DISPLAY_ROTATION_0);

    init_splash_screen();
    init_main_screen();

    lv_obj_add_event_cb(splash_screen, splash_load_cb, LV_EVENT_SCREEN_LOADED,
                        NULL);
    lv_scr_load(splash_screen);
    // Release the mutex
    lvgl_port_unlock();
  }
  // xTaskCreate(start_ui, "UI", 8 * 1024, nullptr, 0, nullptr);
}
