#include "widget.h"
#include "display.h"
#include <string.h>

// Simple keycode to name mapping (expand as needed)
static const struct {
    uint16_t keycode;
    const char *name;
} key_names[] = {
    {KC_A, "A"}, {KC_B, "B"}, {KC_C, "C"}, {KC_D, "D"}, {KC_E, "E"},
    {KC_F, "F"}, {KC_G, "G"}, {KC_H, "H"}, {KC_I, "I"}, {KC_J, "J"},
    {KC_K, "K"}, {KC_L, "L"}, {KC_M, "M"}, {KC_N, "N"}, {KC_O, "O"},
    {KC_P, "P"}, {KC_Q, "Q"}, {KC_R, "R"}, {KC_S, "S"}, {KC_T, "T"},
    {KC_U, "U"}, {KC_V, "V"}, {KC_W, "W"}, {KC_X, "X"}, {KC_Y, "Y"},
    {KC_Z, "Z"},
    {KC_SPACE, "SPC"}, {KC_ENTER, "ENT"}, {KC_ESCAPE, "ESC"}, {KC_TAB, "TAB"},
    {KC_DELETE, "DEL"}, {KC_BACKSPACE, "BSPC"},
    {KC_LEFT_CTRL, "CTRL"}, {KC_LEFT_SHIFT, "SHFT"}, {KC_LEFT_ALT, "ALT"},
    {KC_LEFT_GUI, "CMD"}, {KC_RIGHT_CTRL, "CTRL"}, {KC_RIGHT_SHIFT, "SHFT"},
    {KC_RIGHT_ALT, "ALT"}, {KC_RIGHT_GUI, "CMD"},
    {KC_UP, "UP"}, {KC_DOWN, "DN"}, {KC_LEFT, "LFT"}, {KC_RIGHT, "RGT"},
    {KC_F1, "F1"}, {KC_F2, "F2"}, {KC_F3, "F3"}, {KC_F4, "F4"}, {KC_F5, "F5"},
    {KC_F6, "F6"}, {KC_F7, "F7"}, {KC_F8, "F8"}, {KC_F9, "F9"}, {KC_F10, "F10"},
    {KC_F11, "F11"}, {KC_F12, "F12"},
    {KC_INSERT, "INS"}, {KC_HOME, "HOME"}, {KC_END, "END"},
    {KC_PAGE_UP, "PGUP"}, {KC_PAGE_DOWN, "PGDN"},
    {KC_PRINT_SCREEN, "PSCR"}, {KC_SCROLL_LOCK, "SCRL"}, {KC_PAUSE, "PAUSE"},
    {KC_NUMLOCK, "NUMLK"}, {KC_KP_SLASH, "/"}, {KC_KP_ASTERISK, "*"},
    {KC_KP_MINUS, "-"}, {KC_KP_PLUS, "+"}, {KC_KP_ENTER, "ENT"},
    {KC_KP_1, "1"}, {KC_KP_2, "2"}, {KC_KP_3, "3"}, {KC_KP_4, "4"},
    {KC_KP_5, "5"}, {KC_KP_6, "6"}, {KC_KP_7, "7"}, {KC_KP_8, "8"},
    {KC_KP_9, "9"}, {KC_KP_0, "0"}, {KC_KP_DOT, "."},
    {KC_MINUS, "-"}, {KC_EQUAL, "="}, {KC_LEFT_BRACKET, "["},
    {KC_RIGHT_BRACKET, "]"}, {KC_BACKSLASH, "\\"}, {KC_SEMICOLON, ";"},
    {KC_QUOTE, "'"}, {KC_COMMA, ","}, {KC_DOT, "."}, {KC_SLASH, "/"},
    {KC_GRAVE, "`"}, {KC_CAPS_LOCK, "CAPS"},
    {KC_1, "1"}, {KC_2, "2"}, {KC_3, "3"}, {KC_4, "4"}, {KC_5, "5"},
    {KC_6, "6"}, {KC_7, "7"}, {KC_8, "8"}, {KC_9, "9"}, {KC_0, "0"},
};

const char* get_key_name(uint16_t keycode) {
    for (size_t i = 0; i < sizeof(key_names) / sizeof(key_names[0]); i++) {
        if (key_names[i].keycode == keycode) {
            return key_names[i].name;
        }
    }
    return "KEY";
}

const char* get_modifier_name(uint8_t modifier) {
    switch (modifier) {
        case MOD_LCTL: return "CTRL";
        case MOD_LSFT: return "SHFT";
        case MOD_LALT: return "ALT";
        case MOD_LGUI: return "CMD";
        case MOD_RCTL: return "CTRL";
        case MOD_RSFT: return "SHFT";
        case MOD_RALT: return "ALT";
        case MOD_RGUI: return "CMD";
        default: return "";
    }
}

// State tracking
static uint8_t active_modifiers = 0;
static char key_buffer[32];

static void update_display_with_key(uint16_t keycode) {
    const char *key_name = get_key_name(keycode);
    
    if (active_modifiers) {
        // Build combination
        key_buffer[0] = '\0';
        bool first = true;
        
        if (active_modifiers & MOD_LCTL) {
            strcat(key_buffer, "CTRL");
            first = false;
        }
        if (active_modifiers & MOD_LSFT) {
            if (!first) strcat(key_buffer, "+");
            strcat(key_buffer, "SHFT");
            first = false;
        }
        if (active_modifiers & MOD_LALT) {
            if (!first) strcat(key_buffer, "+");
            strcat(key_buffer, "ALT");
            first = false;
        }
        if (active_modifiers & MOD_LGUI) {
            if (!first) strcat(key_buffer, "+");
            strcat(key_buffer, "CMD");
            first = false;
        }
        
        // Add the key
        strcat(key_buffer, "+");
        strcat(key_buffer, key_name);
        
        display_set_key(key_buffer);
        
        // Reset modifiers after combination
        active_modifiers = 0;
    } else {
        // Single key
        display_set_key(key_name);
    }
}

// Event handlers
static int widget_on_keycode_state_changed(const struct zmk_event_header *eh) {
    const struct zmk_keycode_state_changed *ev = cast_zmk_keycode_state_changed(eh);
    
    if (!ev->state) {
        return 0; // Only show on press
    }
    
    // Check if it's a modifier
    if (ev->keycode >= KC_LEFT_CTRL && ev->keycode <= KC_RIGHT_GUI) {
        // Update active modifiers
        switch (ev->keycode) {
            case KC_LEFT_CTRL:
            case KC_RIGHT_CTRL:
                active_modifiers |= MOD_LCTL;
                break;
            case KC_LEFT_SHIFT:
            case KC_RIGHT_SHIFT:
                active_modifiers |= MOD_LSFT;
                break;
            case KC_LEFT_ALT:
            case KC_RIGHT_ALT:
                active_modifiers |= MOD_LALT;
                break;
            case KC_LEFT_GUI:
            case KC_RIGHT_GUI:
                active_modifiers |= MOD_LGUI;
                break;
        }
        
        // Show modifier if no other key is pressed with it
        char mod_buffer[16];
        mod_buffer[0] = '\0';
        
        if (active_modifiers & MOD_LCTL) strcat(mod_buffer, "CTRL");
        if (active_modifiers & MOD_LSFT) {
            if (strlen(mod_buffer) > 0) strcat(mod_buffer, "+");
            strcat(mod_buffer, "SHFT");
        }
        if (active_modifiers & MOD_LALT) {
            if (strlen(mod_buffer) > 0) strcat(mod_buffer, "+");
            strcat(mod_buffer, "ALT");
        }
        if (active_modifiers & MOD_LGUI) {
            if (strlen(mod_buffer) > 0) strcat(mod_buffer, "+");
            strcat(mod_buffer, "CMD");
        }
        
        display_set_key(mod_buffer);
    } else {
        // Regular key
        update_display_with_key(ev->keycode);
    }
    
    return 0;
}

static int widget_on_layer_state_changed(const struct zmk_event_header *eh) {
    const struct zmk_layer_state_changed *ev = cast_zmk_layer_state_changed(eh);
    
    display_set_layer(ev->layer);
    return 0;
}

// Subscribe to events
ZMK_LISTENER(widget_keyboard_status, widget_on_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_keyboard_status, zmk_keycode_state_changed);

ZMK_LISTENER(widget_layer_status, widget_on_layer_state_changed);
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

void widget_init(void) {
    if (display_init() != 0) {
        printk("Failed to initialize display\n");
        return;
    }
    
    printk("Keyboard status widget initialized\n");
}