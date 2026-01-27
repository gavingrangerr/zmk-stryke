#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CARD_HEIGHT 36
#define MAX_TEXT_HEIGHT 16
#define MAX_LAYERS 5
#define MAX_POSITIONS 12
#define FLASH_DURATION 150
#define FADE_DURATION 300

static lv_obj_t *screen = NULL;
static lv_obj_t *key_card = NULL;
static lv_obj_t *key_label = NULL;
static lv_obj_t *layer_indicators[5];
static lv_obj_t *glow_rect = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *corner_dots[4];
static uint8_t current_layer = 0;
static uint8_t previous_layer = 0;
static char last_key_text[32] = "---";
static int64_t last_key_time = 0;
static uint32_t key_press_count = 0;

static const char* key_names[MAX_LAYERS][MAX_POSITIONS] = {
    { "CMD+C", "CMD+V", "CMD+X", "CMD+Z", "CMD+A", "CMD+S", 
      "CMD+F", "CMD+SHFT+Z", "CMD+SHFT+3", "CMD+SHFT+4", "Bootloader", "KiCad" },
    { "R", "M", "G", "DEL", "V", "E", "X", "W", "B", "F", "H", "Fusion" },
    { "Cmd+K", "R", "L", "D", "E", "Q", "F", "CMD+Z", 
      "CMD+1", "CMD+2", "CMD+0", "VS Code" },
    { "Cmd+P", "CMD+SHFT+P", "CMD+B", "CMD+FSLH", "CMD+D", "CMD+SHFT+L", 
      "CMD+F", "CMD+Z", "CMD+SQRT", "CMD+SHFT+F", "CMD+SHFT+E", "Arduino" },
    { "CMD+R", "CMD+U", "CMD+SHFT+M", "CMD+K", "CMD+T", "CMD+Z", 
      "CMD+L", "CMD+S", "BT Clear", "BT Next", "Reset", "Base" }
};

// Custom patterns for each layer (5x5 pixel icons)
static const uint8_t layer_patterns[5][5] = {
    {0b11111, 0b10001, 0b10001, 0b10001, 0b11111},  // Square
    {0b00100, 0b01110, 0b11111, 0b01110, 0b00100},  // Diamond
    {0b11111, 0b11011, 0b10101, 0b11011, 0b11111},  // Plus
    {0b10101, 0b01110, 0b11111, 0b01110, 0b10101},  // Star
    {0b11111, 0b11111, 0b11111, 0b11111, 0b11111}   // Solid
};

static const char* get_key_name_by_position(uint8_t layer, uint8_t position) {
    if (layer >= MAX_LAYERS || position >= MAX_POSITIONS) {
        return NULL;
    }
    return key_names[layer][position];
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

// Draw custom pixel pattern for layer indicators
static void draw_layer_pattern(lv_obj_t* obj, uint8_t layer) {
    if (obj == NULL || layer >= MAX_LAYERS) return;
    
    lv_obj_clean(obj);
    
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            if (layer_patterns[layer][y] & (1 << (4 - x))) {
                lv_obj_t* dot = lv_obj_create(obj);
                lv_obj_set_size(dot, 2, 2);
                lv_obj_set_pos(dot, x * 3 + 4, y * 2 + 1);
                lv_obj_set_style_bg_color(dot, lv_color_white(), LV_PART_MAIN);
                lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
                lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
                lv_obj_set_style_radius(dot, 0, LV_PART_MAIN);
            }
        }
    }
}

// Update progress bar
static void update_progress_bar(void) {
    if (progress_bar == NULL) return;
    int progress = (key_press_count % 100);
    int bar_width = (progress * 120) / 100;
    lv_obj_set_width(progress_bar, bar_width);
}

// Animate corner dots based on layer
static void animate_corner_dots(void) {
    for (int i = 0; i < 4; i++) {
        if (corner_dots[i] == NULL) continue;
        if (i < current_layer) {
            lv_obj_set_style_bg_opa(corner_dots[i], LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_opa(corner_dots[i], LV_OPA_30, LV_PART_MAIN);
        }
    }
}

static void update_key_display(void) {
    if (key_label == NULL) return;

    lv_label_set_text(key_label, last_key_text);
    int best_size = find_best_text_size(last_key_text, 116, MAX_TEXT_HEIGHT);
    const lv_font_t* font = get_font_for_size(best_size);
    lv_obj_set_style_text_font(key_label, font, LV_PART_MAIN);

    int64_t now = k_uptime_get();
    int64_t time_since_press = now - last_key_time;
    
    // Smooth fade animation
    if (time_since_press < FLASH_DURATION) {
        // Flash state - inverted colors
        lv_obj_set_style_bg_color(key_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(key_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_pad_all(key_label, 4, LV_PART_MAIN);
        lv_obj_set_style_radius(key_label, 4, LV_PART_MAIN);
    } else if (time_since_press < FADE_DURATION) {
        // Fading out
        int fade_progress = ((time_since_press - FLASH_DURATION) * 255) / (FADE_DURATION - FLASH_DURATION);
        lv_opa_t opa = 255 - fade_progress;
        lv_obj_set_style_bg_opa(key_label, opa, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_pad_all(key_label, 4, LV_PART_MAIN);
        lv_obj_set_style_radius(key_label, 4, LV_PART_MAIN);
    } else {
        // Normal state
        lv_obj_set_style_bg_opa(key_label, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_pad_all(key_label, 0, LV_PART_MAIN);
    }
    
    // Fade glow effect
    if (glow_rect != NULL && time_since_press < FADE_DURATION) {
        int fade_progress = (time_since_press * 30) / FADE_DURATION;
        lv_opa_t opa = 30 - fade_progress;
        lv_obj_set_style_bg_opa(glow_rect, opa, LV_PART_MAIN);
    } else if (glow_rect != NULL) {
        lv_obj_set_style_bg_opa(glow_rect, LV_OPA_TRANSP, LV_PART_MAIN);
    }
    
    // Reset card border after bounce
    if (key_card != NULL && time_since_press > 100) {
        lv_obj_set_style_border_width(key_card, 2, LV_PART_MAIN);
    }

    lv_obj_center(key_label);
    lv_obj_invalidate(key_label);
    lv_obj_invalidate(key_card);
    lv_obj_invalidate(screen);
}

static void update_layer_indicators(void) {
    for (int i = 0; i < 5; i++) {
        if (layer_indicators[i] == NULL) continue;

        if (i == current_layer) {
            // Active layer - show pattern
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(layer_indicators[i], 2, LV_PART_MAIN);
            lv_obj_set_style_border_opa(layer_indicators[i], LV_OPA_COVER, LV_PART_MAIN);
            draw_layer_pattern(layer_indicators[i], i);
        } else if (i == previous_layer) {
            // Previously active - dim outline
            lv_obj_clean(layer_indicators[i]);
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(layer_indicators[i], 1, LV_PART_MAIN);
            lv_obj_set_style_border_opa(layer_indicators[i], LV_OPA_70, LV_PART_MAIN);
        } else {
            // Inactive - faint outline
            lv_obj_clean(layer_indicators[i]);
            lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(layer_indicators[i], 1, LV_PART_MAIN);
            lv_obj_set_style_border_opa(layer_indicators[i], LV_OPA_30, LV_PART_MAIN);
        }
    }
    
    animate_corner_dots();
}

static void create_ui(void) {
    if (screen == NULL) return;

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    // Glow effect behind card
    glow_rect = lv_obj_create(screen);
    lv_obj_set_size(glow_rect, 128, CARD_HEIGHT + 4);
    lv_obj_set_pos(glow_rect, 0, 1);
    lv_obj_set_style_bg_color(glow_rect, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(glow_rect, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(glow_rect, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(glow_rect, 0, LV_PART_MAIN);

    // Main key card
    key_card = lv_obj_create(screen);
    lv_obj_set_size(key_card, 124, CARD_HEIGHT);
    lv_obj_set_pos(key_card, 2, 2);
    lv_obj_set_style_bg_opa(key_card, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(key_card, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(key_card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(key_card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(key_card, 0, LV_PART_MAIN);

    // Key label
    key_label = lv_label_create(key_card);
    lv_label_set_text(key_label, last_key_text);
    lv_obj_set_style_text_color(key_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(key_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(key_label);

    // Corner dots
    int dot_positions[4][2] = {{1, 40}, {125, 40}, {1, 62}, {125, 62}};
    for (int i = 0; i < 4; i++) {
        corner_dots[i] = lv_obj_create(screen);
        lv_obj_set_size(corner_dots[i], 2, 2);
        lv_obj_set_pos(corner_dots[i], dot_positions[i][0], dot_positions[i][1]);
        lv_obj_set_style_bg_color(corner_dots[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(corner_dots[i], LV_OPA_30, LV_PART_MAIN);
        lv_obj_set_style_border_width(corner_dots[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(corner_dots[i], 1, LV_PART_MAIN);
    }

    // Progress bar
    lv_obj_t* progress_bg = lv_obj_create(screen);
    lv_obj_set_size(progress_bg, 120, 2);
    lv_obj_set_pos(progress_bg, 4, 61);
    lv_obj_set_style_bg_color(progress_bg, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bg, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bg, 1, LV_PART_MAIN);

    progress_bar = lv_obj_create(screen);
    lv_obj_set_size(progress_bar, 0, 2);
    lv_obj_set_pos(progress_bar, 4, 61);
    lv_obj_set_style_bg_color(progress_bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 1, LV_PART_MAIN);

    // Layer indicators with custom patterns
    int indicator_y = 43;
    int indicator_spacing = 25;
    int start_x = 4;

    for (int i = 0; i < 5; i++) {
        int x = start_x + (i * indicator_spacing);
        layer_indicators[i] = lv_obj_create(screen);
        lv_obj_set_size(layer_indicators[i], 22, 16);
        lv_obj_set_pos(layer_indicators[i], x, indicator_y);
        lv_obj_set_style_radius(layer_indicators[i], 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(layer_indicators[i], lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_width(layer_indicators[i], 1, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(layer_indicators[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_pad_all(layer_indicators[i], 0, LV_PART_MAIN);
    }

    update_layer_indicators();
    update_key_display();
    update_progress_bar();
}

static void animation_timer_cb(lv_timer_t* timer) {
    update_key_display();
}

static int layer_state_changed_cb(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) return 0;

    previous_layer = current_layer;
    current_layer = zmk_keymap_highest_layer_active();
    if (current_layer > 4) current_layer = 4;
    
    update_layer_indicators();
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
            key_press_count++;
            
            // Trigger animations
            if (glow_rect) lv_obj_set_style_bg_opa(glow_rect, LV_OPA_30, LV_PART_MAIN);
            if (key_card) lv_obj_set_style_border_width(key_card, 3, LV_PART_MAIN);
            
            update_key_display();
            update_progress_bar();
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
        previous_layer = current_layer;
        update_layer_indicators();
    }
    return screen;
}

#ifdef __cplusplus
}
#endif