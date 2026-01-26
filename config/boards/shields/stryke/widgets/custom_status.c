#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <lvgl.h>

static lv_obj_t *screen;
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;

lv_obj_t *zmk_display_status_screen(void) {
    if (screen != NULL) {
        return screen;
    }

    screen = lv_obj_create(NULL);
    
    // Style: Black background
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // 1. Battery (Top Left)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    // 2. Layer (Top Right)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    // 3. Main Title "STRYKE"
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "STRYKE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -5);

    // 4. Subtitle "NEXUS PRO"
    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "NEXUS PRO");
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 15);

    return screen;
}