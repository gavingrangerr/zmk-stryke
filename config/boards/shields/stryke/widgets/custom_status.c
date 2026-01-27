#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <lvgl.h>

/* * ZMK mixes C and C++. The 'extern "C"' block ensures the linker 
 * finds the zmk_display_status_screen function correctly.
 */
#ifdef __cplusplus
extern "C" {
#endif

static lv_obj_t *screen = NULL;
static struct k_work_delayable splash_work;

/**
 * create_normal_ui - This function draws your final screen layout.
 */
static void create_normal_ui() {
    if (screen == NULL) return;
    
    // Clear the "Booting" text
    lv_obj_clean(screen); 

    // Main Title: NexusPro
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "NexusPro");
    // Ensure CONFIG_LV_FONT_MONTSERRAT_16=y is in your .conf
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN); 
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -5);

    // Subtitle: STRYKE
    lv_obj_t *status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "STRYKE MODULE");
    // Ensure CONFIG_LV_FONT_MONTSERRAT_10=y is in your .conf
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * splash_expiry_function - Callback for the timer to switch screens.
 */
static void splash_expiry_function(struct k_work *work) {
    create_normal_ui();
}

/**
 * zmk_display_status_screen - This is the entry point ZMK calls to boot the screen.
 */
lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        
        // --- 1. Fix for "Fuzzy" Screen ---
        // Force the screen to be black to clear the power-on noise (static).
        lv_obj_set_size(screen, 128, 64);
        lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

        // --- 2. Initial Splash Screen ---
        lv_obj_t *splash_label = lv_label_create(screen);
        lv_label_set_text(splash_label, "BOOTING...");
        lv_obj_set_style_text_color(splash_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);

        // Schedule the transition to the main NexusPro UI after 3 seconds
        k_work_init_delayable(&splash_work, splash_expiry_function);
        k_work_schedule(&splash_work, K_MSEC(3000));
    }
    return screen;
}

#ifdef __cplusplus
}
#endif