#pragma once
#include <lvgl.h>

struct stryke_display_widget {
    lv_obj_t *obj;
    
    // Main key display area
    lv_obj_t *key_display;
    lv_obj_t *key_label;
    
    // Layer indicators
    lv_obj_t *layer_indicators[5];
    lv_obj_t *layer_labels[5];
};

// Reference key dimensions
#define CARD_HEIGHT 36
#define MAX_TEXT_HEIGHT 16

int stryke_display_init(struct stryke_display_widget *widget, lv_obj_t *parent);
lv_obj_t *stryke_display_obj(struct stryke_display_widget *widget);
void stryke_display_update_layer(uint8_t layer);
void stryke_display_update_key(uint16_t keycode, bool pressed, const char *key_name);