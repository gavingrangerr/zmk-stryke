#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keycode.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "stryke_display.h"

static struct stryke_display_widget *main_widget = NULL;
static const char *layer_names[] = {"BASE", "KICAD", "FUSION", "VSCODE", "ARDUINO"};
static uint8_t current_layer = 0;
static char last_key[24] = "—";
static uint32_t last_key_time = 0;
static bool key_pressed = false;

// Track modifiers for combo display
static bool ctrl_pressed = false;
static bool shift_pressed = false;
static bool alt_pressed = false;
static bool gui_pressed = false;

// Calculate best text size for key display
static int find_best_text_size(const char *text, int max_width) {
    bool is_combo = strchr(text, '+') != NULL;
    int len = strlen(text);
    
    // Try sizes from 4 down to 1
    for (int size = 4; size >= 1; size--) {
        int text_height = size * 8;
        
        if (text_height > MAX_TEXT_HEIGHT) {
            continue;
        }
        
        int approx_width;
        if (is_combo) {
            // Estimate combo width
            approx_width = 0;
            const char *ptr = text;
            while (*ptr) {
                if (*ptr == '+') {
                    approx_width += size * 6; // Plus sign
                } else {
                    approx_width += size * 6; // Character
                }
                ptr++;
            }
        } else {
            approx_width = len * (size * 6);
        }
        
        if (approx_width <= max_width) {
            return size;
        }
    }
    
    return 1;
}

// Convert ZMK keycode to display name
static const char* keycode_to_display_name(uint16_t keycode) {
    // Handle layer toggles
    if (keycode >= ZMK_KEYCODE_TO(0) && keycode <= ZMK_KEYCODE_TO(31)) {
        uint8_t layer = keycode - ZMK_KEYCODE_TO(0);
        static char layer_buf[8];
        snprintf(layer_buf, sizeof(layer_buf), "TO(%d)", layer);
        return layer_buf;
    }
    
    if (keycode >= ZMK_KEYCODE_TG(0) && keycode <= ZMK_KEYCODE_TG(31)) {
        uint8_t layer = keycode - ZMK_KEYCODE_TG(0);
        static char layer_buf[8];
        snprintf(layer_buf, sizeof(layer_buf), "TG(%d)", layer);
        return layer_buf;
    }
    
    if (keycode >= ZMK_KEYCODE_MO(0) && keycode <= ZMK_KEYCODE_MO(31)) {
        uint8_t layer = keycode - ZMK_KEYCODE_MO(0);
        static char layer_buf[8];
        snprintf(layer_buf, sizeof(layer_buf), "MO(%d)", layer);
        return layer_buf;
    }
    
    // Handle special behaviors
    switch (keycode) {
        case ZMK_KEYCODE_BOOTLOADER: return "BOOT";
        case ZMK_KEYCODE_SYSTEM_RESET: return "RESET";
        case ZMK_KEYCODE_CLEAR_BONDS: return "CLEAR";
        case ZMK_KEYCODE_NEXT_BT_PROFILE: return "NEXT";
        
        // Modifiers (these are handled separately for combos)
        case ZMK_KEYCODE_LEFT_CTRL:
        case ZMK_KEYCODE_RIGHT_CTRL:
            return "CTRL";
            
        case ZMK_KEYCODE_LEFT_SHIFT:
        case ZMK_KEYCODE_RIGHT_SHIFT:
            return "SHFT";
            
        case ZMK_KEYCODE_LEFT_ALT:
        case ZMK_KEYCODE_RIGHT_ALT:
            return "ALT";
            
        case ZMK_KEYCODE_LEFT_GUI:
        case ZMK_KEYCODE_RIGHT_GUI:
            return "CMD";
        
        // Letters
        case ZMK_KEYCODE_A: return "A";
        case ZMK_KEYCODE_B: return "B";
        case ZMK_KEYCODE_C: return "C";
        case ZMK_KEYCODE_D: return "D";
        case ZMK_KEYCODE_E: return "E";
        case ZMK_KEYCODE_F: return "F";
        case ZMK_KEYCODE_G: return "G";
        case ZMK_KEYCODE_H: return "H";
        case ZMK_KEYCODE_I: return "I";
        case ZMK_KEYCODE_J: return "J";
        case ZMK_KEYCODE_K: return "K";
        case ZMK_KEYCODE_L: return "L";
        case ZMK_KEYCODE_M: return "M";
        case ZMK_KEYCODE_N: return "N";
        case ZMK_KEYCODE_O: return "O";
        case ZMK_KEYCODE_P: return "P";
        case ZMK_KEYCODE_Q: return "Q";
        case ZMK_KEYCODE_R: return "R";
        case ZMK_KEYCODE_S: return "S";
        case ZMK_KEYCODE_T: return "T";
        case ZMK_KEYCODE_U: return "U";
        case ZMK_KEYCODE_V: return "V";
        case ZMK_KEYCODE_W: return "W";
        case ZMK_KEYCODE_X: return "X";
        case ZMK_KEYCODE_Y: return "Y";
        case ZMK_KEYCODE_Z: return "Z";
        
        // Numbers
        case ZMK_KEYCODE_N0: return "0";
        case ZMK_KEYCODE_N1: return "1";
        case ZMK_KEYCODE_N2: return "2";
        case ZMK_KEYCODE_N3: return "3";
        case ZMK_KEYCODE_N4: return "4";
        case ZMK_KEYCODE_N5: return "5";
        case ZMK_KEYCODE_N6: return "6";
        case ZMK_KEYCODE_N7: return "7";
        case ZMK_KEYCODE_N8: return "8";
        case ZMK_KEYCODE_N9: return "9";
        
        // Special keys
        case ZMK_KEYCODE_SPACE: return "SPC";
        case ZMK_KEYCODE_ENTER: return "ENT";
        case ZMK_KEYCODE_ESCAPE: return "ESC";
        case ZMK_KEYCODE_TAB: return "TAB";
        case ZMK_KEYCODE_BACKSPACE: return "BSPC";
        case ZMK_KEYCODE_DELETE: return "DEL";
        case ZMK_KEYCODE_INSERT: return "INS";
        case ZMK_KEYCODE_HOME: return "HOME";
        case ZMK_KEYCODE_END: return "END";
        case ZMK_KEYCODE_PAGE_UP: return "PGUP";
        case ZMK_KEYCODE_PAGE_DOWN: return "PGDN";
        case ZMK_KEYCODE_FSLH: return "/";
        case ZMK_KEYCODE_SQT: return "'";
        
        // Navigation
        case ZMK_KEYCODE_UP: return "UP";
        case ZMK_KEYCODE_DOWN: return "DN";
        case ZMK_KEYCODE_LEFT: return "LFT";
        case ZMK_KEYCODE_RIGHT: return "RGT";
        
        // Function keys
        case ZMK_KEYCODE_F1: return "F1";
        case ZMK_KEYCODE_F2: return "F2";
        case ZMK_KEYCODE_F3: return "F3";
        case ZMK_KEYCODE_F4: return "F4";
        case ZMK_KEYCODE_F5: return "F5";
        case ZMK_KEYCODE_F6: return "F6";
        case ZMK_KEYCODE_F7: return "F7";
        case ZMK_KEYCODE_F8: return "F8";
        case ZMK_KEYCODE_F9: return "F9";
        case ZMK_KEYCODE_F10: return "F10";
        case ZMK_KEYCODE_F11: return "F11";
        case ZMK_KEYCODE_F12: return "F12";
        
        default: {
            static char hex_buf[8];
            snprintf(hex_buf, sizeof(hex_buf), "0x%02X", keycode);
            return hex_buf;
        }
    }
}

// Build combo string from active modifiers and key
static void build_combo_string(uint16_t keycode) {
    const char *key_name = keycode_to_display_name(keycode);
    
    // Check if this is a modifier key itself
    bool is_modifier = (keycode == ZMK_KEYCODE_LEFT_CTRL || keycode == ZMK_KEYCODE_RIGHT_CTRL ||
                       keycode == ZMK_KEYCODE_LEFT_SHIFT || keycode == ZMK_KEYCODE_RIGHT_SHIFT ||
                       keycode == ZMK_KEYCODE_LEFT_ALT || keycode == ZMK_KEYCODE_RIGHT_ALT ||
                       keycode == ZMK_KEYCODE_LEFT_GUI || keycode == ZMK_KEYCODE_RIGHT_GUI);
    
    // Count active modifiers (excluding the current key if it's a modifier)
    int mod_count = 0;
    if (ctrl_pressed && !(keycode == ZMK_KEYCODE_LEFT_CTRL || keycode == ZMK_KEYCODE_RIGHT_CTRL)) mod_count++;
    if (shift_pressed && !(keycode == ZMK_KEYCODE_LEFT_SHIFT || keycode == ZMK_KEYCODE_RIGHT_SHIFT)) mod_count++;
    if (alt_pressed && !(keycode == ZMK_KEYCODE_LEFT_ALT || keycode == ZMK_KEYCODE_RIGHT_ALT)) mod_count++;
    if (gui_pressed && !(keycode == ZMK_KEYCODE_LEFT_GUI || keycode == ZMK_KEYCODE_RIGHT_GUI)) mod_count++;
    
    // If we have modifiers and the key isn't itself a modifier, create combo
    if (mod_count > 0 && !is_modifier) {
        char combo[32] = "";
        bool first = true;
        
        if (gui_pressed) {
            strcat(combo, "CMD");
            first = false;
        }
        if (alt_pressed) {
            if (!first) strcat(combo, "+");
            strcat(combo, "ALT");
            first = false;
        }
        if (shift_pressed) {
            if (!first) strcat(combo, "+");
            strcat(combo, "SHFT");
            first = false;
        }
        if (ctrl_pressed) {
            if (!first) strcat(combo, "+");
            strcat(combo, "CTRL");
            first = false;
        }
        
        // Add the actual key
        if (!first) strcat(combo, "+");
        strcat(combo, key_name);
        
        strncpy(last_key, combo, sizeof(last_key) - 1);
        last_key[sizeof(last_key) - 1] = '\0';
    } else {
        // Just show the key name
        strncpy(last_key, key_name, sizeof(last_key) - 1);
        last_key[sizeof(last_key) - 1] = '\0';
    }
}

// Update the visual key display
static void update_key_display() {
    if (!main_widget || !main_widget->key_label) return;
    
    // Set the text
    lv_label_set_text(main_widget->key_label, last_key);
    
    // Calculate best font size
    int max_width = 116;
    int best_size = find_best_text_size(last_key, max_width);
    
    // Set appropriate font
    const lv_font_t *font;
    switch (best_size) {
        case 4: font = &lv_font_montserrat_32; break;
        case 3: font = &lv_font_montserrat_24; break;
        case 2: font = &lv_font_montserrat_16; break;
        default: font = &lv_font_montserrat_12; break;
    }
    
    lv_obj_set_style_text_font(main_widget->key_label, font, 0);
    
    // Center the text
    lv_obj_center(main_widget->key_label);
    
    // Add press animation if recent
    uint32_t now = k_uptime_get_32();
    if ((now - last_key_time) < 200) {
        lv_obj_set_style_bg_color(main_widget->key_label, lv_color_white(), 0);
        lv_obj_set_style_text_color(main_widget->key_label, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(main_widget->key_label, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(main_widget->key_label, 4, 0);
        lv_obj_set_style_pad_all(main_widget->key_label, 4, 0);
    } else {
        lv_obj_set_style_bg_opa(main_widget->key_label, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(main_widget->key_label, lv_color_white(), 0);
    }
}

// Update layer indicators
static void update_layer_indicators() {
    if (!main_widget) return;
    
    for (int i = 0; i < 5; i++) {
        if (main_widget->layer_indicators[i]) {
            if (i == current_layer) {
                // Active layer - filled rectangle
                lv_obj_set_style_bg_color(main_widget->layer_indicators[i], lv_color_white(), 0);
                lv_obj_set_style_bg_opa(main_widget->layer_indicators[i], LV_OPA_COVER, 0);
                lv_obj_set_style_text_color(main_widget->layer_labels[i], lv_color_black(), 0);
            } else {
                // Inactive layer - outline only
                lv_obj_set_style_bg_opa(main_widget->layer_indicators[i], LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_color(main_widget->layer_indicators[i], lv_color_white(), 0);
                lv_obj_set_style_border_width(main_widget->layer_indicators[i], 1, 0);
                lv_obj_set_style_text_color(main_widget->layer_labels[i], lv_color_white(), 0);
            }
        }
    }
}

/* ---------- LAYER UPDATE LOGIC ---------- */
static int layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev) {
        current_layer = ev->layer;
        if (main_widget) {
            update_layer_indicators();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(stryke_layer_listener, layer_listener);
ZMK_SUBSCRIPTION(stryke_layer_listener, zmk_layer_state_changed);

/* ---------- KEYCODE UPDATE LOGIC ---------- */
static int key_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    if (ev && main_widget != NULL) {
        // Update modifier states
        switch (ev->keycode) {
            case ZMK_KEYCODE_LEFT_CTRL:
            case ZMK_KEYCODE_RIGHT_CTRL:
                ctrl_pressed = ev->state;
                break;
            case ZMK_KEYCODE_LEFT_SHIFT:
            case ZMK_KEYCODE_RIGHT_SHIFT:
                shift_pressed = ev->state;
                break;
            case ZMK_KEYCODE_LEFT_ALT:
            case ZMK_KEYCODE_RIGHT_ALT:
                alt_pressed = ev->state;
                break;
            case ZMK_KEYCODE_LEFT_GUI:
            case ZMK_KEYCODE_RIGHT_GUI:
                gui_pressed = ev->state;
                break;
        }
        
        // Only update display on key press (not release)
        if (ev->state) {
            last_key_time = k_uptime_get_32();
            build_combo_string(ev->keycode);
            update_key_display();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(stryke_key_listener, key_listener);
ZMK_SUBSCRIPTION(stryke_key_listener, zmk_keycode_state_changed);

/* ---------- WIDGET INITIALIZATION ---------- */
int stryke_display_init(struct stryke_display_widget *widget, lv_obj_t *parent) {
    // Main container
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(widget->obj, lv_color_black(), 0);

    // Key display card (like Arduino code)
    widget->key_display = lv_obj_create(widget->obj);
    lv_obj_set_size(widget->key_display, 124, CARD_HEIGHT);
    lv_obj_align(widget->key_display, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_radius(widget->key_display, 8, 0);
    lv_obj_set_style_border_color(widget->key_display, lv_color_white(), 0);
    lv_obj_set_style_border_width(widget->key_display, 2, 0);
    lv_obj_set_style_bg_color(widget->key_display, lv_color_black(), 0);
    
    // Key label inside the card
    widget->key_label = lv_label_create(widget->key_display);
    lv_label_set_text(widget->key_label, "—");
    lv_obj_set_style_text_color(widget->key_label, lv_color_white(), 0);
    lv_obj_center(widget->key_label);

    // Create layer indicators (L0, L1, L2, L3, L4)
    int indicator_spacing = 25;
    int start_x = 4;
    
    for (int i = 0; i < 5; i++) {
        // Layer indicator box
        widget->layer_indicators[i] = lv_obj_create(widget->obj);
        lv_obj_set_size(widget->layer_indicators[i], 22, 12);
        lv_obj_set_pos(widget->layer_indicators[i], start_x + (i * indicator_spacing), 46);
        lv_obj_set_style_radius(widget->layer_indicators[i], 2, 0);
        lv_obj_set_style_border_color(widget->layer_indicators[i], lv_color_white(), 0);
        lv_obj_set_style_border_width(widget->layer_indicators[i], 1, 0);
        
        // Layer label (L0, L1, etc.)
        widget->layer_labels[i] = lv_label_create(widget->layer_indicators[i]);
        char layer_text[4];
        snprintf(layer_text, sizeof(layer_text), "L%d", i);
        lv_label_set_text(widget->layer_labels[i], layer_text);
        lv_obj_set_style_text_font(widget->layer_labels[i], &lv_font_montserrat_8, 0);
        lv_obj_center(widget->layer_labels[i]);
    }

    main_widget = widget;
    
    // Initialize display
    update_layer_indicators();
    update_key_display();
    
    return 0;
}

lv_obj_t *stryke_display_obj(struct stryke_display_widget *widget) {
    return widget->obj;
}

// Public API to update display
void stryke_display_update_layer(uint8_t layer) {
    current_layer = layer;
    if (main_widget) {
        update_layer_indicators();
    }
}

void stryke_display_update_key(uint16_t keycode, bool pressed, const char *key_name) {
    if (pressed && main_widget) {
        last_key_time = k_uptime_get_32();
        
        if (key_name) {
            strncpy(last_key, key_name, sizeof(last_key) - 1);
            last_key[sizeof(last_key) - 1] = '\0';
        } else {
            build_combo_string(keycode);
        }
        
        update_key_display();
    }
}