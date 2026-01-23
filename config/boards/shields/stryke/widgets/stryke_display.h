#pragma once
#include <lvgl.h>
#include <zephyr/kernel.h>

struct nexuspro_display_widget {
    lv_obj_t *obj;
    lv_obj_t *layer_label;
    lv_obj_t *key_label;
};

int nexuspro_display_init(struct nexuspro_display_widget *widget, lv_obj_t *parent);
lv_obj_t *nexuspro_display_obj(struct nexuspro_display_widget *widget);