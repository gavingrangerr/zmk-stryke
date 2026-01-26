#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <lvgl.h>

static lv_obj_t *screen = NULL;
static bool widgets_initialized = false;
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_battery_status battery_status_widget;
static lv_obj_t *title_label = NULL;
static lv_obj_t *status_label = NULL;

static void create_normal_ui(lv_obj_t *parent) {
    // Battery widget
    zmk_widget_battery_status_init(&battery_status_widget, parent);
    lv_obj_align(
        zmk_widget_battery_status_obj(&battery_status_widget),
        LV_ALIGN_TOP_LEFT, 0, 0
    );

    // Layer widget
    zmk_widget_layer_status_init(&layer_status_widget, parent);
    lv_obj_align(
        zmk_widget_layer_status_obj(&layer_status_widget),
        LV_ALIGN_TOP_RIGHT, 0, 0
    );

    // Center Title
    title_label = lv_label_create(parent);
    lv_label_set_text(title_label, "STRYKE");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // Bottom label
    status_label = lv_label_create(parent);
    lv_label_set_text(status_label, "NEXUS PRO");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -2);
}

lv_obj_t *zmk_display_status_screen(void) {
    // If screen exists and widgets initialized, return it
    if (screen != NULL && widgets_initialized) {
        return screen;
    }

    // If screen exists but widgets not initialized (after splash), create UI
    if (screen != NULL && !widgets_initialized) {
        // Clear splash screen if it exists
        lv_obj_clean(screen);
        create_normal_ui(screen);
        widgets_initialized = true;
        return screen;
    }

    // Create new screen
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Show splash for first 5 seconds
    if (k_uptime_get() < 5000) {
        lv_obj_t *splash_label = lv_label_create(screen);
        lv_label_set_text(splash_label, "STRYKE BOOTING...");
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);
        widgets_initialized = false;
    } else {
        create_normal_ui(screen);
        widgets_initialized = true;
    }

    return screen;
}