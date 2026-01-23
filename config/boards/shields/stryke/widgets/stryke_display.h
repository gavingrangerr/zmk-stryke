#pragma once
#include <lvgl.h>

#define CARD_HEIGHT 40
#define MAX_TEXT_HEIGHT 32

struct stryke_display_widget {
    lv_obj_t *obj;
    lv_obj_t *key_display;  // Added
    lv_obj_t *key_label;
    lv_obj_t *layer_indicators[5]; // Added
    lv_obj_t *layer_labels[5];
};

int stryke_display_init(struct stryke_display_widget *widget, lv_obj_t *parent);
lv_obj_t *stryke_display_obj(struct stryke_display_widget *widget);