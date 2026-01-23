#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include "stryke_display.h"

static struct stryke_display_widget stryke_widget;

static int status_screen_init(lv_obj_t *screen) {
    return stryke_display_init(&stryke_widget, screen);
}

static void status_screen_update() {
    // Updates every second if needed
}

ZMK_DISPLAY_STATUS_SCREEN_CUSTOM(status_screen, NULL, status_screen_init, status_screen_update);