#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <lvgl.h>

static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // 1. Force Black Background to wipe the 'snow'
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // 2. Battery Widget (Top Left)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 4, 4);

    // 3. Layer Widget (Top Right)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_RIGHT, -4, 4);

    // 4. Main Label (Center)
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "STRYKE");
    
    // If you add CONFIG_LV_FONT_MONTSERRAT_16=y to stryke.conf, use this:
    // lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN);
    
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // 5. System Status (Bottom)
    lv_obj_t *status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "NEXUS PRO");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    return screen;
}