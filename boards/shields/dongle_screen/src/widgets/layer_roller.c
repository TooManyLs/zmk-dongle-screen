#include "layer_roller.h"

#include <ctype.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include <fonts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zmk_layer_roller, CONFIG_ZMK_LOG_LEVEL);

/* Config */
#define MAX_LAYER_NAME_BUFFER 256

static char layer_names_buffer[MAX_LAYER_NAME_BUFFER];
static int layer_select_id[ZMK_KEYMAP_LAYERS_LEN];
static int layer_display_order[ZMK_KEYMAP_LAYERS_LEN];
static int total_layers = 0;

/* widget list */
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_roller_state {
    uint8_t index;
};

static void layer_roller_set_sel(lv_obj_t *roller, struct layer_roller_state state)
{
    if (state.index >= ZMK_KEYMAP_LAYERS_LEN) {
        return;
    }

    int sel_layer = state.index;
    if (layer_select_id[sel_layer] == -1) {
        return;
    }

    int display_pos = layer_select_id[sel_layer];

    /* choose color based on relative position to center */
    lv_color_t color;
    int center_pos = total_layers / 2;
    int rel_pos = display_pos - center_pos;

    if (display_pos == center_pos) {
        color = lv_color_white();
    } else {
        static const lv_palette_t before_center[] = {
            LV_PALETTE_DEEP_ORANGE,
            LV_PALETTE_ORANGE,
            LV_PALETTE_AMBER,
        };
        static const lv_palette_t after_center[] = {
            LV_PALETTE_LIGHT_GREEN,
            LV_PALETTE_GREEN,
            LV_PALETTE_TEAL,
        };

        if (rel_pos < 0) {
            int idx = (-rel_pos) - 1;
            size_t max = ARRAY_SIZE(before_center);
            if (idx >= (int)max) idx = (int)max - 1;
            color = lv_palette_main(before_center[idx]);
        } else {
            int idx = rel_pos - 1;
            size_t max = ARRAY_SIZE(after_center);
            if (idx >= (int)max) idx = (int)max - 1;
            color = lv_palette_main(after_center[idx]);
        }
    }

    /* Apply color and select */
    lv_obj_set_style_text_color(roller, color, LV_PART_SELECTED);
    lv_roller_set_selected(roller, display_pos, LV_ANIM_ON);
}

static void layer_roller_update_cb(struct layer_roller_state state)
{
    struct zmk_widget_layer_roller *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget && widget->obj) {
            layer_roller_set_sel(widget->obj, state);
        }
    }
}

static struct layer_roller_state layer_roller_get_state(const zmk_event_t *eh)
{
    ARG_UNUSED(eh);
    uint8_t idx = zmk_keymap_highest_layer_active();
    return (struct layer_roller_state){ .index = idx };
}

/* Register widget listener (macro used in modern ZMK) */
ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_roller, struct layer_roller_state, layer_roller_update_cb,
                            layer_roller_get_state)
ZMK_SUBSCRIPTION(widget_layer_roller, zmk_layer_state_changed);

static void mask_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    static int16_t mask_top_id = -1;
    static int16_t mask_bottom_id = -1;

    if (code == LV_EVENT_COVER_CHECK) {
        lv_event_set_cover_res(e, LV_COVER_RES_MASKED);
    } else if (code == LV_EVENT_DRAW_MAIN_BEGIN) {
        const lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_SELECTED);
        lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
        lv_coord_t font_h = lv_font_get_line_height(font);

        lv_area_t roller_coords;
        lv_obj_get_coords(obj, &roller_coords);

        lv_area_t rect_area;
        rect_area.x1 = roller_coords.x1;
        rect_area.x2 = roller_coords.x2;
        rect_area.y1 = roller_coords.y1;
        rect_area.y2 = roller_coords.y1 + (lv_obj_get_height(obj) - font_h) / 2;

        lv_draw_mask_fade_param_t *fade_mask_top = lv_mem_alloc(sizeof(lv_draw_mask_fade_param_t));
        if (fade_mask_top) {
            lv_draw_mask_fade_init(fade_mask_top, &rect_area, LV_OPA_TRANSP, rect_area.y1, LV_OPA_COVER, rect_area.y2);
            mask_top_id = lv_draw_mask_add(fade_mask_top, NULL);
        }

        rect_area.y1 = rect_area.y2 + font_h + line_space - 1;
        rect_area.y2 = roller_coords.y2;

        lv_draw_mask_fade_param_t *fade_mask_bottom = lv_mem_alloc(sizeof(lv_draw_mask_fade_param_t));
        if (fade_mask_bottom) {
            lv_draw_mask_fade_init(fade_mask_bottom, &rect_area, LV_OPA_COVER, rect_area.y1, LV_OPA_TRANSP, rect_area.y2);
            mask_bottom_id = lv_draw_mask_add(fade_mask_bottom, NULL);
        }
    } else if (code == LV_EVENT_DRAW_POST_END) {
        if (mask_top_id >= 0) {
            lv_draw_mask_fade_param_t *p = lv_draw_mask_remove_id(mask_top_id);
            if (p) {
                lv_draw_mask_free_param(p);
                lv_mem_free(p);
            }
            mask_top_id = -1;
        }
        if (mask_bottom_id >= 0) {
            lv_draw_mask_fade_param_t *p = lv_draw_mask_remove_id(mask_bottom_id);
            if (p) {
                lv_draw_mask_free_param(p);
                lv_mem_free(p);
            }
            mask_bottom_id = -1;
        }
    }
}

static void init_layer_arrays(void)
{
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    /* Count available layers */
    total_layers = 0;
    for (int i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        const char *name = zmk_keymap_layer_name(i);
        if (name != NULL) {
            total_layers++;
        }
    }

    if (total_layers <= 0) {
        total_layers = 1;
    }

    /* initialize maps */
    for (int i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        layer_select_id[i] = -1;
        layer_display_order[i] = -1;
    }

    int center_pos = total_layers / 2;

    /* Put layer 0 in center */
    layer_display_order[center_pos] = 0;
    layer_select_id[0] = center_pos;

    /* Fill before center with odd numbers (1,3,5...) */
    int odd = 1;
    for (int i = center_pos - 1; i >= 0; i--) {
        if (odd < total_layers) {
            layer_display_order[i] = odd;
            layer_select_id[odd] = i;
            odd += 2;
        }
    }

    /* Fill after center with even numbers (2,4,6...) */
    int even = 2;
    for (int i = center_pos + 1; i < total_layers; i++) {
        if (even < total_layers) {
            layer_display_order[i] = even;
            layer_select_id[even] = i;
            even += 2;
        }
    }

    LOG_DBG("Layer display order (total %d):", total_layers);
    for (int i = 0; i < total_layers; i++) {
        LOG_DBG("Position %d: Layer %d", i, layer_display_order[i]);
    }
}

int zmk_widget_layer_roller_init(struct zmk_widget_layer_roller *widget, lv_obj_t *parent)
{
    init_layer_arrays();

    if (!widget) {
        return -EINVAL;
    }

    widget->obj = lv_roller_create(parent);
    if (!widget->obj) {
        return -ENOMEM;
    }

    lv_obj_set_size(widget->obj, 240, 80);

    static lv_style_t style_main;
    lv_style_init(&style_main);
    lv_style_set_bg_color(&style_main, lv_color_black());
    lv_style_set_text_color(&style_main, lv_color_white());
    lv_style_set_text_line_space(&style_main, 0);
    lv_style_set_pad_all(&style_main, 0);
    lv_obj_add_style(widget->obj, &style_main, 0);

    /* Selected part styles */
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_LEFT, LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_SELECTED);
    lv_obj_set_style_text_font(widget->obj, &lv_font_montserrat_40, LV_PART_SELECTED);
    lv_obj_set_style_text_color(widget->obj, lv_color_white(), LV_PART_SELECTED);

    /* Main part styles */
    lv_obj_set_style_text_font(widget->obj, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->obj, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);

    /* Build layer names safely */
    layer_names_buffer[0] = '\0';
    char *ptr = layer_names_buffer;
    size_t rem = sizeof(layer_names_buffer);

    for (int i = 0; i < total_layers; i++) {
        int layer_idx = layer_display_order[i];
        const char *layer_name = zmk_keymap_layer_name(layer_idx);
        if (!layer_name) {
            /* fallback to numeric label */
            char tmp[4];
            int n = snprintf(tmp, sizeof(tmp), "%d", layer_idx);
            if (n > 0 && (size_t)n < rem) {
                if (i > 0) {
                    strncat(ptr, "\n", rem - strlen(ptr) - 1);
                }
                strncat(ptr, tmp, rem - strlen(ptr) - 1);
                rem = sizeof(layer_names_buffer) - strlen(ptr);
            }
            continue;
        }

        if (i > 0) {
            if (rem > 1) {
                strncat(ptr, "\n", rem - strlen(ptr) - 1);
                rem = sizeof(layer_names_buffer) - strlen(ptr);
            }
        }

#if IS_ENABLED(CONFIG_LAYER_ROLLER_ALL_CAPS)
        /* copy uppercase */
        for (const char *c = layer_name; *c && rem > 1; c++) {
            *ptr++ = (char)toupper((unsigned char)*c);
            rem--;
        }
        if (rem > 0) {
            *ptr = '\0';
            rem = sizeof(layer_names_buffer) - strlen(layer_names_buffer);
        }
#else
        strncat(ptr, layer_name, rem - strlen(ptr) - 1);
        rem = sizeof(layer_names_buffer) - strlen(layer_names_buffer);
#endif
    }

    lv_roller_set_options(widget->obj, layer_names_buffer, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(widget->obj, 3);

    /* optionally enable mask event if desired */
    lv_obj_add_event_cb(widget->obj, mask_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_set_style_anim_time(widget->obj, 400, 0);

    sys_slist_append(&widgets, &widget->node);

    /* register listener so ZMK updates this widget automatically */
    widget_layer_roller_init();

    return 0;
}

lv_obj_t *zmk_widget_layer_roller_obj(struct zmk_widget_layer_roller *widget)
{
    return widget ? widget->obj : NULL;
}
