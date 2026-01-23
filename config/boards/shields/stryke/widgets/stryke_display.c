#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keycode.h>
#include "stryke_display.h"

static struct stryke_display_widget *main_widget = NULL;
static const char *layer_names[] = {"Base", "KiCad", "Fusion", "VS-Code", "Arduino"};

/* ---------- LAYER UPDATE LOGIC ---------- */
static int layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev && main_widget != NULL) {
        if (ev->layer < ARRAY_SIZE(layer_names)) {
            lv_label_set_text_fmt(main_widget->layer_label, "LAYER: %s", layer_names[ev->layer]);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_listener, layer_listener);
ZMK_SUBSCRIPTION(layer_listener, zmk_layer_state_changed);

/* ---------- KEYCODE UPDATE LOGIC ---------- */
static int key_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    // Only update on "Press" (state = true), ignore "Release"
    if (ev && ev->state && main_widget != NULL) {
        char buffer[24];
        
        // Convert the ZMK keycode into a human-readable string
        // Note: zmk_keycode_to_string is a helper; if it's unavailable in your 
        // specific ZMK version, you may need a custom mapper.
        snprintf(buffer, sizeof(buffer), "ID: %d", ev->keycode); 
        
        lv_label_set_text_fmt(main_widget->key_label, "LAST: %s", buffer);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(key_listener, key_listener);
ZMK_SUBSCRIPTION(key_listener, zmk_keycode_state_changed);

/* ---------- WIDGET INITIALIZATION ---------- */
int stryke_display_init(struct stryke_display_widget *widget, lv_obj_t *parent) {
    // Standard ZMK container styling
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 32);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    // Layer Label (Top Line)
    widget->layer_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->layer_label, "LAYER: Base");
    lv_obj_align(widget->layer_label, LV_ALIGN_TOP_LEFT, 2, 2);

    // Keycode Label (Bottom Line)
    widget->key_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->key_label, "LAST: —");
    lv_obj_align(widget->key_label, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    main_widget = widget;
    return 0;
}

lv_obj_t *stryke_display_obj(struct stryke_display_widget *widget) {
    return widget->obj;
}