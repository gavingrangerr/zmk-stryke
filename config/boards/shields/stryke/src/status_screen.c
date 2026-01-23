#include <zmk/display/status_screen.h>
#include <lvgl.h>

// Note: Keep your existing listeners/subscriptions below this function
lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Force a simple text label to prove the screen works
    lv_obj_t *hello_label = lv_label_create(screen);
    lv_label_set_text(hello_label, "NexusPro Active");
    lv_obj_align(hello_label, LV_ALIGN_CENTER, 0, 0);

    // Create your actual status labels
    layer_label = lv_label_create(screen);
    lv_obj_align(layer_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(layer_label, "Layer: Base");

    return screen;
}