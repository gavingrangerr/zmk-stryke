#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include "custom_status.h"

static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_layer_status layer_status_widget;
static lv_obj_t *last_key_label;

/* * Event listener for Key Presses
 */
static int key_event_cb(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) { // If key is pressed (not released)
        // Note: keycodes are raw HID values; 0x04 is 'A', etc.
        char buf[20];
        snprintf(buf, sizeof(buf), "Last Key: 0x%02X", ev->keycode);
        lv_label_set_text(last_key_label, buf);
    }
    return 0;
}

ZMK_LISTENER(custom_key_listener, key_event_cb);
ZMK_SUBSCRIPTION(custom_key_listener, zmk_keycode_state_changed);

/*
 * Screen Construction
 */
lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // 1. Connection Status (Top Left)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 5);

    // 2. Layer Name/Index (Top Right)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_RIGHT, 0, 5);

    // 3. Horizontal Divider Line
    static lv_point_t line_points[] = {{0, 0}, {128, 0}};
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 2);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_GREY));

    lv_obj_t *line = lv_line_create(screen);
    lv_line_set_points(line, line_points, 2);
    lv_obj_add_style(line, &style_line, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 25);

    // 4. Custom Label for "Last Key" (Center-ish)
    last_key_label = lv_label_create(screen);
    lv_label_set_text(last_key_label, "Last Key: None");
    lv_obj_align(last_key_label, LV_ALIGN_CENTER, 0, 10);

    return screen;
}