#include <zmk/display/status.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <lvgl.h>
#include "status.h"

static lv_obj_t *key_label;
static lv_obj_t *layer_label;

// Map raw keycodes to text
static const char *get_key_str(uint16_t code) {
    if (code >= 0x04 && code <= 0x1D) {
        static char letter[2] = {0};
        letter[0] = (code - 0x04) + 'A';
        return letter;
    }
    switch(code) {
        case 0x2C: return "SPACE";
        case 0x28: return "ENTER";
        default: return "KEY";
    }
}

// Event Handlers
int key_handler(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state && key_label) {
        lv_label_set_text(key_label, get_key_str(ev->keycode));
    }
    return 0;
}
ZMK_LISTENER(key_listener, key_handler);
ZMK_SUBSCRIPTION(key_listener, zmk_keycode_state_changed);

int layer_handler(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (layer_label) {
        char buf[12];
        snprintf(buf, sizeof(buf), "LYR: %d", ev->layer);
        lv_label_set_text(layer_label, buf);
    }
    return 0;
}
ZMK_LISTENER(layer_listener, layer_handler);
ZMK_SUBSCRIPTION(layer_listener, zmk_layer_state_changed);

// Main UI Construction
lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    key_label = lv_label_create(screen);
    lv_obj_set_style_text_font(key_label, &lv_font_unscii_16, 0);
    lv_label_set_text(key_label, "GO");
    lv_obj_align(key_label, LV_ALIGN_CENTER, 0, -10);

    layer_label = lv_label_create(screen);
    lv_obj_set_style_text_font(layer_label, &lv_font_unscii_8, 0);
    lv_label_set_text(layer_label, "LYR: 0");
    lv_obj_align(layer_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}
