#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

static lv_obj_t *screen = NULL;

lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_size(screen, 128, 64);
        lv_obj_t *label = lv_label_create(screen);
        lv_label_set_text(label, "NexusPro");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
    return screen;
}

#ifdef __cplusplus
}
#endif