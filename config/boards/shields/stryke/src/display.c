#include "display.h"
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

// Get I2C device from devicetree
#define I2C_DEV_NODE DT_IO_CHANNELS_CTLR(DT_NODELABEL(keyboard_status))
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);

static struct display_data display_data = {
    .current_layer = 0,
    .last_key_time = 0,
    .initialized = false
};

// SSD1306 commands (same as before)
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

static uint8_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];

static void ssd1306_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write(i2c_dev, buf, 2, OLED_ADDRESS);
}

static void ssd1306_init_display(void) {
    // Init sequence
    ssd1306_command(SSD1306_DISPLAYOFF);
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_command(0x80);
    ssd1306_command(SSD1306_SETMULTIPLEX);
    ssd1306_command(SCREEN_HEIGHT - 1);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    ssd1306_command(0x00);
    ssd1306_command(SSD1306_SETSTARTLINE | 0x00);
    ssd1306_command(SSD1306_CHARGEPUMP);
    ssd1306_command(0x14);
    ssd1306_command(SSD1306_MEMORYMODE);
    ssd1306_command(0x00);
    ssd1306_command(SSD1306_SEGREMAP | 0x01);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);
    ssd1306_command(0x12);
    ssd1306_command(SSD1306_SETCONTRAST);
    ssd1306_command(0x7F);
    ssd1306_command(SSD1306_SETPRECHARGE);
    ssd1306_command(0xF1);
    ssd1306_command(SSD1306_SETVCOMDETECT);
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
    ssd1306_command(SSD1306_NORMALDISPLAY);
    ssd1306_command(SSD1306_DISPLAYON);
    
    memset(buffer, 0, sizeof(buffer));
}

// Drawing functions (same implementation as before, but without device parameter)
static void draw_pixel(int16_t x, int16_t y, bool color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        if (color) {
            buffer[x + (y / 8) * SCREEN_WIDTH] |= (1 << (y & 7));
        } else {
            buffer[x + (y / 8) * SCREEN_WIDTH] &= ~(1 << (y & 7));
        }
    }
}

static void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = -abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    
    while (true) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    draw_line(x, y, x + w - 1, y, color);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    draw_line(x + w - 1, y + h - 1, x, y + h - 1, color);
    draw_line(x, y + h - 1, x, y, color);
}

static void draw_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    for (int16_t i = x; i < x + w; i++) {
        for (int16_t j = y; j < y + h; j++) {
            draw_pixel(i, j, color);
        }
    }
}

static void draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color) {
    // Straight edges
    draw_line(x + r, y, x + w - r - 1, y, color);
    draw_line(x + r, y + h - 1, x + w - r - 1, y + h - 1, color);
    draw_line(x, y + r, x, y + h - r - 1, color);
    draw_line(x + w - 1, y + r, x + w - 1, y + h - r - 1, color);
    
    // Rounded corners
    for (int16_t i = 0; i < r; i++) {
        for (int16_t j = 0; j < r; j++) {
            if ((i * i + j * j) <= (r * r)) {
                draw_pixel(x + r - i, y + r - j, color);
                draw_pixel(x + w - r + i - 1, y + r - j, color);
                draw_pixel(x + r - i, y + h - r + j - 1, color);
                draw_pixel(x + w - r + i - 1, y + h - r + j - 1, color);
            }
        }
    }
}

static void draw_fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color) {
    // Fill center
    draw_fill_rect(x + r, y, w - 2 * r, h, color);
    
    // Fill sides
    draw_fill_rect(x, y + r, r, h - 2 * r, color);
    draw_fill_rect(x + w - r, y + r, r, h - 2 * r, color);
    
    // Fill corners
    for (int16_t i = 0; i < r; i++) {
        for (int16_t j = 0; j < r; j++) {
            if ((i * i + j * j) <= (r * r)) {
                for (int16_t k = -j; k <= j; k++) {
                    draw_pixel(x + r - i, y + r + k, color);
                    draw_pixel(x + w - r + i - 1, y + r + k, color);
                    draw_pixel(x + r - i, y + h - r + k - 1, color);
                    draw_pixel(x + w - r + i - 1, y + h - r + k - 1, color);
                }
            }
        }
    }
}

// Font and text drawing (same as before)
static void draw_char(char c, int16_t x, int16_t y, uint8_t size, bool color) {
    // 5x7 font data (same as before, abbreviated for brevity)
    const uint8_t font5x7[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, // Space
        0x00, 0x00, 0x5F, 0x00, 0x00, // !
        // ... rest of font data
    };
    
    if (c < 32 || c > 127) return;
    
    const uint8_t *glyph = &font5x7[(c - 32) * 5];
    
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (uint8_t j = 0; j < 7; j++) {
            if (line & 0x01) {
                if (size == 1) {
                    draw_pixel(x + i, y + j, color);
                } else {
                    draw_fill_rect(x + i * size, y + j * size, size, size, color);
                }
            }
            line >>= 1;
        }
    }
}

static void draw_text(const char *text, int16_t x, int16_t y, uint8_t size, bool color) {
    int16_t cursor_x = x;
    
    while (*text) {
        draw_char(*text, cursor_x, y, size, color);
        cursor_x += 6 * size;
        text++;
    }
}

static int get_text_width(const char *text, uint8_t size) {
    return strlen(text) * 6 * size;
}

static int find_best_text_size(const char *text, int max_width, int max_height) {
    bool is_combo = strchr(text, '+') != NULL;
    
    for (int size = 4; size >= 1; size--) {
        int text_height = size * 8;
        
        if (text_height > max_height) {
            continue;
        }
        
        int approx_width = 0;
        if (is_combo) {
            const char *ptr = text;
            while (*ptr) {
                const char *next_plus = strchr(ptr, '+');
                size_t part_len = next_plus ? (next_plus - ptr) : strlen(ptr);
                approx_width += part_len * (size * 6);
                if (next_plus) {
                    approx_width += size * 6;
                    ptr = next_plus + 1;
                } else {
                    break;
                }
            }
        } else {
            approx_width = strlen(text) * (size * 6);
        }
        
        if (approx_width <= max_width) {
            return size;
        }
    }
    
    return 1;
}

static void draw_key_display(void) {
    int card_x = 2;
    int card_y = 2;
    int card_w = 124;
    int card_h = CARD_HEIGHT;
    
    // Draw card outline
    draw_round_rect(card_x + 1, card_y + 1, card_w, card_h, 8, true);
    draw_round_rect(card_x, card_y, card_w, card_h, 8, true);
    
    // Inner glow for active key
    uint64_t now = k_uptime_get();
    if (now - display_data.last_key_time < 300) {
        for (int i = 2; i < 4; i++) {
            draw_round_rect(card_x + i, card_y + i, card_w - 2 * i, card_h - 2 * i, 8, true);
        }
    }
    
    // Find best text size
    int max_available_width = card_w - 8;
    int best_size = find_best_text_size(display_data.last_key, max_available_width, MAX_TEXT_HEIGHT);
    bool is_combo = strchr(display_data.last_key, '+') != NULL;
    
    if (is_combo) {
        // Multi-key combination
        char combo[MAX_KEY_LENGTH];
        strncpy(combo, display_data.last_key, sizeof(combo) - 1);
        combo[sizeof(combo) - 1] = '\0';
        
        // Calculate total width
        int total_width = 0;
        const char *ptr = combo;
        
        while (*ptr) {
            const char *next_plus = strchr(ptr, '+');
            size_t part_len = next_plus ? (next_plus - ptr) : strlen(ptr);
            total_width += part_len * (best_size * 6);
            
            if (next_plus) {
                total_width += best_size * 6;
                ptr = next_plus + 1;
            } else {
                break;
            }
        }
        
        // Calculate position
        int start_x = card_x + (card_w - total_width) / 2;
        int text_height = best_size * 8;
        int y_pos = card_y + (card_h - MAX_TEXT_HEIGHT) / 2;
        y_pos += (MAX_TEXT_HEIGHT - text_height) / 2;
        
        // Press animation
        bool fresh_press = (now - display_data.last_key_time < 200);
        if (fresh_press) {
            int padding = best_size;
            draw_fill_round_rect(start_x - padding, y_pos - 1, total_width + (padding * 2), text_height + 2, 2, true);
        }
        
        // Draw combination
        ptr = combo;
        int draw_x = start_x;
        bool draw_inverse = fresh_press;
        
        while (*ptr) {
            const char *next_plus = strchr(ptr, '+');
            size_t part_len = next_plus ? (next_plus - ptr) : strlen(ptr);
            
            char part[MAX_KEY_LENGTH];
            strncpy(part, ptr, part_len);
            part[part_len] = '\0';
            
            if (draw_inverse) {
                int part_width = part_len * (best_size * 6);
                draw_fill_rect(draw_x, y_pos, part_width, text_height, false);
                draw_text(part, draw_x, y_pos, best_size, true);
            } else {
                draw_text(part, draw_x, y_pos, best_size, true);
            }
            
            draw_x += part_len * (best_size * 6);
            
            if (next_plus) {
                if (draw_inverse) {
                    draw_fill_rect(draw_x, y_pos, best_size * 6, text_height, false);
                    draw_text("+", draw_x, y_pos, best_size, true);
                } else {
                    draw_text("+", draw_x, y_pos, best_size, true);
                }
                draw_x += best_size * 6;
                ptr = next_plus + 1;
            } else {
                break;
            }
        }
    } else {
        // Single key
        int text_width = strlen(display_data.last_key) * (best_size * 6);
        int text_height = best_size * 8;
        int center_x = card_x + (card_w - text_width) / 2;
        int center_y = card_y + (card_h - MAX_TEXT_HEIGHT) / 2;
        center_y += (MAX_TEXT_HEIGHT - text_height) / 2;
        
        bool fresh_press = (now - display_data.last_key_time < 200);
        if (fresh_press) {
            int padding = best_size;
            draw_fill_round_rect(center_x - padding, center_y - 1, text_width + (padding * 2), text_height + 2, 2, true);
            draw_fill_rect(center_x, center_y, text_width, text_height, false);
            draw_text(display_data.last_key, center_x, center_y, best_size, true);
        } else {
            draw_text(display_data.last_key, center_x, center_y, best_size, true);
        }
    }
}

static void draw_layer_indicators(void) {
    int indicator_y = 46;
    int indicator_spacing = 25;
    int start_x = 4;
    
    for (int i = 0; i < 5; i++) {
        int x = start_x + (i * indicator_spacing);
        
        if (i == display_data.current_layer) {
            // Active layer
            draw_fill_round_rect(x, indicator_y, 22, 12, 2, true);
            
            char layer_label[3] = {'L', '0' + i, '\0'};
            draw_fill_rect(x + 2, indicator_y + 2, 18, 8, false);
            draw_text(layer_label, x + 2, indicator_y + 2, 1, true);
        } else {
            // Inactive layer
            draw_round_rect(x, indicator_y, 22, 12, 2, true);
            
            char layer_label[3] = {'L', '0' + i, '\0'};
            draw_text(layer_label, x + 2, indicator_y + 2, 1, true);
        }
    }
}

static void update_display_buffer(void) {
    memset(buffer, 0, sizeof(buffer));
    
    draw_key_display();
    draw_layer_indicators();
}

int display_init(void) {
    if (!device_is_ready(i2c_dev)) {
        printk("I2C device not ready\n");
        return -ENODEV;
    }
    
    // Initialize SSD1306
    ssd1306_init_display();
    
    // Set default key
    strcpy(display_data.last_key, "-");
    
    display_data.initialized = true;
    
    // Startup animation
    for (int i = 0; i < 64; i += 2) {
        memset(buffer, 0, sizeof(buffer));
        draw_round_rect(64 - i, 32 - i/2, i*2, i, 3, true);
        
        // Send to display
        ssd1306_command(SSD1306_COLUMNADDR);
        ssd1306_command(0);
        ssd1306_command(SCREEN_WIDTH - 1);
        ssd1306_command(SSD1306_PAGEADDR);
        ssd1306_command(0);
        ssd1306_command(7);
        
        for (size_t idx = 0; idx < sizeof(buffer); idx++) {
            ssd1306_command(buffer[idx]);
        }
        
        k_sleep(K_MSEC(20));
    }
    
    // Show startup text
    memset(buffer, 0, sizeof(buffer));
    draw_text("KEYBOARD", 20, 18, 2, true);
    draw_text("OS v2.5", 35, 38, 1, true);
    draw_round_rect(0, 0, 128, 64, 3, true);
    
    // Send to display
    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(0);
    ssd1306_command(SCREEN_WIDTH - 1);
    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(0);
    ssd1306_command(7);
    
    for (size_t idx = 0; idx < sizeof(buffer); idx++) {
        ssd1306_command(buffer[idx]);
    }
    
    k_sleep(K_MSEC(1500));
    
    return 0;
}

void display_update(void) {
    if (!display_data.initialized) {
        return;
    }
    
    update_display_buffer();
    
    // Send buffer to display
    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(0);
    ssd1306_command(SCREEN_WIDTH - 1);
    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(0);
    ssd1306_command(7);
    
    for (size_t idx = 0; idx < sizeof(buffer); idx++) {
        ssd1306_command(buffer[idx]);
    }
}

void display_set_key(const char *key) {
    if (!display_data.initialized) return;
    
    strncpy(display_data.last_key, key, MAX_KEY_LENGTH - 1);
    display_data.last_key[MAX_KEY_LENGTH - 1] = '\0';
    display_data.last_key_time = k_uptime_get();
    
    display_update();
}

void display_set_layer(uint8_t layer) {
    if (!display_data.initialized) return;
    
    display_data.current_layer = layer % 5;
    display_update();
}