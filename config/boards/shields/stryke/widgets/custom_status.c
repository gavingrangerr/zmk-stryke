#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <lvgl.h>

static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Battery Widget
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    // Layer Widget
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    // Simple Text to prove it works
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "NexusPro Active");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    return screen;
}