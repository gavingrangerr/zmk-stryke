#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Configuration from devicetree
#define SCREEN_WIDTH DT_PROP(DT_NODELABEL(keyboard_status), width)
#define SCREEN_HEIGHT DT_PROP(DT_NODELABEL(keyboard_status), height)
#define OLED_ADDRESS DT_PROP(DT_NODELABEL(keyboard_status), reg)

// Font sizes
#define FONT_SMALL 1
#define FONT_MEDIUM 2
#define FONT_LARGE 3
#define FONT_XLARGE 4

// Key display constants
#define MAX_TEXT_HEIGHT 16
#define CARD_HEIGHT 36
#define MAX_KEY_LENGTH 32

struct display_data {
    const struct device *i2c_dev;
    uint8_t current_layer;
    char last_key[MAX_KEY_LENGTH];
    uint64_t last_key_time;
    bool initialized;
};

int display_init(void);
void display_update(void);
void display_set_key(const char *key);
void display_set_layer(uint8_t layer);