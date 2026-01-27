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
#define CARD_HEIGHT 36
#define MAX_TEXT_HEIGHT 16
#define MAX_LAYERS 5
#define MAX_POSITIONS 12

static lv_obj_t *screen = NULL;
static lv_obj_t *key_card = NULL;
static lv_obj_t *key_label = NULL;
static lv_obj_t *layer_indicators[5];

static uint8_t current_layer = 0;
static char last_key_text[32] = "---";
static int64_t last_key_time = 0;

static const char* key_names[MAX_LAYERS][MAX_POSITIONS] = {
    {
        "Copy",         "Paste",        "Cut",          "Undo",
        "Select All",   "Save",         "Find",         "Redo",
        "SS Full",      "SS Snip",      "Bootloader",   "KiCad"
    },
    {
        "Rotate",       "Move",         "Grab",         "Delete",
        "Via",          "Edit Value",   "Exchange",     "Wire",
        "Bus",          "Fill Zone",    "Highlight",    "Fusion"
    },
    {
        "Cmd+K",        "Rotate",       "Line",         "Dimension",
        "Extrude",      "Sketch",       "Fillet",       "Undo",
        "Tab 1",        "Tab 2",        "Zoom All",     "VS Code"
    },
    {
        "Cmd Palette",  "Show Cmds",    "Toggle Bar",   "Comment",
        "Add Cursor",   "Sel Next",     "Find",         "Undo",
        "Terminal",     "Find Files",   "Extensions",   "Arduino"
    },
    {
        "Upload",       "Upload USB",   "Monitor",      "Cmd+K",
        "New Tab",      "Undo",         "Line Ref",     "Save",
        "BT Clear",     "BT Next",      "Reset",        "Base"
    }
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
        update_layer_indicators();
    }
    
    return screen;
}

#ifdef __cplusplus
}
#endif