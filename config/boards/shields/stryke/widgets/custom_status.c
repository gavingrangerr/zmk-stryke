#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>

static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;
static lv_obj_t *last_key_label;

static int key_event_cb(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) {
        char buf[20];
        snprintf(buf, sizeof(buf), "Key: 0x%02X", ev->keycode);
        lv_label_set_text(last_key_label, buf);
    }
    return 0;
}

ZMK_LISTENER(custom_key_listener, key_event_cb);
ZMK_SUBSCRIPTION(custom_key_listener, zmk_keycode_state_changed);

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Battery (Top Left)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    // Layer (Top Right)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    // Key Press Label (Center)
    last_key_label = lv_label_create(screen);
    lv_label_set_text(last_key_label, "NexusPro Ready");
    lv_obj_align(last_key_label, LV_ALIGN_CENTER, 0, 0);

    return screen;
}