#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include "widget.h"

static int zmk_widget_keyboard_status_init(void) {
    widget_init();
    return 0;
}

SYS_INIT(zmk_widget_keyboard_status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);