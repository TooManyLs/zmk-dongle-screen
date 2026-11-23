#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct mod_status_state {
    uint8_t mods;
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void mod_status_update_cb(struct mod_status_state state)
{
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        lv_obj_set_style_text_color(widget->ctrl_label,
            (state.mods & (MOD_LCTL | MOD_RCTL)) ? lv_color_hex(0x00a8ff) : lv_color_hex(0x909090),
            LV_PART_MAIN);

        lv_obj_set_style_text_color(widget->shift_label,
            (state.mods & (MOD_LSFT | MOD_RSFT)) ? lv_color_hex(0x00a8ff) : lv_color_hex(0x909090),
            LV_PART_MAIN);

        lv_obj_set_style_text_color(widget->alt_label,
            (state.mods & (MOD_LALT | MOD_RALT)) ? lv_color_hex(0x00a8ff) : lv_color_hex(0x909090),
            LV_PART_MAIN);

        lv_obj_set_style_text_color(widget->gui_label,
            (state.mods & (MOD_LGUI | MOD_RGUI)) ? lv_color_hex(0x00a8ff) : lv_color_hex(0x909090),
            LV_PART_MAIN);
    }
}

static struct mod_status_state mod_status_get_state(const zmk_event_t *eh)
{
    const struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    uint8_t mods = (report != NULL) ? report->body.modifiers : 0x00;
    return (struct mod_status_state){ .mods = mods };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mod_status, struct mod_status_state,
                            mod_status_update_cb, mod_status_get_state)

ZMK_SUBSCRIPTION(widget_mod_status, zmk_keycode_state_changed);

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);
    lv_obj_set_style_bg_opa(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    widget->ctrl_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->ctrl_label, "󰘴");
    lv_obj_set_style_text_font(widget->ctrl_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->ctrl_label, lv_color_hex(0x909090), LV_PART_MAIN);

    widget->shift_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->shift_label, "󰘶");
    lv_obj_set_style_text_font(widget->shift_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->shift_label, lv_color_hex(0x909090), LV_PART_MAIN);

    widget->alt_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->alt_label, "󰘵");
    lv_obj_set_style_text_font(widget->alt_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->alt_label, lv_color_hex(0x909090), LV_PART_MAIN);

#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
    lv_label_set_text(widget->gui_label = lv_label_create(widget->obj), "󰌽");
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
    lv_label_set_text(widget->gui_label = lv_label_create(widget->obj), "");
#else
    lv_label_set_text(widget->gui_label = lv_label_create(widget->obj), "󰘳");
#endif
    lv_obj_set_style_text_font(widget->gui_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->gui_label, lv_color_hex(0x909090), LV_PART_MAIN);

    sys_slist_append(&widgets, &widget->node);

    widget_mod_status_init();

    return 0;
}

lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
