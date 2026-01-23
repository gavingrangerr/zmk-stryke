#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

// Keycode to string mapping
const char* get_key_name(uint16_t keycode);
const char* get_modifier_name(uint8_t modifier);

void widget_init(void);