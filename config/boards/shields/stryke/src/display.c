#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display.h>
#include <zephyr/logging/log.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>

LOG_MODULE_REGISTER(oled_display, LOG_LEVEL_DBG);

// Display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CARD_HEIGHT 36
#define MAX_TEXT_HEIGHT 16
#define OLED_ADDRESS 0x3C

// Layer names (must match your keymap)
static const char* layer_names[] = {"BASE", "KICAD", "FUSION", "VS-CODE", "ARDUINO"};

// Command descriptions for your specific keymap
static const char* command_descriptions[] = {
    // Base Layer
    "COPY", "PASTE", "CUT", "UNDO",
    "SEL ALL", "SAVE", "FIND", "REDO",
    "SCREENSHOT", "SCREEN REC", "BOOT", "TOGGLE KICAD",
    
    // KiCad Layer
    "ROTATE", "MIRROR", "GRID", "DELETE",
    "VIA", "EDIT", "XOR", "WIRE",
    "BUS", "FOOTPRINT", "HIGHLIGHT", "TOGGLE FUSION",
    
    // Fusion Layer
    "SHOW KEY", "ROTATE", "LINE", "DIMENSION",
    "EXTEND", "QUIT", "FILET", "UNDO",
    "VIEW 1", "VIEW 2", "HOME VIEW", "TOGGLE VS-CODE",
    
    // VS-Code Layer
    "CMD PALETTE", "PROJ SEARCH", "SIDEBAR", "TERMINAL",
    "GOTO DEF", "GOTO LINE", "FIND", "UNDO",
    "QUICK FIX", "FIND FILES", "EXPLORER", "TOGGLE ARDUINO",
    
    // Arduino Layer
    "RUN", "UPLOAD", "SERIAL MON", "ADD LIB",
    "AUTOFORMAT", "ZOOM", "LIB MANAGER", "SAVE",
    "CLEAR BT", "NEXT BT", "RESET", "TOGGLE BASE"
};

// Display variables
static uint8_t current_layer = 0;
static char last_key[32] = "—";
static char last_command[32] = "—";
static int64_t last_key_time = 0;
static struct device *display_dev;
static bool display_initialized = false;

// Buffer for building key combinations
static char key_buffer[32];
static bool combo_active = false;
static int64_t combo_start_time = 0;
static uint32_t active_modifiers = 0;

// Key position tracking
#define MAX_KEYS 12  // 3x4 keypad
static uint8_t last_key_position = 0;

// Helper function to find best text size
static int find_best_text_size(const char* text, int max_width, int max_height) {
    bool is_combo = strchr(text, '+') != NULL;
    
    // Try sizes from 4 down to 1
    for (int size = 4; size >= 1; size--) {
        int text_height = size * 8;
        
        if (text_height > max_height) {
            continue;
        }
        
        int approx_width = 0;
        if (is_combo) {
            const char* combo = text;
            const char* start = combo;
            
            while (*combo) {
                if (*combo == '+') {
                    int part_len = combo - start;
                    approx_width += part_len * (size * 6);
                    approx_width += size * 6; // Plus sign
                    start = combo + 1;
                }
                combo++;
            }
            // Last part
            int part_len = combo - start;
            approx_width += part_len * (size * 6);
        } else {
            approx_width = strlen(text) * (size * 6);
        }
        
        if (approx_width <= max_width) {
            return size;
        }
    }
    
    return 1;
}

// Convert ZMK keycode to string with modifier awareness
static void keycode_to_string(uint16_t keycode, bool is_modifier, char* buffer, size_t buffer_size) {
    switch (keycode) {
        // Letters
        case KEY_A: strcpy(buffer, "A"); break;
        case KEY_B: strcpy(buffer, "B"); break;
        case KEY_C: strcpy(buffer, "C"); break;
        case KEY_D: strcpy(buffer, "D"); break;
        case KEY_E: strcpy(buffer, "E"); break;
        case KEY_F: strcpy(buffer, "F"); break;
        case KEY_G: strcpy(buffer, "G"); break;
        case KEY_H: strcpy(buffer, "H"); break;
        case KEY_I: strcpy(buffer, "I"); break;
        case KEY_J: strcpy(buffer, "J"); break;
        case KEY_K: strcpy(buffer, "K"); break;
        case KEY_L: strcpy(buffer, "L"); break;
        case KEY_M: strcpy(buffer, "M"); break;
        case KEY_N: strcpy(buffer, "N"); break;
        case KEY_O: strcpy(buffer, "O"); break;
        case KEY_P: strcpy(buffer, "P"); break;
        case KEY_Q: strcpy(buffer, "Q"); break;
        case KEY_R: strcpy(buffer, "R"); break;
        case KEY_S: strcpy(buffer, "S"); break;
        case KEY_T: strcpy(buffer, "T"); break;
        case KEY_U: strcpy(buffer, "U"); break;
        case KEY_V: strcpy(buffer, "V"); break;
        case KEY_W: strcpy(buffer, "W"); break;
        case KEY_X: strcpy(buffer, "X"); break;
        case KEY_Y: strcpy(buffer, "Y"); break;
        case KEY_Z: strcpy(buffer, "Z"); break;
        
        // Numbers
        case KEY_0: strcpy(buffer, "0"); break;
        case KEY_1: strcpy(buffer, "1"); break;
        case KEY_2: strcpy(buffer, "2"); break;
        case KEY_3: strcpy(buffer, "3"); break;
        case KEY_4: strcpy(buffer, "4"); break;
        case KEY_5: strcpy(buffer, "5"); break;
        case KEY_6: strcpy(buffer, "6"); break;
        case KEY_7: strcpy(buffer, "7"); break;
        case KEY_8: strcpy(buffer, "8"); break;
        case KEY_9: strcpy(buffer, "9"); break;
        
        // Modifiers
        case KEY_LEFTCTRL: 
        case KEY_RIGHTCTRL: 
            if (active_modifiers & MOD_LCTRL || active_modifiers & MOD_RCTRL) {
                strcpy(buffer, "CTRL");
            } else {
                strcpy(buffer, "CTRL");
            }
            break;
            
        case KEY_LEFTSHIFT: 
        case KEY_RIGHTSHIFT: 
            if (active_modifiers & MOD_LSFT || active_modifiers & MOD_RSFT) {
                strcpy(buffer, "SHFT");
            } else {
                strcpy(buffer, "SHFT");
            }
            break;
            
        case KEY_LEFTALT: 
        case KEY_RIGHTALT: 
            if (active_modifiers & MOD_LALT || active_modifiers & MOD_RALT) {
                strcpy(buffer, "ALT");
            } else {
                strcpy(buffer, "ALT");
            }
            break;
            
        case KEY_LEFTMETA: 
        case KEY_RIGHTMETA: 
            if (active_modifiers & MOD_LGUI || active_modifiers & MOD_RGUI) {
                strcpy(buffer, "CMD");
            } else {
                strcpy(buffer, "CMD");
            }
            break;
        
        // Special keys
        case KEY_SPACE: strcpy(buffer, "SPC"); break;
        case KEY_ENTER: strcpy(buffer, "ENT"); break;
        case KEY_ESC: strcpy(buffer, "ESC"); break;
        case KEY_TAB: strcpy(buffer, "TAB"); break;
        case KEY_BACKSPACE: strcpy(buffer, "BSPC"); break;
        case KEY_DELETE: strcpy(buffer, "DEL"); break;
        case KEY_INSERT: strcpy(buffer, "INS"); break;
        case KEY_HOME: strcpy(buffer, "HOME"); break;
        case KEY_END: strcpy(buffer, "END"); break;
        case KEY_PAGEUP: strcpy(buffer, "PGUP"); break;
        case KEY_PAGEDOWN: strcpy(buffer, "PGDN"); break;
        case KEY_SLASH: strcpy(buffer, "/"); break;
        case KEY_BACKSLASH: strcpy(buffer, "\\"); break;
        case KEY_EQUAL: strcpy(buffer, "="); break;
        case KEY_MINUS: strcpy(buffer, "-"); break;
        case KEY_COMMA: strcpy(buffer, ","); break;
        case KEY_DOT: strcpy(buffer, "."); break;
        case KEY_SEMICOLON: strcpy(buffer, ";"); break;
        case KEY_APOSTROPHE: strcpy(buffer, "'"); break;
        case KEY_GRAVE: strcpy(buffer, "`"); break;
        case KEY_LEFTBRACE: strcpy(buffer, "["); break;
        case KEY_RIGHTBRACE: strcpy(buffer, "]"); break;
        
        // Function keys
        case KEY_F1: strcpy(buffer, "F1"); break;
        case KEY_F2: strcpy(buffer, "F2"); break;
        case KEY_F3: strcpy(buffer, "F3"); break;
        case KEY_F4: strcpy(buffer, "F4"); break;
        case KEY_F5: strcpy(buffer, "F5"); break;
        case KEY_F6: strcpy(buffer, "F6"); break;
        case KEY_F7: strcpy(buffer, "F7"); break;
        case KEY_F8: strcpy(buffer, "F8"); break;
        case KEY_F9: strcpy(buffer, "F9"); break;
        case KEY_F10: strcpy(buffer, "F10"); break;
        case KEY_F11: strcpy(buffer, "F11"); break;
        case KEY_F12: strcpy(buffer, "F12"); break;
        
        // Arrow keys
        case KEY_UP: strcpy(buffer, "UP"); break;
        case KEY_DOWN: strcpy(buffer, "DN"); break;
        case KEY_LEFT: strcpy(buffer, "LFT"); break;
        case KEY_RIGHT: strcpy(buffer, "RGT"); break;
        
        // Bluetooth
        case C_BT_CLR: strcpy(buffer, "BT CLR"); break;
        case C_BT_NXT: strcpy(buffer, "BT NXT"); break;
        
        // System
        case C_BOOTLOADER: strcpy(buffer, "BOOT"); break;
        case C_SYSTEM_RESET: strcpy(buffer, "RESET"); break;
        
        default: 
            snprintf(buffer, buffer_size, "KEY%u", keycode);
            break;
    }
}

// Get command description based on layer and key position
static void get_command_description(uint8_t layer, uint8_t key_pos, char* buffer, size_t buffer_size) {
    int index = (layer * 12) + key_pos;
    if (index < (int)(sizeof(command_descriptions) / sizeof(command_descriptions[0]))) {
        strncpy(buffer, command_descriptions[index], buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    } else {
        strcpy(buffer, "UNKNOWN");
    }
}

// Get friendly key name based on your keymap bindings
static void get_friendly_key_name(uint8_t layer, uint8_t key_pos, char* buffer, size_t buffer_size) {
    const char* base_keys[] = {"CMD+C", "CMD+V", "CMD+X", "CMD+Z", 
                               "CMD+A", "CMD+S", "CMD+F", "CMD+SHFT+Z",
                               "CMD+SHFT+3", "CMD+SHFT+4", "BOOT", "TOG1"};
    
    const char* kicad_keys[] = {"R", "M", "G", "DEL", 
                                "V", "E", "X", "W",
                                "B", "F", "H", "TOG2"};
    
    const char* fusion_keys[] = {"CMD+K", "R", "L", "D", 
                                 "E", "Q", "F", "CMD+Z",
                                 "CMD+1", "CMD+2", "CMD+0", "TOG3"};
    
    const char* vscode_keys[] = {"CMD+P", "SHFT+CMD+P", "CMD+B", "CMD+/", 
                                 "CMD+D", "SHFT+CMD+L", "CMD+F", "CMD+Z",
                                 "CMD+'", "SHFT+CMD+F", "SHFT+CMD+E", "TOG4"};
    
    const char* arduino_keys[] = {"CMD+R", "CMD+U", "SHFT+CMD+M", "CMD+K", 
                                  "CMD+T", "CMD+Z", "CMD+L", "CMD+S",
                                  "BT_CLR", "BT_NXT", "RESET", "TOG0"};
    
    const char** layer_keys;
    
    switch (layer) {
        case 0: layer_keys = base_keys; break;
        case 1: layer_keys = kicad_keys; break;
        case 2: layer_keys = fusion_keys; break;
        case 3: layer_keys = vscode_keys; break;
        case 4: layer_keys = arduino_keys; break;
        default: 
            strcpy(buffer, "UNKNOWN");
            return;
    }
    
    if (key_pos < 12) {
        strncpy(buffer, layer_keys[key_pos], buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    } else {
        strcpy(buffer, "UNKNOWN");
    }
}

// Draw key display with both key combo and command
static void draw_key_display(struct display_buffer_descriptor *desc, uint8_t *buf) {
    int card_x = 2;
    int card_y = 2;
    int card_w = 124;
    int card_h = CARD_HEIGHT;
    
    // Draw card border (top card for key combo)
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < card_w; x++) {
            int px = card_x + x;
            int py = card_y + y;
            
            if ((x == 0 || x == card_w-1 || y == 0 || y == 19) ||
                ((x == 1 || x == card_w-2) && y > 0 && y < 19)) {
                if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                    buf[py * SCREEN_WIDTH + px] = 1;
                }
            }
        }
    }
    
    // Active key glow
    int64_t now = k_uptime_get();
    if (now - last_key_time < 300) {
        for (int i = 2; i < 4; i++) {
            int glow_w = card_w - 2*i;
            int glow_h = 20 - 2*i;
            int glow_x = card_x + i;
            int glow_y = card_y + i;
            
            for (int y = 0; y < glow_h; y++) {
                for (int x = 0; x < glow_w; x++) {
                    if ((x == 0 || x == glow_w-1 || y == 0 || y == glow_h-1)) {
                        int px = glow_x + x;
                        int py = glow_y + y;
                        if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                            buf[py * SCREEN_WIDTH + px] = 1;
                        }
                    }
                }
            }
        }
    }
    
    // Draw key combination (top half)
    int max_key_width = card_w - 8;
    int key_size = find_best_text_size(last_key, max_key_width, 16);
    
    // Center the key text
    int key_len = strlen(last_key);
    int key_text_width = key_len * key_size * 6;
    int key_start_x = card_x + (card_w - key_text_width) / 2;
    int key_start_y = card_y + 6;
    
    // Animation for fresh press
    if (now - last_key_time < 200) {
        int padding = key_size;
        int anim_w = key_text_width + padding * 2;
        int anim_h = key_size * 8 + 2;
        int anim_x = key_start_x - padding;
        int anim_y = key_start_y - 1;
        
        // Draw inverted rectangle
        for (int y = anim_y; y < anim_y + anim_h; y++) {
            for (int x = anim_x; x < anim_x + anim_w; x++) {
                if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                    // Clear area for inverted text
                    if (y >= key_start_y && y < key_start_y + key_size * 8 &&
                        x >= key_start_x && x < key_start_x + key_text_width) {
                        buf[y * SCREEN_WIDTH + x] = 0;
                    } else {
                        buf[y * SCREEN_WIDTH + x] = 1;
                    }
                }
            }
        }
    }
    
    // Draw command description (bottom half)
    int command_y = card_y + 22;
    
    // Draw separator line
    for (int x = card_x + 10; x < card_x + card_w - 10; x++) {
        if (x < SCREEN_WIDTH && command_y < SCREEN_HEIGHT) {
            buf[command_y * SCREEN_WIDTH + x] = 1;
        }
    }
    
    // Draw command box
    for (int y = command_y + 2; y < command_y + 12; y++) {
        for (int x = card_x; x < card_x + card_w; x++) {
            if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
                if ((x == card_x || x == card_x + card_w - 1 || 
                     y == command_y + 2 || y == command_y + 11)) {
                    buf[y * SCREEN_WIDTH + x] = 1;
                }
            }
        }
    }
    
    // Draw command text
    int cmd_len = strlen(last_command);
    int cmd_size = 1; // Fixed size for command text
    int cmd_text_width = cmd_len * cmd_size * 6;
    int cmd_start_x = card_x + (card_w - cmd_text_width) / 2;
    int cmd_start_y = command_y + 6;
}

// Draw layer indicators with layer names
static void draw_layer_indicators(uint8_t *buf) {
    int indicator_y = 46;
    int indicator_spacing = 25;
    int start_x = 4;
    
    for (int i = 0; i < 5; i++) {
        int x = start_x + (i * indicator_spacing);
        
        // Draw indicator box
        for (int y = indicator_y; y < indicator_y + 12; y++) {
            for (int dx = 0; dx < 22; dx++) {
                int px = x + dx;
                if (px < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
                    if (i == current_layer) {
                        // Active layer: filled
                        if (dx > 1 && dx < 20 && y > indicator_y + 1 && y < indicator_y + 11) {
                            buf[y * SCREEN_WIDTH + px] = 1;
                        }
                    } else {
                        // Inactive layer: outline
                        if ((dx == 0 || dx == 21 || y == indicator_y || y == indicator_y + 11) ||
                            ((dx == 1 || dx == 20) && (y > indicator_y + 1 && y < indicator_y + 10))) {
                            buf[y * SCREEN_WIDTH + px] = 1;
                        }
                    }
                }
            }
        }
        
        // Draw layer number
        if (i == current_layer) {
            // Active: inverted text
            for (int dy = 2; dy < 10; dy++) {
                for (int dx = 2; dx < 20; dx++) {
                    int px = x + dx;
                    int py = indicator_y + dy;
                    if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                        buf[py * SCREEN_WIDTH + px] = 0;
                    }
                }
            }
        }
    }
    
    // Draw layer name at the bottom
    int name_y = 60;
    const char* layer_name = layer_names[current_layer];
    int name_len = strlen(layer_name);
    int name_start_x = (SCREEN_WIDTH - name_len * 6) / 2;
    
    // Simple character rendering (in real implementation, use font)
    for (int i = 0; i < name_len; i++) {
        char c = layer_name[i];
        for (int dy = 0; dy < 8; dy++) {
            for (int dx = 0; dx < 6; dx++) {
                int px = name_start_x + i * 6 + dx;
                int py = name_y + dy;
                if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                    // Simple dot for each character
                    if (dx == 2 || dx == 3) {
                        buf[py * SCREEN_WIDTH + px] = 1;
                    }
                }
            }
        }
    }
}

// Update display
static void update_display() {
    if (!display_initialized || !display_dev) {
        return;
    }
    
    uint8_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 8] = {0};
    struct display_buffer_descriptor desc = {
        .buf_size = sizeof(buf),
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .pitch = SCREEN_WIDTH
    };
    
    // Clear display
    memset(buf, 0, sizeof(buf));
    
    // Draw components
    draw_key_display(&desc, buf);
    draw_layer_indicators(buf);
    
    // Update display
    display_write(display_dev, 0, 0, &desc, buf);
}

// Event handlers
static int layer_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev != NULL) {
        current_layer = zmk_keymap_highest_layer_active();
        LOG_DBG("Layer changed to %d", current_layer);
        update_display();
    }
    return 0;
}

static int keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    if (ev != NULL) {
        int64_t now = k_uptime_get();
        
        if (ev->state) { // Key press
            // Get friendly key name based on position (simplified)
            // In real implementation, you'd track key matrix position
            last_key_position = (last_key_position + 1) % 12;
            
            // Get the friendly key combination
            get_friendly_key_name(current_layer, last_key_position, last_key, sizeof(last_key));
            
            // Get the command description
            get_command_description(current_layer, last_key_position, last_command, sizeof(last_command));
            
            last_key_time = now;
            
            LOG_DBG("Key pressed: Layer=%d, Pos=%d, Key=%s, Command=%s", 
                   current_layer, last_key_position, last_key, last_command);
            
            update_display();
        }
    }
    
    return 0;
}

static int modifiers_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_modifiers_state_changed *ev = as_zmk_modifiers_state_changed(eh);
    
    if (ev != NULL) {
        active_modifiers = ev->modifiers;
        LOG_DBG("Modifiers changed: 0x%02X", active_modifiers);
    }
    
    return 0;
}

// Initialize display
static int display_init(const struct device *dev) {
    display_dev = device_get_binding("SSD1306");
    if (display_dev == NULL) {
        LOG_ERR("Could not get display device");
        return -1;
    }
    
    display_initialized = true;
    LOG_INF("OLED display initialized");
    
    // Startup animation
    uint8_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 8] = {0};
    struct display_buffer_descriptor desc = {
        .buf_size = sizeof(buf),
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .pitch = SCREEN_WIDTH
    };
    
    // Expansion animation
    for (int i = 0; i < 16; i++) {
        memset(buf, 0, sizeof(buf));
        for (int y = 32 - i; y < 32 + i; y++) {
            for (int x = 64 - i * 4; x < 64 + i * 4; x++) {
                if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                    if (x == 64 - i * 4 || x == 64 + i * 4 - 1 || 
                        y == 32 - i || y == 32 + i - 1) {
                        buf[y * SCREEN_WIDTH + x] = 1;
                    }
                }
            }
        }
        display_write(display_dev, 0, 0, &desc, buf);
        k_sleep(K_MSEC(30));
    }
    
    // Show startup message
    memset(buf, 0, sizeof(buf));
    // Draw border
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (x == 0 || x == SCREEN_WIDTH - 1 || y == 0 || y == SCREEN_HEIGHT - 1) {
                buf[y * SCREEN_WIDTH + x] = 1;
            }
        }
    }
    
    // Draw "PRO KEYPAD" text (simplified)
    for (int i = 0; i < 10; i++) {
        for (int y = 20; y < 28; y++) {
            for (int x = 30 + i * 6; x < 30 + i * 6 + 4; x++) {
                if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
                    buf[y * SCREEN_WIDTH + x] = 1;
                }
            }
        }
    }
    
    display_write(display_dev, 0, 0, &desc, buf);
    k_sleep(K_MSEC(1000));
    
    // Clear for normal operation
    memset(buf, 0, sizeof(buf));
    display_write(display_dev, 0, 0, &desc, buf);
    
    return 0;
}

// Register event listeners
ZMK_LISTENER(oled_display_layer, layer_state_changed_listener);
ZMK_SUBSCRIPTION(oled_display_layer, zmk_layer_state_changed);

ZMK_LISTENER(oled_display_keycode, keycode_state_changed_listener);
ZMK_SUBSCRIPTION(oled_display_keycode, zmk_keycode_state_changed);

ZMK_LISTENER(oled_display_modifiers, modifiers_state_changed_listener);
ZMK_SUBSCRIPTION(oled_display_modifiers, zmk_modifiers_state_changed);

// Initialize on boot
SYS_INIT(display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);