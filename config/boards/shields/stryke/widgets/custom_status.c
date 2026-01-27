#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/activity_state_changed.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

static lv_obj_t *screen = NULL;
static struct k_work_delayable splash_work;
static bool splash_done = false;

/**
 * create_normal_ui - This function draws your final screen layout.
 */
static void create_normal_ui(lv_obj_t *parent) {
    if (parent == NULL) return;
    
    // Clear everything
    lv_obj_clean(parent); 

    // Main Title: NexusPro
    lv_obj_t *title_label = lv_label_create(parent);
    lv_label_set_text(title_label, "NexusPro");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN); 
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -10);

    // Subtitle: STRYKE
    lv_obj_t *status_label = lv_label_create(parent);
    lv_label_set_text(status_label, "STRYKE MODULE");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(status_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 10);
    
    splash_done = true;
}

/**
 * splash_expiry_function - Callback for the timer to switch screens.
 */
static void splash_expiry_function(struct k_work *work) {
    if (screen != NULL && !splash_done) {
        create_normal_ui(screen);
    }
}

/**
 * zmk_display_status_screen - Entry point ZMK calls to boot the screen.
 */
lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        
        // Set screen size and background
        lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

        // Initial Splash Screen
        lv_obj_t *splash_label = lv_label_create(screen);
        lv_label_set_text(splash_label, "BOOTING...");
        lv_obj_set_style_text_color(splash_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);

        // Schedule the transition
        k_work_init_delayable(&splash_work, splash_expiry_function);
        k_work_schedule(&splash_work, K_MSEC(2000));
    }
    return screen;
}

#ifdef __cplusplus
}
#endif