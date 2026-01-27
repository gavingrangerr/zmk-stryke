#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

static lv_obj_t *screen = NULL;
static struct k_work_delayable splash_work;

static void show_main_ui() {
    if (screen == NULL) return;
    
    lv_obj_clean(screen); // Wipe "BOOTING..."

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "NexusPro");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -5);

    lv_obj_t *sub = lv_label_create(screen);
    lv_label_set_text(sub, "STRYKE MODULE");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Force LVGL to re-draw the screen now
    lv_refr_now(NULL); 
}

static void splash_expiry(struct k_work *work) {
    show_main_ui();
}

lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        lv_obj_set_size(screen, 128, 64);
        lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

        lv_obj_t *boot_text = lv_label_create(screen);
        lv_label_set_text(boot_text, "BOOTING...");
        lv_obj_align(boot_text, LV_ALIGN_CENTER, 0, 0);

        k_work_init_delayable(&splash_work, splash_expiry);
        k_work_schedule(&splash_work, K_MSEC(2000));
    }
    return screen;
}

#ifdef __cplusplus
}
#endif