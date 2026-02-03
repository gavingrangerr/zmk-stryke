#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MAX_TEXT_HEIGHT 16
#define MAX_LAYERS 5
#define MAX_POSITIONS 12

// Eastern Standard Time offset (UTC-5)
#define EST_OFFSET_HOURS -5

static lv_obj_t *screen = NULL;
static lv_obj_t *key_label = NULL;
static lv_obj_t *time_img = NULL;
static lv_obj_t *layer_img = NULL;

static uint8_t current_layer = 0;
static char last_key_text[32] = "---";
static int64_t last_key_time = 0;

static const uint8_t org_01_bitmaps[] = {
    // '0' - 5 wide
    0xFC, 0x63, 0x1F, 0x80,
    // '1' - 1 wide
    0xF8,
    // '2' - 5 wide
    0xF8, 0x7F, 0x0F, 0x80,
    // '3' - 5 wide
    0xF8, 0x7E, 0x1F, 0x80,
    // '4' - 5 wide
    0x8C, 0x7E, 0x10, 0x80,
    // '5' - 5 wide
    0xFC, 0x3E, 0x1F, 0x80,
    // '6' - 5 wide
    0xFC, 0x3F, 0x1F, 0x80,
    // '7' - 5 wide
    0xF8, 0x42, 0x10, 0x80,
    // '8' - 5 wide
    0xFC, 0x7F, 0x1F, 0x80,
    // '9' - 5 wide
    0xFC, 0x7E, 0x1F, 0x80,
    // ':' - 1 wide
    0x90,
    // 'A' - 5 wide
    0xFC, 0x7F, 0x18, 0x80,
    // 'B' - 5 wide
    0xF4, 0x7D, 0x1F, 0x00,
    // 'C' - 5 wide
    0xFC, 0x21, 0x0F, 0x80,
    // 'D' - 5 wide
    0xF4, 0x63, 0x1F, 0x00,
    // 'E' - 5 wide
    0xFC, 0x3F, 0x0F, 0x80,
    // 'F' - 5 wide
    0xFC, 0x3F, 0x08, 0x00,
    // 'G' - 5 wide
    0xFC, 0x2F, 0x1F, 0x80,
    // 'H' - 5 wide
    0x8C, 0x7F, 0x18, 0x80,
    // 'I' - 5 wide
    0xF9, 0x08, 0x4F, 0x80,
    // 'J' - 5 wide
    0x78, 0x85, 0x2F, 0x80,
    // 'K' - 5 wide
    0x8D, 0xB1, 0x68, 0x80,
    // 'L' - 5 wide
    0x84, 0x21, 0x0F, 0x80,
    // 'M' - 5 wide
    0xFD, 0x6B, 0x5A, 0x80,
    // 'N' - 5 wide
    0xFC, 0x63, 0x18, 0x80,
    // 'O' - 5 wide
    0xFC, 0x63, 0x1F, 0x80,
    // 'P' - 5 wide
    0xFC, 0x7F, 0x08, 0x00,
    // 'Q' - 5 wide
    0xFC, 0x63, 0x3F, 0x80,
    // 'R' - 5 wide
    0xFC, 0x7F, 0x29, 0x00,
    // 'S' - 5 wide
    0xFC, 0x3E, 0x1F, 0x80,
    // 'T' - 5 wide
    0xF9, 0x08, 0x42, 0x00,
    // 'U' - 5 wide
    0x8C, 0x63, 0x1F, 0x80,
    // 'V' - 5 wide
    0x8C, 0x62, 0xA2, 0x00,
    // 'W' - 5 wide
    0xAD, 0x6B, 0x5F, 0x80,
    // 'X' - 5 wide
    0x8A, 0x88, 0xA8, 0x80,
    // 'Y' - 5 wide
    0x8C, 0x54, 0x42, 0x00,
    // 'Z' - 5 wide
    0xF8, 0x7F, 0x0F, 0x80,
};

// Bitmap offsets for each character (in bytes)
static const uint8_t org_01_offsets[] = {
    0,   // '0'
    4,   // '1'
    5,   // '2'
    9,   // '3'
    13,  // '4'
    17,  // '5'
    21,  // '6'
    25,  // '7'
    29,  // '8'
    33,  // '9'
    37,  // ':'
    38,  // 'A'
    42,  // 'B'
    46,  // 'C'
    50,  // 'D'
    54,  // 'E'
    58,  // 'F'
    62,  // 'G'
    66,  // 'H'
    70,  // 'I'
    74,  // 'J'
    78,  // 'K'
    82,  // 'L'
    86,  // 'M'
    90,  // 'N'
    94,  // 'O'
    98,  // 'P'
    102, // 'Q'
    106, // 'R'
    110, // 'S'
    114, // 'T'
    118, // 'U'
    122, // 'V'
    126, // 'W'
    130, // 'X'
    134, // 'Y'
    138, // 'Z'
};

// Width of each character in pixels
static const uint8_t org_01_widths[] = {
    5, 1, 5, 5, 5, 5, 5, 5, 5, 5, // 0-9
    1, // :
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 // A-Z
};

static const char* layer_names[MAX_LAYERS] = {
    "BASE",  // Layer 0
    "EDA",   // Layer 1
    "CAD",   // Layer 2
    "IDE",   // Layer 3
    "MCU"    // Layer 4
};

static const char* key_names[MAX_LAYERS][MAX_POSITIONS] = {
    {
        "CMD+C",        "CMD+V",        "CMD+X",        "CMD+Z",
        "CMD+A",        "CMD+S",        "SIGNOUT",      "EMAIL",
        "CMD+SHFT+3",   "CMD+SHFT+4",   "Bootloader",   "KiCad"
    },
    {
        "R",            "M",            "G",            "DEL",
        "V",            "E",            "X",            "W",
        "B",            "F",            "H",            "Fusion"
    },
    {
        "Cmd+K",        "R",            "L",            "D",
        "E",            "Q",            "F",            "CMD+Z",
        "CMD+1",        "CMD+2",        "CMD+0",        "VS Code"
    },
    {
        "Cmd+P",        "CMD+SHFT+P",   "CMD+B",        "CMD+FSLH",
        "CMD+D",        "CMD+SHFT+L",   "CMD+F",        "CMD+Z",
        "CMD+SQRT",     "CMD+SHFT+F",   "CMD+SHFT+E",   "Arduino"
    },
    {
        "CMD+R",        "CMD+U",        "CMD+SHFT+M",   "CMD+K",
        "CMD+T",        "CMD+Z",        "CMD+L",        "CMD+S",
        "BT Clear",     "BT Next",      "Reset",        "Base"
    }
};

static const char* get_key_name_by_position(uint8_t layer, uint8_t position) {
    if (layer >= MAX_LAYERS || position >= MAX_POSITIONS) {
        return NULL;
    }
    return key_names[layer][position];
}

static const char* get_layer_name(uint8_t layer) {
    if (layer >= MAX_LAYERS) {
        return "???";
    }
    return layer_names[layer];
}

static void get_est_time_string(char* buffer, size_t buffer_size) {
    // Get uptime in seconds
    int64_t uptime_sec = k_uptime_get() / 1000;
    
    // Calculate relative hours/minutes since boot
    // Note: Without an RTC, ZMK doesn't know the 'real' time, 
    // but this will show time since the keyboard turned on.
    int hours = (uptime_sec / 3600) % 24;
    int mins = (uptime_sec / 60) % 60;
    
    snprintf(buffer, buffer_size, "%02d:%02d", hours, mins);
}

// Draw a single Org_01 character on image buffer
static void draw_org_char(lv_color_t* buf, int16_t buf_w, int16_t buf_h, char c, int16_t x, int16_t y) {
    int char_index = -1;
    
    // Map character to index
    if (c >= '0' && c <= '9') {
        char_index = c - '0';
    } else if (c == ':') {
        char_index = 10;
    } else if (c >= 'A' && c <= 'Z') {
        char_index = 11 + (c - 'A');
    } else if (c >= 'a' && c <= 'z') {
        char_index = 11 + (c - 'a'); // Convert lowercase to uppercase
    } else {
        return; // Unsupported character
    }
    
    uint8_t offset = org_01_offsets[char_index];
    uint8_t width = org_01_widths[char_index];
    
    // Draw the character pixel by pixel (5 pixel height)
    for (int cy = 0; cy < 5; cy++) {
        for (int cx = 0; cx < width; cx++) {
            int bit_index = cy * width + cx;
            int byte_index = offset + (bit_index / 8);
            int bit_offset = 7 - (bit_index % 8);
            
            if (org_01_bitmaps[byte_index] & (1 << bit_offset)) {
                int px = x + cx;
                int py = y + cy;
                if (px >= 0 && px < buf_w && py >= 0 && py < buf_h) {
                    buf[py * buf_w + px] = lv_color_white();
                }
            }
        }
    }
}

// Draw a string using Org_01 font
static void draw_org_string(lv_color_t* buf, int16_t buf_w, int16_t buf_h, const char* str, int16_t x, int16_t y) {
    int16_t cursor_x = x;
    
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        draw_org_char(buf, buf_w, buf_h, c, cursor_x, y);
        
        // Get character width and advance cursor
        int char_index = -1;
        if (c >= '0' && c <= '9') {
            char_index = c - '0';
        } else if (c == ':') {
            char_index = 10;
        } else if (c >= 'A' && c <= 'Z') {
            char_index = 11 + (c - 'A');
        } else if (c >= 'a' && c <= 'z') {
            char_index = 11 + (c - 'a');
        }
        
        if (char_index >= 0) {
            cursor_x += org_01_widths[char_index] + 1; // 1 pixel spacing
        }
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

static void update_time_display(void) {
    if (time_img == NULL) return;
    
    char time_str[6];
    get_est_time_string(time_str, sizeof(time_str));
    
    // Get the image descriptor and buffer
    lv_img_dsc_t* img_dsc = (lv_img_dsc_t*)lv_obj_get_user_data(time_img);
    if (img_dsc && img_dsc->data) {
        lv_color_t* buf = (lv_color_t*)img_dsc->data;
        
        // Clear buffer to black
        memset(buf, 0, 30 * 5 * sizeof(lv_color_t));
        
        // Draw the time string
        draw_org_string(buf, 30, 5, time_str, 0, 0);
        
        // Invalidate to trigger redraw
        lv_obj_invalidate(time_img);
    }
}

static void update_layer_display(void) {
    if (layer_img == NULL) return;
    
    const char* layer_name = get_layer_name(current_layer);
    
    // Get the image descriptor and buffer
    lv_img_dsc_t* img_dsc = (lv_img_dsc_t*)lv_obj_get_user_data(layer_img);
    if (img_dsc && img_dsc->data) {
        lv_color_t* buf = (lv_color_t*)img_dsc->data;
        
        // Clear buffer to black
        memset(buf, 0, 30 * 5 * sizeof(lv_color_t));
        
        // Draw the layer name
        draw_org_string(buf, 30, 5, layer_name, 0, 0);
        
        // Invalidate to trigger redraw
        lv_obj_invalidate(layer_img);
    }
}

static void update_key_display(void) {
    if (key_label == NULL) return;
    
    lv_label_set_text(key_label, last_key_text);
    
    int best_size = find_best_text_size(last_key_text, 124, MAX_TEXT_HEIGHT);
    const lv_font_t* font = get_font_for_size(best_size);
    lv_obj_set_style_text_font(key_label, font, LV_PART_MAIN);
    
    int64_t now = k_uptime_get();
    bool fresh_press = (now - last_key_time) < 200;
    
    if (fresh_press) {
        lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(key_label, lv_color_make(100, 100, 100), LV_PART_MAIN);
    }
    
    lv_obj_center(key_label);
    lv_obj_invalidate(key_label);
}

static void create_ui(void) {
    if (screen == NULL) return;
    
    lv_obj_clean(screen);
    
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    
    // Create image buffer for time (30x5 pixels)
    static lv_color_t time_buf[30 * 5];
    static lv_img_dsc_t time_img_dsc = {
        .header.cf = LV_IMG_CF_TRUE_COLOR,
        .header.always_zero = 0,
        .header.reserved = 0,
        .header.w = 30,
        .header.h = 5,
        .data_size = 30 * 5 * sizeof(lv_color_t),
        .data = (uint8_t*)time_buf,
    };
    memset(time_buf, 0, sizeof(time_buf));
    
    time_img = lv_img_create(screen);
    lv_img_set_src(time_img, &time_img_dsc);
    lv_obj_set_pos(time_img, 51, 54);
    lv_obj_set_user_data(time_img, &time_img_dsc);
    static lv_color_t layer_buf[30 * 5];
    static lv_img_dsc_t layer_img_dsc = {
        .header.cf = LV_IMG_CF_TRUE_COLOR,
        .header.always_zero = 0,
        .header.reserved = 0,
        .header.w = 30,
        .header.h = 5,
        .data_size = 30 * 5 * sizeof(lv_color_t),
        .data = (uint8_t*)layer_buf,
    };
    memset(layer_buf, 0, sizeof(layer_buf));
    
    layer_img = lv_img_create(screen);
    lv_img_set_src(layer_img, &layer_img_dsc);
    lv_obj_set_pos(layer_img, 76, 2);
    lv_obj_set_user_data(layer_img, &layer_img_dsc);
    
    // Key label - centered on screen (using standard LVGL font)
    key_label = lv_label_create(screen);
    lv_label_set_text(key_label, last_key_text);
    lv_obj_set_style_text_color(key_label, lv_color_make(100, 100, 100), LV_PART_MAIN);
    lv_obj_set_style_text_font(key_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(key_label);
    
    update_time_display();
    update_layer_display();
    update_key_display();
}

static void animation_timer_cb(lv_timer_t* timer) {
    update_key_display();
    update_time_display();
}

static int layer_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) return 0;
    
    current_layer = zmk_keymap_highest_layer_active();
    if (current_layer > 4) current_layer = 4;
    
    update_layer_display();
    
    return 0;
}

static int position_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) return 0;
    
    uint8_t position = ev->position;
    bool pressed = ev->state;
    
    if (pressed) {
        const char* key_name = get_key_name_by_position(current_layer, position);
        
        if (key_name != NULL) {
            strncpy(last_key_text, key_name, sizeof(last_key_text) - 1);
            last_key_text[sizeof(last_key_text) - 1] = '\0';
            last_key_time = k_uptime_get();
            update_key_display();
        }
    }
    
    return 0;
}

ZMK_LISTENER(custom_status_layer, layer_state_changed_cb);
ZMK_SUBSCRIPTION(custom_status_layer, zmk_layer_state_changed);

ZMK_LISTENER(custom_status_position, position_state_changed_cb);
ZMK_SUBSCRIPTION(custom_status_position, zmk_position_state_changed);

lv_obj_t *zmk_display_status_screen(void) {
    if (screen == NULL) {
        screen = lv_obj_create(NULL);
        lv_obj_set_size(screen, SCREEN_WIDTH, SCREEN_HEIGHT);
        
        create_ui();
        lv_timer_create(animation_timer_cb, 50, NULL);
        
        current_layer = zmk_keymap_highest_layer_active();
        if (current_layer > 4) current_layer = 4;
    }
    
    return screen;
}

#ifdef __cplusplus
}
#endif