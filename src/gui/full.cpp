#include "configuration/synth.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "font/lv_symbol_def.h"
#include "freertos/task.h"
#include "helpers/sysinfo.h"
#include "input/ble_midi.hpp"
#include "lv_api_map_v8.h"
#include "misc/lv_area.h"
#include "misc/lv_async.h"
#include "misc/lv_types.h"
#include "notes.hpp"
#include "synthesizer_events.hpp"
#include "widgets/label/lv_label.h"
#include "widgets/menu/lv_menu.h"
#include <cstdint>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#if CONFIG_TESLASYNTH_GUI_FULL

static const char *TAG = "GUI";

static lv_display_t *display;
extern lv_display_t *install_display();
extern esp_err_t lcd_display_brightness_set(int brightness_percent);
extern esp_err_t lcd_display_backlight_off(void);
extern esp_err_t lcd_display_backlight_on(void);

#if CONFIG_TESLASYNTH_TOUCH_ENABLED
extern lv_indev_t *install_touch(lv_display_t *display);
#endif

LV_IMG_DECLARE(teslasynth_240p_large);

lv_obj_t *main_screen, *splash_screen;

void init_ui() {
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
}

lv_obj_t *label1, *label2;
void render_config(void *) {
  if (label1 == nullptr || label2 == nullptr)
    return;
  const Config &config = get_config();

  lv_label_set_text_fmt(label1, "Max on: %s",
                        std::string(config.max_on_time).c_str());
  lv_label_set_text_fmt(label2, "Notes: %u", config.notes);
}

void splash_load_cb(lv_event_t *e) {
  /* load main screen after 3000ms */
  lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 3000, false);
  lcd_display_brightness_set(70);
}

lv_obj_t *bluetooth_indicator, *play_indicator;
static lv_timer_t *blink_timer;

static void blink_cb(lv_timer_t *t) {
  lv_obj_t *icon = static_cast<lv_obj_t *>(lv_timer_get_user_data(t));

  if (lv_obj_has_flag(icon, LV_OBJ_FLAG_HIDDEN))
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
}
static void ui_on_connection_changed(void *event) {
  if (static_cast<bool>(event)) {
    lv_timer_pause(blink_timer);
    lv_obj_clear_flag(bluetooth_indicator, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_timer_resume(blink_timer);
  }
}

static void ui_on_track_play_changed(void *event) {
  if (static_cast<bool>(event)) {
    lv_image_set_src(play_indicator, LV_SYMBOL_PLAY);
  } else {
    lv_image_set_src(play_indicator, LV_SYMBOL_PAUSE);
  }
}

void init_splash_screen() {
  ESP_LOGI(TAG, "splash screen");
  splash_screen = lv_obj_create(nullptr);

  lv_obj_t *welcome = lv_image_create(splash_screen);
  lv_image_set_src(welcome, &teslasynth_240p_large);
  lv_obj_align(welcome, LV_ALIGN_TOP_LEFT, 0, 0);
}

static lv_obj_t *create_text(lv_obj_t *parent, const char *icon,
                             const char *txt, bool variant2 = false) {
  lv_obj_t *obj = lv_menu_cont_create(parent);

  lv_obj_t *img = NULL;
  lv_obj_t *label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
  }

  if (txt) {
    label = lv_label_create(obj);
    lv_label_set_text(label, txt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  if (variant2 && icon && txt) {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

static lv_obj_t *create_text_fmt(lv_obj_t *parent, const char *icon,
                                 const char *fmt, ...) {
  lv_obj_t *obj = lv_menu_cont_create(parent);
  lv_obj_t *img = NULL;
  lv_obj_t *label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
  }

  if (fmt) {
    label = lv_label_create(obj);
    va_list args;
    va_start(args, fmt);
    lv_label_set_text_vfmt(label, fmt, args);
    va_end(args);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  return obj;
}

static lv_obj_t *create_slider(lv_obj_t *parent, const char *icon,
                               const char *txt, int32_t min, int32_t max,
                               int32_t val) {
  lv_obj_t *obj = create_text(parent, icon, txt, true);

  lv_obj_t *slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (icon == NULL) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  }

  return obj;
}

void create_statusbar(lv_obj_t *screen) {
  lv_obj_t *status_bar = lv_obj_create(screen);
  lv_obj_set_size(status_bar, LV_HOR_RES, 30);
  lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_pad_all(status_bar, 5, 0);

  lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);

  lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *tslabel = lv_label_create(status_bar);
  lv_label_set_long_mode(tslabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text(tslabel, "TeslaSynth");

  lv_obj_t *icon_container = lv_obj_create(status_bar);
  lv_obj_set_size(icon_container, 128, LV_SIZE_CONTENT);
  lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
  /* Remove border/background */
  lv_obj_set_style_border_width(icon_container, 0, 0);
  lv_obj_set_style_bg_opa(icon_container, LV_OPA_TRANSP, 0);

  /* Flex row so icons line up */
  lv_obj_set_flex_flow(icon_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(icon_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  bluetooth_indicator = lv_image_create(icon_container);
  lv_image_set_src(bluetooth_indicator, LV_SYMBOL_BLUETOOTH);
  blink_timer = lv_timer_create(blink_cb, 750, bluetooth_indicator);

  play_indicator = lv_image_create(icon_container);
  lv_image_set_src(play_indicator, LV_SYMBOL_PAUSE);
}

lv_obj_t *create_home_section(lv_obj_t *menu) {
  /*Create sub pages*/
  lv_obj_t *sub_status_page = lv_menu_page_create(menu, "Home");
  lv_obj_set_style_pad_hor(
      sub_status_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_status_page);
  auto section = lv_menu_section_create(sub_status_page);

  label1 = lv_label_create(section);
  lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(label1, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label1, LV_ALIGN_LEFT_MID, 0, 0);

  label2 = lv_label_create(section);
  lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);

  lv_obj_set_width(label2, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label2, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  render_config(nullptr);

  return sub_status_page;
}

lv_obj_t *create_test_section(lv_obj_t *menu) {
  /*Create sub pages*/
  lv_obj_t *sub_mechanics_page = lv_menu_page_create(menu, "Test");
  lv_obj_set_style_pad_hor(
      sub_mechanics_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_mechanics_page);
  auto section = lv_menu_section_create(sub_mechanics_page);
  create_slider(section, LV_SYMBOL_SETTINGS, "Velocity", 0, 150, 120);
  create_slider(section, LV_SYMBOL_SETTINGS, "Acceleration", 0, 150, 50);
  create_slider(section, LV_SYMBOL_SETTINGS, "Weight limit", 0, 150, 80);

  return sub_mechanics_page;
}

lv_obj_t *create_software_info_section(lv_obj_t *menu) {
  lv_obj_t *sub_software_info_page =
      lv_menu_page_create(menu, "Software info.");
  lv_obj_set_style_pad_hor(
      sub_software_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  auto section = lv_menu_section_create(sub_software_info_page);

  auto app_version = esp_app_get_description();
  create_text_fmt(section, nullptr, "Version: %s", app_version->version);
  create_text_fmt(section, nullptr, "Compiled at: %s %s", app_version->date,
                  app_version->time);
  create_text_fmt(section, nullptr, "IDF Version: %s", esp_get_idf_version());
  return sub_software_info_page;
}

lv_obj_t *create_hardware_info_section(lv_obj_t *menu) {
  lv_obj_t *sub_hardware_info_page =
      lv_menu_page_create(menu, "Hardware info.");
  lv_obj_set_style_pad_hor(
      sub_hardware_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  auto section = lv_menu_section_create(sub_hardware_info_page);

  ChipInfo info;
  auto err = get_chip_info(info);
  if (err == ESP_OK) {
    create_text_fmt(section, nullptr, "Chip model: %s", info.model);
    create_text_fmt(section, nullptr, "Cores: %d", info.cores);
    create_text(section, nullptr, "Features: ");
    create_text_fmt(section, nullptr, "%s%s%s%s%" PRIu32 "%s\r\n",
                    info.wifi ? "/802.11bgn" : "", info.ble ? "/BLE" : "",
                    info.bt ? "/BT" : "",
                    info.emb_flash ? "/Embedded-Flash:" : "/External-Flash:",
                    info.flash_size, " MB");
    create_text_fmt(section, nullptr, "Revision: %d", info.revision);
  } else {
    create_text(section, nullptr, "Unknown hardware!");
  }

  return sub_hardware_info_page;
}

lv_obj_t *create_legal_info_section(lv_obj_t *menu) {
  lv_obj_t *sub_legal_info_page = lv_menu_page_create(menu, "Legal info.");
  lv_obj_set_style_pad_hor(
      sub_legal_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  auto section = lv_menu_section_create(sub_legal_info_page);
  create_text(section, NULL, "");
  return sub_legal_info_page;
}

lv_obj_t *create_about_section(lv_obj_t *menu) {
  lv_obj_t *sub_software_info_page = create_software_info_section(menu);
  lv_obj_t *sub_hardware_info_page = create_hardware_info_section(menu);
  lv_obj_t *sub_legal_info_page = create_legal_info_section(menu);

  lv_obj_t *sub_about_page = lv_menu_page_create(menu, "About");
  lv_obj_set_style_pad_hor(
      sub_about_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_about_page);
  auto section = lv_menu_section_create(sub_about_page);
  auto cont = create_text(section, NULL, "Software information");
  lv_menu_set_load_page_event(menu, cont, sub_software_info_page);
  cont = create_text(section, NULL, "Hardware information");
  lv_menu_set_load_page_event(menu, cont, sub_hardware_info_page);
  cont = create_text(section, NULL, "Legal information");
  lv_menu_set_load_page_event(menu, cont, sub_legal_info_page);

  return sub_about_page;
}

static lv_obj_t *root_page;

void create_main_menu(lv_obj_t *screen) {
  lv_obj_t *menu = lv_menu_create(screen);

  lv_color_t bg_color = lv_obj_get_style_bg_color(menu, LV_PART_MAIN);
  if (lv_color_brightness(bg_color) > 127) {
    lv_obj_set_style_bg_color(
        menu,
        lv_color_darken(lv_obj_get_style_bg_color(menu, LV_PART_MAIN), 10), 0);
  } else {
    lv_obj_set_style_bg_color(
        menu,
        lv_color_darken(lv_obj_get_style_bg_color(menu, LV_PART_MAIN), 50), 0);
  }
  lv_menu_set_mode_header(menu, LV_MENU_HEADER_BOTTOM_FIXED);
  lv_obj_set_flex_grow(menu, 1);
  lv_obj_set_width(menu, LV_PCT(100));
  lv_obj_center(menu);

  // Adjusting the back button
  lv_obj_t *btn = lv_menu_get_main_header_back_button(menu);
  /* Make sure it's clickable and large enough */
  lv_obj_set_width(btn, 80);
  lv_obj_set_style_pad_all(btn, 8, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *back_btn_label = lv_label_create(btn);
  lv_label_set_text(back_btn_label, "back");

  // Creating sections
  auto sub_home_page = create_home_section(menu);
  auto sub_sound_page = create_test_section(menu);
  auto sub_display_page = create_test_section(menu);
  auto sub_about_page = create_about_section(menu);

  /*Create a root page*/
  lv_obj_t *cont, *section;
  root_page = lv_menu_page_create(menu, nullptr);
  lv_obj_set_style_pad_hor(
      root_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  section = lv_menu_section_create(root_page);
  cont = create_text(section, LV_SYMBOL_HOME, "Home");
  lv_menu_set_load_page_event(menu, cont, sub_home_page);
  cont = create_text(section, LV_SYMBOL_AUDIO, "Sound");
  lv_menu_set_load_page_event(menu, cont, sub_sound_page);
  cont = create_text(section, LV_SYMBOL_SETTINGS, "Config");
  lv_menu_set_load_page_event(menu, cont, sub_display_page);
  cont = create_text(section, NULL, "About");
  lv_menu_set_load_page_event(menu, cont, sub_about_page);

  lv_menu_set_sidebar_page(menu, NULL);
  lv_menu_set_page(menu, root_page);
}

void init_main_screen() {
  ESP_LOGI(TAG, "main screen");
  main_screen = lv_obj_create(nullptr);
  lv_obj_set_flex_flow(main_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(main_screen, 0, 0); // no extra padding

  create_statusbar(main_screen);
  create_main_menu(main_screen);
}

static void ble_event_handler(void *, esp_event_base_t, int32_t id, void *) {
  lv_async_call(ui_on_connection_changed, (void *)(id == BLE_DEVICE_CONNECTED));
}
static void track_event_handler(void *, esp_event_base_t, int32_t id, void *) {
  lv_async_call(ui_on_track_play_changed, (void *)(id == SYNTHESIZER_PLAYING));
}
static void config_update_handler(void *, esp_event_base_t, int32_t, void *) {
  lv_async_call(render_config, nullptr);
}

void start_gui() {
  /* Rotation of the screen */
  // lv_disp_set_rotation(display, LV_DISPLAY_ROTATION_0);

  lcd_display_brightness_set(50);
  init_splash_screen();
  init_main_screen();

  lv_obj_add_event_cb(splash_screen, splash_load_cb, LV_EVENT_SCREEN_LOADED,
                      NULL);
  lv_scr_load(splash_screen);
}

void init_gui() {
  init_ui();
  display = install_display();
#if CONFIG_TESLASYNTH_TOUCH_ENABLED
  install_touch(display);
#endif

  ESP_LOGI(TAG, "starting the UI");
  if (lvgl_port_lock(0)) {
    start_gui();
    lvgl_port_unlock();
  }
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(EVENT_BLE_BASE, BLE_DEVICE_CONNECTED,
                                          ble_event_handler, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_BLE_BASE, BLE_DEVICE_DISCONNECTED, ble_event_handler, nullptr,
      nullptr));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_SYNTHESIZER_BASE, SYNTHESIZER_PLAYING, track_event_handler, nullptr,
      nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_SYNTHESIZER_BASE, SYNTHESIZER_STOPPED, track_event_handler, nullptr,
      nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_SYNTHESIZER_BASE, SYNTHESIZER_CONFIG_UPDATED, config_update_handler,
      nullptr, nullptr));
}

#endif
