#pragma once
#include <lvgl.h>

struct stryke_display_widget {
    lv_obj_t *obj;
    lv_obj_t *key_label;
    lv_obj_t *layer_labels[5];
};

int stryke_display_init(struct stryke_display_widget *widget, lv_obj_t *parent);
lv_obj_t *stryke_display_obj(struct stryke_display_widget *widget);