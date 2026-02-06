/*
Copyright (c) 2020 The ZMK Contributors
SPDX-License-Identifier: MIT
*/
#include "layer_roller.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <string.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_roller_state {
    uint8_t index;
    uint8_t total_layers;
};

static void populate_roller_options(lv_obj_t *roller) {
    char opts[256] = {0};
    char *ptr = opts;
    size_t remaining = sizeof(opts);
    
    for (int i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        const char *label = zmk_keymap_layer_name(i);
        if (label != NULL) {
            int written = snprintf(ptr, remaining, "%s\n", label);
            if (written < 0 || written >= remaining) break;
            ptr += written;
            remaining -= written;
        } else {
            int written = snprintf(ptr, remaining, "Layer %d\n", i);
            if (written < 0 || written >= remaining) break;
            ptr += written;
            remaining -= written;
        }
    }
    
    // Remove trailing newline
    if (ptr > opts) {
        *(ptr - 1) = '\0';
    }
    
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
}

static void layer_roller_update_cb(struct layer_roller_state state) {
    struct zmk_widget_layer_roller *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        lv_roller_set_selected(widget->obj, state.index, LV_ANIM_ON);
    }
}

static struct layer_roller_state layer_roller_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_roller_state){
        .index = index,
        .total_layers = ZMK_KEYMAP_LAYERS_LEN
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_roller, struct layer_roller_state, 
                           layer_roller_update_cb, layer_roller_get_state)
ZMK_SUBSCRIPTION(widget_layer_roller, zmk_layer_state_changed);

int zmk_widget_layer_roller_init(struct zmk_widget_layer_roller *widget, lv_obj_t *parent) {
    widget->obj = lv_roller_create(parent);
    
    // Configure roller appearance
    lv_obj_set_style_text_font(widget->obj, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(widget->obj, lv_color_white(), 0);
    lv_obj_set_style_bg_color(widget->obj, lv_color_hex(0x333333), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_50, LV_PART_SELECTED);
    
    // Set roller size
    lv_obj_set_width(widget->obj, LV_SIZE_CONTENT);
    lv_obj_set_height(widget->obj, 80); // Adjust height as needed
    
    // Set visible row count
    lv_roller_set_visible_row_count(widget->obj, 3);
    
    // Populate with layer options
    populate_roller_options(widget->obj);
    
    sys_slist_append(&widgets, &widget->node);
    widget_layer_roller_init();
    
    return 0;
}

lv_obj_t *zmk_widget_layer_roller_obj(struct zmk_widget_layer_roller *widget) {
    return widget->obj;
}