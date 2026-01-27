#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screen configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CARD_HEIGHT 36
#define MAX_TEXT_HEIGHT 16

// UI state
static lv_obj_t *screen = NULL;
static lv_obj_t *key_card = NULL;
static lv_obj_t *key_label = NULL;
static lv_obj_t *layer_indicators[5];

static uint8_t current_layer = 0;
static char last_key_text[32] = "---";
static int64_t last_key_time = 0;

// Modifier state tracking
static bool lgui_active = false;
static bool lshift_active = false;
static bool lctrl_active = false;
static bool lalt_active = false;

// Key code to string mapping (simplified - extend as needed)
static const char* get_key_name(uint32_t keycode) {
    // HID usage codes to key names
    switch(keycode) {
        case 0x04: return "A";
        case 0x05: return "B";
        case 0x06: return "C";
        case 0x07: return "D";
        case 0x08: return "E";
        case 0x09: return "F";
        case 0x0A: return "G";
        case 0x0B: return "H";
        case 0x0C: return "I";
        case 0x0D: return "J";
        case 0x0E: return "K";
        case 0x0F: return "L";
        case 0x10: return "M";
        case 0x11: return "N";
        case 0x12: return "O";
        case 0x13: return "P";
        case 0x14: return "Q";
        case 0x15: return "R";
        case 0x16: return "S";
        case 0x17: return "T";
        case 0x18: return "U";
        case 0x19: return "V";
        case 0x1A: return "W";
        case 0x1B: return "X";
        case 0x1C: return "Y";
        case 0x1D: return "Z";
        case 0x1E: return "1";
        case 0x1F: return "2";
        case 0x20: return "3";
        case 0x21: return "4";
        case 0x22: return "5";
        case 0x23: return "6";
        case 0x24: return "7";
        case 0x25: return "8";
        case 0x26: return "9";
        case 0x27: return "0";
        case 0x28: return "ENT";
        case 0x29: return "ESC";
        case 0x2A: return "BSPC";
        case 0x2B: return "TAB";
        case 0x2C: return "SPC";
        case 0x2D: return "-";
        case 0x2E: return "=";
        case 0x2F: return "[";
        case 0x30: return "]";
        case 0x31: return "\\";
        case 0x33: return ";";
        case 0x34: return "'";
        case 0x35: return "`";
        case 0x36: return ",";
        case 0x37: return ".";
        case 0x38: return "/";
        case 0x39: return "CAPS";
        case 0x3A: return "F1";
        case 0x3B: return "F2";
        case 0x3C: return "F3";
        case 0x3D: return "F4";
        case 0x3E: return "F5";
        case 0x3F: return "F6";
        case 0x40: return "F7";
        case 0x41: return "F8";
        case 0x42: return "F9";
        case 0x43: return "F10";
        case 0x44: return "F11";
        case 0x45: return "F12";
        case 0x49: return "INS";
        case 0x4A: return "HOME";
        case 0x4B: return "PGUP";
        case 0x4C: return "DEL";
        case 0x4D: return "END";
        case 0x4E: return "PGDN";
        case 0x4F: return "RGT";
        case 0x50: return "LFT";
        case 0x51: return "DN";
        case 0x52: return "UP";
        case 0xE0: return "CTL";
        case 0xE1: return "SFT";
        case 0xE2: return "ALT";
        case 0xE3: return "GUI";
        case 0xE4: return "CTL";
        case 0xE5: return "SFT";
        case 0xE6: return "ALT";
        case 0xE7: return "GUI";
        default: return NULL;
    }
}

// Build display string with modifiers
static void build_key_display(uint32_t keycode) {
    const char* key_name = get_key_name(keycode);
    
    // Don't display if key name not found
    if (key_name == NULL) {
        return;
    }
    
    // Reset buffer
    last_key_text[0] = '\0';
    
    // Build modifier string
    if (lgui_active) {
        strcat(last_key_text, "CMD+");
    }
    if (lshift_active) {
        strcat(last_key_text, "SFT+");
    }
    if (lctrl_active) {
        strcat(last_key_text, "CTL+");
    }
    if (lalt_active) {
        strcat(last_key_text, "ALT+");
    }
    
    // Add the key name
    strcat(last_key_text, key_name);
    
    // Update timestamp
    last_key_time = k_uptime_get();
}

// Find best text size that fits within constraints
static int find_best_text_size(const char* text, int max_width, int max_height) {
    int text_len = strlen(text);
    
    // Try sizes from 3 down to 1 (LVGL font sizes)
    for (int size = 3; size >= 1; size--) {
        int char_width = (size == 3) ? 16 : (size == 2) ? 12 : 8;
        int char_height = (size == 3) ? 24 : (size == 2) ? 16 : 8;
        
        int text_width = text_len * char_width;
        int text_height = char_height;
        
        if (text_height <= max_height && text_width <= max_width) {
            return size;
        }
    }
    
    return 1; // Smallest size
}

// Get LVGL font for size
static const lv_font_t* get_font_for_size(int size) {
    if (size >= 3) {
        return &lv_font_montserrat_20;
    } else if (size == 2) {
        return &lv_font_montserrat_16;
    } else {
        return &lv_font_montserrat_10;
    }
}

// Update the key display card
static void update_key_display(void) {
    if (key_label == NULL) return;
    
    // Update the label text
    lv_label_set_text(key_label, last_key_text);
    
    // Find best text size
    int best_size = find_best_text_size(last_key_text, 116, MAX_TEXT_HEIGHT);
    const lv_font_t* font = get_font_for_size(best_size);
    lv_obj_set_style_text_font(key_label, font, LV_PART_MAIN);
    
    // Check if key press is fresh (within 200ms for press animation)
    int64_t now = k_uptime_get();
    bool fresh_press = (now - last_key_time) < 200;
    
    if (fresh_press) {
        // Inverted colors for fresh press
        lv_obj_set_style_bg_color(key_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(key_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_black(), LV_PART_MAIN);
    } else {
        // Normal colors
        lv_obj_set_style_bg_opa(key_label, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
    }
    
    // Center the label
    lv_obj_center(key_label);
}

// Update layer indicators
static void update_layer_indicators(void) {
    for (int i = 0; i < 5; i++) {
        if (layer_indicators[i] == NULL) continue;
        
        lv_obj_t* label = lv_obj_get_child(layer_indicators[i], 0);
        
        if (i == current_layer) {
            // Active layer - filled
            lv_obj_set_style_bg_color(layer_indicators[i], lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
        } else {
            // Inactive layer - outline only
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        }
    }
}

// Create the main UI
static void create_ui(void) {
    if (screen == NULL) return;
    
    // Clear screen
    lv_obj_clean(screen);
    
    // Set black background
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // === KEY DISPLAY CARD ===
    key_card = lv_obj_create(screen);
    lv_obj_set_size(key_card, 124, CARD_HEIGHT);
    lv_obj_set_pos(key_card, 2, 2);
    lv_obj_set_style_bg_opa(key_card, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(key_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(key_card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(key_card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(key_card, 0, LV_PART_MAIN);
    
    // Key label inside card
    key_label = lv_label_create(key_card);
    lv_label_set_text(key_label, last_key_text);
    lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(key_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(key_label);
    
    // === LAYER INDICATORS ===
    int indicator_y = 46;
    int indicator_spacing = 25;
    int start_x = 4;
    
    for (int i = 0; i < 5; i++) {
        int x = start_x + (i * indicator_spacing);
        
        // Create indicator container
        layer_indicators[i] = lv_obj_create(screen);
        lv_obj_set_size(layer_indicators[i], 22, 12);
        lv_obj_set_pos(layer_indicators[i], x, indicator_y);
        lv_obj_set_style_radius(layer_indicators[i], 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(layer_indicators[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_width(layer_indicators[i], 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(layer_indicators[i], 0, LV_PART_MAIN);
        
        // Create label inside indicator
        lv_obj_t* label = lv_label_create(layer_indicators[i]);
        char layer_text[4];
        snprintf(layer_text, sizeof(layer_text), "L%d", i);
        lv_label_set_text(label, layer_text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
    
    // Initial update
    update_layer_indicators();
    update_key_display();
}

// Animation timer callback
static void animation_timer_cb(lv_timer_t* timer) {
    update_key_display();
}

// Layer state change event handler
static int layer_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) return 0;
    
    current_layer = zmk_keymap_highest_layer_active();
    if (current_layer > 4) current_layer = 4; // Cap at 5 layers
    
    update_layer_indicators();
    
    return 0;
}

// Keycode state change event handler
static int keycode_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) return 0;
    
    uint32_t keycode = ev->keycode;
    bool pressed = ev->state;
    
    // Track modifier states
    if (keycode == 0xE0 || keycode == 0xE4) { // Left/Right Ctrl
        lctrl_active = pressed;
        return 0; // Don't display modifiers alone
    }
