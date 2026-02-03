#include <zephyr/kernel.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <lvgl.h>
#include "custom_bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MAX_TEXT_HEIGHT 16
#define MAX_LAYERS 5
#define MAX_POSITIONS 12

static lv_obj_t *screen = NULL;
static lv_obj_t *key_label = NULL;
static lv_obj_t *time_img = NULL;
static lv_obj_t *layer_img = NULL;
static lv_obj_t *bg_canvas = NULL;

static uint8_t current_layer = 0;
static char last_key_text[32] = "-";
static int64_t last_key_time = 0;

static const uint8_t org_01_bitmaps[] = {
    0xFC, 0x63, 0x1F, 0x80,
    0xF8,
    0xF8, 0x7F, 0x0F, 0x80,
    0xF8, 0x7E, 0x1F, 0x80,
    0x8C, 0x7E, 0x10, 0x80,
    0xFC, 0x3E, 0x1F, 0x80,
    0xFC, 0x3F, 0x1F, 0x80,
    0xF8, 0x42, 0x10, 0x80,
    0xFC, 0x7F, 0x1F, 0x80,
    0xFC, 0x7E, 0x1F, 0x80,
    0x90,
    0xFC, 0x7F, 0x18, 0x80,
    0xF4, 0x7D, 0x1F, 0x00,
    0xFC, 0x21, 0x0F, 0x80,
    0xF4, 0x63, 0x1F, 0x00,
    0xFC, 0x3F, 0x0F, 0x80,
    0xFC, 0x3F, 0x08, 0x00,
    0xFC, 0x2F, 0x1F, 0x80,
    0x8C, 0x7F, 0x18, 0x80,
    0xF9, 0x08, 0x4F, 0x80,
    0x78, 0x85, 0x2F, 0x80,
    0x8D, 0xB1, 0x68, 0x80,
    0x84, 0x21, 0x0F, 0x80,
    0xFD, 0x6B, 0x5A, 0x80,
    0xFC, 0x63, 0x18, 0x80,
    0xFC, 0x63, 0x1F, 0x80,
    0xFC, 0x7F, 0x08, 0x00,
    0xFC, 0x63, 0x3F, 0x80,
    0xFC, 0x7F, 0x29, 0x00,
    0xFC, 0x3E, 0x1F, 0x80,
    0xF9, 0x08, 0x42, 0x00,
    0x8C, 0x63, 0x1F, 0x80,
    0x8C, 0x62, 0xA2, 0x00,
    0xAD, 0x6B, 0x5F, 0x80,
    0x8A, 0x88, 0xA8, 0x80,
    0x8C, 0x54, 0x42, 0x00,
    0xF8, 0x7F, 0x0F, 0x80,
};

static const uint8_t org_01_offsets[] = {
    0, 4, 5, 9, 13, 17, 21, 25, 29, 33,
    37, 38, 42, 46, 50, 54, 58, 62, 66, 70,
    74, 78, 82, 86, 90, 94, 98, 102, 106, 110,
    114, 118, 122, 126, 130, 134, 138,
};

static const uint8_t org_01_widths[] = {
    5, 1, 5, 5, 5, 5, 5, 5, 5, 5,
    1,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5,
};

static const char* layer_names[MAX_LAYERS] = {
    "BASE",
    "EDA",
    "CAD",
    "IDE",
    "MCU"
};

static const char* key_names[MAX_LAYERS][MAX_POSITIONS] = {
    {
        "CMD+C", "CMD+V", "CMD+X", "CMD+Z",
        "CMD+A", "CMD+S", "SIGNOUT", "EMAIL",
        "CMD+SHFT+3", "CMD+SHFT+4", "Bootloader", "KiCad"
    },
    {
        "R", "M", "G", "DEL",
        "V", "E", "X", "W",
        "B", "F", "H", "Fusion"
    },
    {
        "Cmd+K", "R", "L", "D",
        "E", "Q", "F", "CMD+Z",
        "CMD+1", "CMD+2", "CMD+0", "VS Code"
    },
    {
        "Cmd+P", "CMD+SHFT+P", "CMD+B", "CMD+FSLH",
        "CMD+D", "CMD+SHFT+L", "CMD+F", "CMD+Z",
        "CMD+SQRT", "CMD+SHFT+F", "CMD+SHFT+E", "Arduino"
    },
    {
        "CMD+R", "CMD+U", "CMD+SHFT+M", "CMD+K",
        "CMD+T", "CMD+Z", "CMD+L", "CMD+S",
        "BT Clear", "BT Next", "Reset", "Base"
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
    int64_t uptime_sec = k_uptime_get() / 1000;
    int hours = (uptime_sec / 3600) % 24;
    int mins = (uptime_sec / 60) % 60;
    snprintf(buffer, buffer_size, "%02d:%02d", hours, mins);
}

static void draw_org_char(lv_color_t* buf, int16_t buf_w, int16_t buf_h, char c, int16_t x, int16_t y) {
    int char_index = -1;
    
    if (c >= '0' && c <= '9') {
        char_index = c - '0';
    } else if (c == ':') {
        char_index = 10;
    } else if (c >= 'A' && c <= 'Z') {
        char_index = 11 + (c - 'A');
    } else if (c >= 'a' && c <= 'z') {
        char_index = 11 + (c - 'a');
    } else {
        return;
    }
    
    uint8_t offset = org_01_offsets[char_index];
    uint8_t width = org_01_widths[char_index];
    
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

static void draw_org_string(lv_color_t* buf, int16_t buf_w, int16_t buf_h, const char* str, int16_t x, int16_t y) {
    int16_t cursor_x = x;
    
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        draw_org_char(buf, buf_w, buf_h, c, cursor_x, y);
        
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
            cursor_x += org_01_widths[char_index] + 1;
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

static void create_custom_background(void) {
    if (bg_canvas != NULL) {
        lv_obj_del(bg_canvas);
    }

    bg_canvas = lv_canvas_create(screen);
    lv_obj_set_size(bg_canvas, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_move_to_index(bg_canvas, 0);
    
    static lv_color_t bg_buf[SCREEN_WIDTH * SCREEN_HEIGHT];
    lv_canvas_set_buffer(bg_canvas, bg_buf, SCREEN_WIDTH, SCREEN_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    
    // Initialize buffer to black (all zeros = no pixels drawn on monochrome OLED)
    memset(bg_buf, 0, sizeof(bg_buf));

    // Draw only the white bitmap pixels where bit = 1
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int pixel_index = y * SCREEN_WIDTH + x;
            int byte_index = pixel_index / 8;
            int bit_index = 7 - (pixel_index % 8);
            
            if (byte_index < sizeof(custom_background)) {
                if (custom_background[byte_index] & (1 << bit_index)) {
                    lv_canvas_set_px_color(bg_canvas, x, y, lv_color_white());
                }
            }
        }
    }
}

static void update_time_display(void) {
    if (time_img == NULL) return;
    
    char time_str[6];
    get_est_time_string(time_str, sizeof(time_str));
    
    lv_img_dsc_t* img_dsc = (lv_img_dsc_t*)lv_obj_get_user_data(time_img);
    if (img_dsc && img_dsc->data) {
        lv_color_t* buf = (lv_color_t*)img_dsc->data;
        memset(buf, 0, 30 * 5 * sizeof(lv_color_t));
        draw_org_string(buf, 30, 5, time_str, 0, 0);
        lv_obj_invalidate(time_img);
    }
}

static void update_layer_display(void) {
    if (layer_img == NULL) return;
    
    const char* layer_name = get_layer_name(current_layer);
    lv_img_dsc_t* img_dsc = (lv_img_dsc_t*)lv_obj_get_user_data(layer_img);
    if (img_dsc && img_dsc->data) {
        lv_color_t* buf = (lv_color_t*)img_dsc->data;
        memset(buf, 0, 30 * 5 * sizeof(lv_color_t));
        draw_org_string(buf, 30, 5, layer_name, 0, 0);
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
    
    // Set screen background to black
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    
    create_custom_background();
    
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
    
    key_label = lv_label_create(screen);
    lv_label_set_text(key_label, last_key_text);
    lv_obj_set_style_text_color(key_label, lv_color_make(100, 100, 100), LV_PART_MAIN);
    lv_obj_set_style_text_font(key_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(key_label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(key_label, 4, LV_PART_MAIN);
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
    
    uint8_t new_layer = ev->layer;
    
    if (new_layer < MAX_LAYERS) {
        current_layer = new_layer;
        update_layer_display();
    }
    
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
        
        if (position == 11) {
            current_layer = zmk_keymap_highest_layer_active();
            if (current_layer >= MAX_LAYERS) current_layer = 0;
            update_layer_display();
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