#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <lvgl.h>

static lv_obj_t *screen;
static lv_obj_t *key_card;
static lv_obj_t *key_label;
static lv_obj_t *layer_rects[5];
static lv_obj_t *layer_texts[5];

// --- Keypress Event Handler ---
int key_listener_cb(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) { // On Press
        // Simplified: Shows raw keycode. In production, use a lookup table for names.
        char buf[10];
        snprintf(buf, sizeof(buf), "K: %02X", ev->keycode);
        lv_label_set_text(key_label, buf);
        
        // Visual "Flash" animation (white background)
        lv_obj_set_style_bg_color(key_card, lv_color_white(), 0);
        lv_obj_set_style_text_color(key_label, lv_color_black(), 0);
    } else { // On Release
        lv_obj_set_style_bg_color(key_card, lv_color_black(), 0);
        lv_obj_set_style_text_color(key_label, lv_color_white(), 0);
    }
    return 0;
}
ZMK_LISTENER(key_listener, key_listener_cb);
ZMK_SUBSCRIPTION(key_listener, zmk_keycode_state_changed);

// --- Layer Event Handler ---
int layer_listener_cb(const zmk_event_t *eh) {
    uint8_t active = zmk_layer_state_get();
    for (int i = 0; i < 5; i++) {
        if (i == active) {
            lv_obj_set_style_bg_opa(layer_rects[i], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(layer_texts[i], lv_color_black(), 0);
        } else {
            lv_obj_set_style_bg_opa(layer_rects[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(layer_texts[i], lv_color_white(), 0);
        }
    }
    return 0;
}
ZMK_LISTENER(layer_listener, layer_listener_cb);
ZMK_SUBSCRIPTION(layer_listener, zmk_layer_state_changed);

// --- Initialization of the Screen ---
lv_obj_t *zmk_display_status_screen() {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    // 1. Draw Key Card (Top Area)
    key_card = lv_obj_create(screen);
    lv_obj_set_size(key_card, 124, 36);
    lv_obj_align(key_card, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_radius(key_card, 8, 0);
    lv_obj_set_style_border_width(key_card, 2, 0);
    lv_obj_set_style_border_color(key_card, lv_color_white(), 0);
    lv_obj_set_style_bg_color(key_card, lv_color_black(), 0);

    key_label = lv_label_create(key_card);
    lv_label_set_text(key_label, "ZMK READY");
    lv_obj_center(key_label);

    // 2. Draw Layer Boxes (L0 to L4)
    for (int i = 0; i < 5; i++) {
        layer_rects[i] = lv_obj_create(screen);
        lv_obj_set_size(layer_rects[i], 22, 12);
        lv_obj_set_pos(layer_rects[i], 4 + (i * 25), 46);
        lv_obj_set_style_radius(layer_rects[i], 2, 0);
        lv_obj_set_style_border_width(layer_rects[i], 1, 0);
        lv_obj_set_style_border_color(layer_rects[i], lv_color_white(), 0);
        lv_obj_set_style_bg_color(layer_rects[i], lv_color_white(), 0);
        lv_obj_set_style_bg_opa(layer_rects[i], (i == 0) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

        layer_texts[i] = lv_label_create(layer_rects[i]);
        lv_label_set_text_fmt(layer_texts[i], "L%d", i);
        lv_obj_set_style_text_color(layer_texts[i], (i == 0) ? lv_color_black() : lv_color_white(), 0);
        lv_obj_center(layer_texts[i]);
    }

    return screen;
}