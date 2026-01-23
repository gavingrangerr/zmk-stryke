#include <zmk/display/status_screen.h>
#include "widgets/stryke_display.h"

static struct stryke_display_widget stryke_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    stryke_display_init(&stryke_widget, screen);
    return screen;
}