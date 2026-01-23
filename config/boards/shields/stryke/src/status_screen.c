#include <zmk/display/status_screen.h>
#include <lvgl.h>

// Global pointers so the listeners can find them later
static lv_obj_t *screen;

lv_obj_t *zmk_display_status_screen() {
    screen = lv_obj_create(NULL);

    // 1. Create a background (solid black)
    lv_obj_t *bg = lv_obj_create(screen);
    lv_obj_set_size(bg, 128, 64);
    lv_obj_set_style_bg_color(bg, lv_color_black(), 0);

    // 2. Create a very obvious test label
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "STRYKE ACTIVE");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 3. Return the screen object
    return screen;
}