#include <zmk/display/status_screen.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <lvgl.h>

static lv_obj_t *key_label;
static lv_obj_t *layer_label;

// 1. Listen for key presses to update the "Center" UI
int key_event_cb(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) { // Only on "Press", not "Release"
        // Update label with the keycode (simplified hex or custom logic)
        char buf[10];
        snprintf(buf, sizeof(buf), "0x%02X", ev->keycode);
        lv_label_set_text(key_label, buf);
    }
    return 0;
}
ZMK_LISTENER(key_listener, key_event_cb);
ZMK_SUBSCRIPTION(key_listener, zmk_keycode_state_changed);

// 2. Listen for Layer changes
int layer_event_cb(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    char buf[12];
    snprintf(buf, sizeof(buf), "LAYER: %d", ev->layer);
    lv_label_set_text(layer_label, buf);
    return 0;
}
ZMK_LISTENER(layer_listener, layer_event_cb);
ZMK_SUBSCRIPTION(layer_listener, zmk_layer_state_changed);

// 3. Build the UI
lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Center Label (Key Pressed)
    key_label = lv_label_create(screen);
    lv_obj_set_style_text_font(key_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(key_label, "READY");
    lv_obj_align(key_label, LV_ALIGN_CENTER, 0, -10);

    // Bottom Label (Layer)
    layer_label = lv_label_create(screen);
    lv_obj_set_style_text_font(layer_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(layer_label, "LAYER: 0");
    lv_obj_align(layer_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}