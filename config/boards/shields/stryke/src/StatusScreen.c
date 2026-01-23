#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <lvgl.h>

static lv_obj_t *layer_label;
static lv_obj_t *key_label;

// Handle Layer Changes
int layer_status_cb(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    char *layer_name = "Base"; // Default
    
    // Logic to match your layer index to names
    switch (ev->layer) {
        case 1: layer_name = "KiCad"; break;
        case 2: layer_name = "Fusion"; break;
        case 3: layer_name = "VS Code"; break;
    }
    lv_label_set_text(layer_label, layer_name);
    return 0;
}

// Handle Keypresses
int key_status_cb(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev->state) { // Key Pressed
        lv_label_set_text_fmt(key_label, "Key: %d", ev->position);
    }
    return 0;
}

ZMK_LISTENER(layer_status, layer_status_cb);
ZMK_SUBSCRIPTION(layer_status, zmk_layer_state_changed);

ZMK_LISTENER(key_status, key_status_cb);
ZMK_SUBSCRIPTION(key_status, zmk_position_state_changed);

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Create Layer Title
    layer_label = lv_label_create(screen);
    lv_label_set_text(layer_label, "Base");
    lv_obj_align(layer_label, LV_ALIGN_TOP_MID, 0, 5);

    // Create Keypress indicator
    key_label = lv_label_create(screen);
    lv_label_set_text(key_label, "Waiting...");
    lv_obj_align(key_label, LV_ALIGN_CENTER, 0, 0);

    return screen;
}