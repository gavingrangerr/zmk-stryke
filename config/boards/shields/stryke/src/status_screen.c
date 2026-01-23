#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/event_manager.h>
#include <lvgl.h>

static lv_obj_t *layer_label;
static lv_obj_t *key_label;

// Update Layer Text
int layer_status_cb(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    const char *layer_name;

    switch (ev->layer) {
        case 0: layer_name = "Base"; break;
        case 1: layer_name = "KiCad"; break;
        case 2: layer_name = "Fusion"; break;
        case 3: layer_name = "VS Code"; break;
        case 4: layer_name = "Arduino"; break;
        default: layer_name = "Extra"; break;
    }
    
    if (layer_label != NULL) {
        lv_label_set_text(layer_label, layer_name);
    }
    return 0;
}

// Update Keypress Text
int key_status_cb(const zmk_event_t *eh) {
    struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (key_label != NULL && ev->state) {
        lv_label_set_text_fmt(key_label, "Last Key: %d", ev->position);
    }
    return 0;
}

ZMK_LISTENER(layer_status, layer_status_cb);
ZMK_SUBSCRIPTION(layer_status, zmk_layer_state_changed);

ZMK_LISTENER(key_status, key_status_cb);
ZMK_SUBSCRIPTION(key_status, zmk_position_state_changed);

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    layer_label = lv_label_create(screen);
    lv_obj_align(layer_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(layer_label, "Base");

    key_label = lv_label_create(screen);
    lv_obj_align(key_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(key_label, "Ready...");

    return screen;
}