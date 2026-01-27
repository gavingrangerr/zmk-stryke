#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/events/keycode_state_changed.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CARD_HEIGHT 36
#define MAX_TEXT_HEIGHT 16

static lv_obj_t *screen = NULL;
static lv_obj_t *key_card = NULL;
static lv_obj_t *key_label = NULL;
static lv_obj_t *layer_indicators[5];

static uint8_t current_layer = 0;
static char last_key_text[32] = "---";
static int64_t last_key_time = 0;

static struct k_work_delayable display_work;
static uint32_t pending_keycode = 0;

static const char* get_key_name(uint8_t keycode, bool shift) {
    if (shift) {
        switch(keycode) {
            case 0x1E: return "!";
            case 0x1F: return "@";
            case 0x20: return "#";
            case 0x21: return "$";
            case 0x22: return "%";
            case 0x23: return "^";
            case 0x24: return "&";
            case 0x25: return "*";
            case 0x26: return "(";
            case 0x27: return ")";
            case 0x2D: return "_";
            case 0x2E: return "+";
            case 0x2F: return "{";
            case 0x30: return "}";
            case 0x31: return "|";
            case 0x33: return ":";
            case 0x34: return "\"";
            case 0x35: return "~";
            case 0x36: return "<";
            case 0x37: return ">";
            case 0x38: return "?";
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
        }
    }
    
    switch(keycode) {
        case 0x04: return "a";
        case 0x05: return "b";
        case 0x06: return "c";
        case 0x07: return "d";
        case 0x08: return "e";
        case 0x09: return "f";
        case 0x0A: return "g";
        case 0x0B: return "h";
        case 0x0C: return "i";
        case 0x0D: return "j";
        case 0x0E: return "k";
        case 0x0F: return "l";
        case 0x10: return "m";
        case 0x11: return "n";
        case 0x12: return "o";
        case 0x13: return "p";
        case 0x14: return "q";
        case 0x15: return "r";
        case 0x16: return "s";
        case 0x17: return "t";
        case 0x18: return "u";
        case 0x19: return "v";
        case 0x1A: return "w";
        case 0x1B: return "x";
        case 0x1C: return "y";
        case 0x1D: return "z";
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
        default: return NULL;
    }
}

static int find_best_text_size(const char* text, int max_width, int max_height) {
    int text_len = strlen(text);
    
    for (int size = 3; size >= 1; size--) {
        int char_width = (size == 3) ? 16 : (size == 2) ? 12 : 8;
        int char_height = (size == 3) ? 24 : (size == 2) ? 16 : 8;
        
        int text_width = text_len * char_width;
        int text_height = char_height;
        
        if (text_height <= max_height && text_width <= max_width) {
            return size;
        }
    }
    
    return 1;
}

static const lv_font_t* get_font_for_size(int size) {
    if (size >= 3) {
        return &lv_font_montserrat_20;
    } else if (size == 2) {
        return &lv_font_montserrat_16;
    } else {
        return &lv_font_montserrat_10;
    }
}

static void update_key_display(void) {
    if (key_label == NULL) return;
    
    lv_label_set_text(key_label, last_key_text);
    
    int best_size = find_best_text_size(last_key_text, 116, MAX_TEXT_HEIGHT);
    const lv_font_t* font = get_font_for_size(best_size);
    lv_obj_set_style_text_font(key_label, font, LV_PART_MAIN);
    
    int64_t now = k_uptime_get();
    bool fresh_press = (now - last_key_time) < 200;
    
    if (fresh_press) {
        lv_obj_set_style_bg_color(key_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(key_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_black(), LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_opa(key_label, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
    }
    
    lv_obj_center(key_label);
    
    lv_obj_invalidate(key_label);
    lv_obj_invalidate(key_card);
    lv_obj_invalidate(screen);
}

static void update_layer_indicators(void) {
    for (int i = 0; i < 5; i++) {
        if (layer_indicators[i] == NULL) continue;
        
        lv_obj_t* label = lv_obj_get_child(layer_indicators[i], 0);
        
        if (i == current_layer) {
            lv_obj_set_style_bg_color(layer_indicators[i], lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        }
    }
}

static void display_work_handler(struct k_work *work) {
    if (pending_keycode == 0) return;
    
    last_key_text[0] = '\0';
    
    uint8_t modifiers = (pending_keycode >> 24) & 0xFF;
    uint8_t usage_page = (pending_keycode >> 16) & 0xFF;
    uint8_t base_keycode = pending_keycode & 0xFF;
    
    if (usage_page != 0x07) {
        pending_keycode = 0;
        return;
    }
    
    bool gui = (modifiers & 0x08) != 0;
    bool shift = (modifiers & 0x02) != 0;
    bool ctrl = (modifiers & 0x01) != 0;
    bool alt = (modifiers & 0x04) != 0;
    
    if (gui) {
        if (shift) {
            switch(base_keycode) {
                case 0x1D: strcpy(last_key_text, "CMD+SFT+Z"); break;
                case 0x20: strcpy(last_key_text, "CMD+SFT+3"); break;
                case 0x21: strcpy(last_key_text, "CMD+SFT+4"); break;
                case 0x13: strcpy(last_key_text, "CMD+SFT+P"); break;
                case 0x0F: strcpy(last_key_text, "CMD+SFT+L"); break;
                case 0x09: strcpy(last_key_text, "CMD+SFT+F"); break;
                case 0x08: strcpy(last_key_text, "CMD+SFT+E"); break;
                case 0x10: strcpy(last_key_text, "CMD+SFT+M"); break;
                default: {
                    const char* key_name = get_key_name(base_keycode, true);
                    if (key_name != NULL) {
                        strcpy(last_key_text, "CMD+SFT+");
                        strcat(last_key_text, key_name);
                    }
                    break;
                }
            }
        } else {
            switch(base_keycode) {
                case 0x06: strcpy(last_key_text, "CMD+C"); break;
                case 0x19: strcpy(last_key_text, "CMD+V"); break;
                case 0x1B: strcpy(last_key_text, "CMD+X"); break;
                case 0x1D: strcpy(last_key_text, "CMD+Z"); break;
                case 0x04: strcpy(last_key_text, "CMD+A"); break;
                case 0x16: strcpy(last_key_text, "CMD+S"); break;
                case 0x09: strcpy(last_key_text, "CMD+F"); break;
                case 0x0E: strcpy(last_key_text, "CMD+K"); break;
                case 0x1E: strcpy(last_key_text, "CMD+1"); break;
                case 0x1F: strcpy(last_key_text, "CMD+2"); break;
                case 0x27: strcpy(last_key_text, "CMD+0"); break;
                case 0x13: strcpy(last_key_text, "CMD+P"); break;
                case 0x05: strcpy(last_key_text, "CMD+B"); break;
                case 0x38: strcpy(last_key_text, "CMD+/"); break;
                case 0x07: strcpy(last_key_text, "CMD+D"); break;
                case 0x34: strcpy(last_key_text, "CMD+\'"); break;
                case 0x15: strcpy(last_key_text, "CMD+R"); break;
                case 0x18: strcpy(last_key_text, "CMD+U"); break;
                case 0x17: strcpy(last_key_text, "CMD+T"); break;
                case 0x0F: strcpy(last_key_text, "CMD+L"); break;
                default: {
                    const char* key_name = get_key_name(base_keycode, false);
                    if (key_name != NULL) {
                        strcpy(last_key_text, "CMD+");
                        strcat(last_key_text, key_name);
                    }
                    break;
                }
            }
        }
    } else if (shift && ctrl && alt && gui) {
        strcpy(last_key_text, "CTL+ALT+SFT+CMD+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (ctrl && alt && gui) {
        strcpy(last_key_text, "CTL+ALT+CMD+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (shift && alt && gui) {
        strcpy(last_key_text, "ALT+SFT+CMD+");
        const char* key_name = get_key_name(base_keycode, true);
        if (key_name) strcat(last_key_text, key_name);
    } else if (shift && ctrl && gui) {
        strcpy(last_key_text, "CTL+SFT+CMD+");
        const char* key_name = get_key_name(base_keycode, true);
        if (key_name) strcat(last_key_text, key_name);
    } else if (ctrl && alt) {
        strcpy(last_key_text, "CTL+ALT+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (shift && alt) {
        strcpy(last_key_text, "ALT+SFT+");
        const char* key_name = get_key_name(base_keycode, true);
        if (key_name) strcat(last_key_text, key_name);
    } else if (shift && ctrl) {
        strcpy(last_key_text, "CTL+SFT+");
        const char* key_name = get_key_name(base_keycode, true);
        if (key_name) strcat(last_key_text, key_name);
    } else if (alt && gui) {
        strcpy(last_key_text, "ALT+CMD+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (ctrl && gui) {
        strcpy(last_key_text, "CTL+CMD+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (shift && gui) {
        strcpy(last_key_text, "SFT+CMD+");
        const char* key_name = get_key_name(base_keycode, true);
        if (key_name) strcat(last_key_text, key_name);
    } else if (gui) {
        strcpy(last_key_text, "CMD+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (shift) {
        strcpy(last_key_text, "SFT+");
        const char* key_name = get_key_name(base_keycode, true);
        if (key_name) strcat(last_key_text, key_name);
    } else if (ctrl) {
        strcpy(last_key_text, "CTL+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else if (alt) {
        strcpy(last_key_text, "ALT+");
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) strcat(last_key_text, key_name);
    } else {
        const char* key_name = get_key_name(base_keycode, false);
        if (key_name) {
            strcpy(last_key_text, key_name);
        } else if (base_keycode != 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "0x%02X", base_keycode);
            strcpy(last_key_text, buf);
        }
    }
    
    last_key_time = k_uptime_get();
    update_key_display();
    pending_keycode = 0;
}

static void create_ui(void) {
    if (screen == NULL) return;
    
    lv_obj_clean(screen);
    
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    
    key_card = lv_obj_create(screen);
    lv_obj_set_size(key_card, 124, CARD_HEIGHT);
    lv_obj_set_pos(key_card, 2, 2);
    lv_obj_set_style_bg_opa(key_card, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(key_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(key_card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(key_card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(key_card, 0, LV_PART_MAIN);
    
    key_label = lv_label_create(key_card);
    lv_label_set_text(key_label, last_key_text);
    lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(key_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(key_label);
    
    int indicator_y = 46;
    int indicator_spacing = 25;
    int start_x = 4;
    
    for (int i = 0; i < 5; i++) {
        int x = start_x + (i * indicator_spacing);
        
        layer_indicators[i] = lv_obj_create(screen);
        lv_obj_set_size(layer_indicators[i], 22, 12);
        lv_obj_set_pos(layer_indicators[i], x, indicator_y);
        lv_obj_set_style_radius(layer_indicators[i], 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(layer_indicators[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_width(layer_indicators[i], 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(layer_indicators[i], 0, LV_PART_MAIN);
        
        lv_obj_t* label = lv_label_create(layer_indicators[i]);
        char layer_text[4];
        snprintf(layer_text, sizeof(layer_text), "L%d", i);
        lv_label_set_text(label, layer_text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
    
    update_layer_indicators();
    update_key_display();
}

static void animation_timer_cb(lv_timer_t* timer) {
    update_key_display();
}

static int layer_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) return 0;
    
    current_layer = zmk_keymap_highest_layer_active();
    if (current_layer > 4) current_layer = 4;
    
    update_layer_indicators();
    
    return 0;
}

static int keycode_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) return 0;
    
    uint32_t keycode = ev->keycode;
    bool pressed = ev->state;
    
    if (pressed) {
        pending_keycode = keycode;
        k_work_schedule(&display_work, K_MSEC(20));
    }
    
    return 0;
}

ZMK_LISTENER(custom_status_layer, layer_state_changed_cb);
ZMK_SUBSCRIPTION(custom_status_layer, zmk_layer_state_changed);

ZMK_LISTENER(custom_status_keycode, keycode_state_changed_cb);
ZMK_SUBSCRIPTION(custom_status_keycode, zmk_keycode_state_changed);

lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        lv_obj_set_size(screen, SCREEN_WIDTH, SCREEN_HEIGHT);
        
        k_work_init_delayable(&display_work, display_work_handler);
        
        create_ui();
        
        lv_timer_create(animation_timer_cb, 50, NULL);
        
        current_layer = zmk_keymap_highest_layer_active();
        if (current_layer > 4) current_layer = 4;
        update_layer_indicators();
    }
    
    return screen;
}

#ifdef __cplusplus
}
#endif