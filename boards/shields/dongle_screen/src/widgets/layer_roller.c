#include "layer_roller.h"

#include <ctype.h>
#include <string.h>
#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include <lvgl.h>  // Critical: Explicit LVGL include for roller APIs
#include <fonts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static char layer_names_buffer[256] = {0}; // Buffer for concatenated layer names
static int layer_select_id[ZMK_KEYMAP_LAYERS_LEN] = {0};   // Maps layer index to display position
static int layer_display_order[ZMK_KEYMAP_LAYERS_LEN] = {0}; // Maps display position to layer index
static int total_layers = 0;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_roller_state {
    uint8_t index;
};

static void layer_roller_set_sel(lv_obj_t *roller, struct layer_roller_state state) {
    if (state.index >= ZMK_KEYMAP_LAYERS_LEN || layer_select_id[state.index] == -1) {
        return;
    }

    int display_pos = layer_select_id[state.index];

    // Set color based on position and layer
    lv_color_t color;
    // Get relative position from center
    int center_pos = total_layers / 2;
    int rel_pos = display_pos - center_pos;

    // Position-based coloring using predefined colors
    if (display_pos == center_pos) {
        // Center (usually layer 0) - White
        color = lv_color_white();
    } else {
        // Colors for positions relative to center
        static const lv_palette_t before_center[] = {
            LV_PALETTE_DEEP_ORANGE,  // -3
            LV_PALETTE_ORANGE,       // -2
            LV_PALETTE_AMBER,        // -1
        };
        static const lv_palette_t after_center[] = {
            LV_PALETTE_LIGHT_GREEN,  // +1
            LV_PALETTE_GREEN,        // +2
            LV_PALETTE_TEAL,         // +3
        };
        
        if (rel_pos < 0) {
            // Before center (negative positions)
            int idx = (-rel_pos) - 1;
            if (idx < sizeof(before_center)/sizeof(before_center[0])) {
                color = lv_palette_main(before_center[idx]);
            } else {
                color = lv_palette_main(before_center[sizeof(before_center)/sizeof(before_center[0]) - 1]);
            }
        } else {
            // After center (positive positions)
            int idx = rel_pos - 1;
            if (idx < sizeof(after_center)/sizeof(after_center[0])) {
                color = lv_palette_main(after_center[idx]);
            } else {
                color = lv_palette_main(after_center[sizeof(after_center)/sizeof(after_center[0]) - 1]);
            }
        }
    }

    lv_obj_set_style_text_color(roller, color, LV_PART_SELECTED);
    lv_roller_set_selected(roller, display_pos, LV_ANIM_ON);
}

// CRITICAL FIX: Use 4-argument macro signature (pre-widget-refactor ZMK)
static void layer_roller_update_cb(struct layer_roller_state state) {
    struct zmk_widget_layer_roller *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        layer_roller_set_sel(widget->obj, state);
    }
}

static struct layer_roller_state layer_roller_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_roller_state){
        .index = index,
    };
}

// 4-ARGUMENT MACRO (compatible with current ZMK main branch as of early 2026)
ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_roller, struct layer_roller_state, layer_roller_update_cb,
                            layer_roller_get_state)
ZMK_SUBSCRIPTION(widget_layer_roller, zmk_layer_state_changed);

static void init_layer_arrays(void) {
    static bool initialized = false;
    if (initialized) {
        return; // Already initialized
    }
    initialized = true;

    // Count available layers
    total_layers = 0;
    for (int i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        if (zmk_keymap_layer_name(i) != NULL) {
            total_layers++;
        }
    }

    // Initialize arrays with invalid values
    for (int i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        layer_select_id[i] = -1;
        layer_display_order[i] = -1;
    }

    // Initialize arrays with layer 0 in the center
    int center_pos = total_layers / 2;
    
    // Place layer 0 in the center
    layer_display_order[center_pos] = 0;
    layer_select_id[0] = center_pos;
    
    // Fill positions before center with odd numbers
    int odd = 1;
    for (int i = center_pos - 1; i >= 0; i--) {
        if (odd < total_layers) {
            layer_display_order[i] = odd;
            layer_select_id[odd] = i;
            odd += 2;
        }
    }
    
    // Fill positions after center with even numbers
    int even = 2;
    for (int i = center_pos + 1; i < total_layers; i++) {
        if (even < total_layers) {
            layer_display_order[i] = even;
            layer_select_id[even] = i;
            even += 2;
        }
    }

    LOG_DBG("Layer display order:");
    for (int i = 0; i < total_layers; i++) {
        LOG_DBG("Position %d: Layer %d", i, layer_display_order[i]);
    }
}

int zmk_widget_layer_roller_init(struct zmk_widget_layer_roller *widget, lv_obj_t *parent) {
    init_layer_arrays();

    widget->obj = lv_roller_create(parent);
    lv_obj_set_size(widget->obj, 240, 80);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_text_line_space(&style, 0);
    lv_style_set_pad_all(&style, 0);
    lv_obj_add_style(widget->obj, &style, 0);

    // LVGL 8: Use text alignment constants (not position alignments)
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_LEFT, LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_SELECTED);
    lv_obj_set_style_text_font(widget->obj, &lv_font_montserrat_40, LV_PART_SELECTED);   
    lv_obj_set_style_text_color(widget->obj, lv_color_white(), LV_PART_SELECTED);

    // Set the text size and color of the non-selected layers.
    lv_obj_set_style_text_font(widget->obj, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->obj, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);

    // SAFER buffer handling with size tracking
    layer_names_buffer[0] = '\0';
    size_t remaining = sizeof(layer_names_buffer);
    char *ptr = layer_names_buffer;

    for (int i = 0; i < total_layers; i++) {
        const char *layer_name = zmk_keymap_layer_name(layer_display_order[i]);
        if (!layer_name || !*layer_name) {
            // Use layer index if no name
            char index_str[4]; // Support up to 3 digits + null terminator
            snprintf(index_str, sizeof(index_str), "%d", layer_display_order[i]);
            layer_name = index_str;
        }

        // Add newline before all but first entry
        if (i > 0) {
            if (remaining > 1) {
                *ptr++ = '\n';
                *ptr = '\0';
                remaining--;
            }
        }

        // Safely copy layer name (with optional uppercase conversion)
        size_t name_len = strlen(layer_name);
        size_t to_copy = (name_len < remaining - 1) ? name_len : remaining - 1;
        
#if IS_ENABLED(CONFIG_LAYER_ROLLER_ALL_CAPS)
        for (size_t j = 0; j < to_copy; j++) {
            ptr[j] = toupper((unsigned char)layer_name[j]);
        }
#else
        memcpy(ptr, layer_name, to_copy);
#endif
        
        ptr[to_copy] = '\0';
        ptr += to_copy;
        remaining -= to_copy;
        
        if (remaining <= 1) {
            LOG_WRN("Layer names buffer truncated");
            break;
        }
    }

    lv_roller_set_options(widget->obj, layer_names_buffer, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(widget->obj, 3);
    
    lv_obj_set_style_anim_time(widget->obj, 400, 0);
    
    sys_slist_append(&widgets, &widget->node);
    
    widget_layer_roller_init(); // Register widget with event system
    return 0;
}

lv_obj_t *zmk_widget_layer_roller_obj(struct zmk_widget_layer_roller *widget) {
    return widget->obj;
}