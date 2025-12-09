#pragma once

#include "lvgl.h"

lv_obj_t *create_text(lv_obj_t *parent, const char *icon, const char *txt,
                      bool new_track = false);

lv_obj_t *create_text_fmt(lv_obj_t *parent, const char *icon, const char *fmt,
                          ...);
