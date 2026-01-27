#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <lvgl.h>

static lv_obj_t *screen = NULL;
struct zmk_widget_layer_status layer_status_widget;
struct zmk_widget_battery_status battery_status_widget;

static struct k_work_delayable splash_work;

static void create_normal_ui() {
    if (screen == NULL) return;
    
    lv_obj_clean(screen); 

    // Battery widget (Top Left)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    // Layer widget (Top Right)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    // Center Title - NEXUS PRO
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "STRYKE");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN); 
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -5);

    // Bottom Subtitle
    lv_obj_t *status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "NEXUS PRO");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static void splash_expiry_function(struct k_work *work) {
    create_normal_ui();
}

lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        // Force the screen to match SSD1306 dimensions
        lv_obj_set_size(screen, 128, 64);
        lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

        // Splash Label
        lv_obj_t *splash_label = lv_label_create(screen);
        lv_label_set_text(splash_label, "STRYKE BOOTING...");
        lv_obj_set_style_text_color(splash_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);

        k_work_init_delayable(&splash_work, splash_expiry_function);
        k_work_schedule(&splash_work, K_MSEC(5000));
    }
    return screen;
}