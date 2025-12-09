#include "components.hpp"

lv_obj_t *create_text(lv_obj_t *parent, const char *icon, const char *txt,
                      bool new_track) {
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

  if (new_track && icon && txt) {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

lv_obj_t *create_text_fmt(lv_obj_t *parent, const char *icon, const char *fmt,
                          ...) {
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
