#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <lvgl.h>

static lv_obj_t *screen;
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;

lv_obj_t *zmk_display_status_screen(void) {
    // 1. If screen exists, just return it (LVGL handles the drawing)
    if (screen != NULL) {
        return screen;
    }

    // 2. Initialize the screen container
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // 3. Check uptime. If less than 5 seconds, show Splash Screen
    if (k_uptime_get() < 5000) {
        lv_obj_t *splash_label = lv_label_create(screen);
        lv_label_set_text(splash_label, "STRYKE BOOTING...");
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);
        return screen; 
    }

    // 4. Normal UI: Battery widget
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(
        zmk_widget_battery_status_obj(&battery_status_widget),
        LV_ALIGN_TOP_LEFT, 0, 0
    );

    // 5. Normal UI: Layer widget
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(
        zmk_widget_layer_status_obj(&layer_status_widget),
        LV_ALIGN_TOP_RIGHT, 0, 0
    );

    // 6. Normal UI: Center Title
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "STRYKE");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // 7. Normal UI: Bottom label
    lv_obj_t *status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "NEXUS PRO");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    return screen;
}