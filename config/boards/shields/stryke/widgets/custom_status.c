#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

static lv_obj_t *screen = NULL;
static struct k_work_delayable splash_work;

/**
 * create_normal_ui - This function draws your final screen layout.
 */
static void create_normal_ui(lv_obj_t *parent) {
    if (parent == NULL) return;
    
    // Clear the "Booting" text
    lv_obj_clean(parent); 

    // Main Title: NexusPro
    lv_obj_t *title_label = lv_label_create(parent);
    lv_label_set_text(title_label, "NexusPro");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN); 
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN); // Added white color
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -5);

    // Subtitle: STRYKE
    lv_obj_t *status_label = lv_label_create(parent);
    lv_label_set_text(status_label, "STRYKE MODULE");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(status_label, lv_color_white(), LV_PART_MAIN); // Added white color
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // *** CRITICAL: Force LVGL to refresh the display ***
    lv_refr_now(NULL);
}

/**
 * splash_expiry_function - Callback for the timer to switch screens.
 */
static void splash_expiry_function(struct k_work *work) {
    if (screen != NULL) {
        create_normal_ui(screen);
    }
}

/**
 * zmk_display_status_screen - This is the entry point ZMK calls to boot the screen.
 */
lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        
        // Force the screen to be black
        lv_obj_set_size(screen, 128, 64);
        lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

        // Initial Splash Screen
        lv_obj_t *splash_label = lv_label_create(screen);
        lv_label_set_text(splash_label, "BOOTING...");
        lv_obj_set_style_text_color(splash_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);

        // Schedule the transition
        k_work_init_delayable(&splash_work, splash_expiry_function);
        k_work_schedule(&splash_work, K_MSEC(3000));
    }
    return screen;
}

#ifdef __cplusplus
}
#endif
```

## Key Changes:

1. **Added `lv_refr_now(NULL)`** - Forces an immediate LVGL display refresh
2. **Added white text color** - Your labels might be rendering as black-on-black
3. **Pass `screen` as parameter** - More explicit about which object we're modifying

## Alternative: If the above doesn't work

The display might be going to sleep. Try adding this to your `.conf` file:
```
CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y
CONFIG_ZMK_IDLE_TIMEOUT=300000
```

Or if you want to force the display to stay on:
```
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_IDLE_TIMEOUT=0