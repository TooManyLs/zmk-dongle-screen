#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static lv_obj_t *ctrl_label;
static lv_obj_t *shift_label;
static lv_obj_t *alt_label;
static lv_obj_t *gui_label;

static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;

    lv_obj_set_style_text_color(ctrl_label,
        (mods & (MOD_LCTL | MOD_RCTL)) ? lv_color_hex(0x00FFFF) : lv_color_hex(0x090909),
        LV_PART_MAIN);

    lv_obj_set_style_text_color(shift_label,
        (mods & (MOD_LSFT | MOD_RSFT)) ? lv_color_hex(0x00FFFF) : lv_color_hex(0x090909),
        LV_PART_MAIN);

    lv_obj_set_style_text_color(alt_label,
        (mods & (MOD_LALT | MOD_RALT)) ? lv_color_hex(0x00FFFF) : lv_color_hex(0x090909),
        LV_PART_MAIN);

    lv_obj_set_style_text_color(gui_label,
        (mods & (MOD_LGUI | MOD_RGUI)) ? lv_color_hex(0x00FFFF) : lv_color_hex(0x090909),
        LV_PART_MAIN);
}

static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

static struct k_timer mod_status_timer;

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);
    lv_obj_set_style_bg_opa(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    ctrl_label = lv_label_create(widget->obj);
    lv_label_set_text(ctrl_label, "󰘴");
    lv_obj_set_style_text_font(ctrl_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctrl_label, lv_color_hex(0x090909), LV_PART_MAIN);

    shift_label = lv_label_create(widget->obj);
    lv_label_set_text(shift_label, "󰘶");
    lv_obj_set_style_text_font(shift_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(shift_label, lv_color_hex(0x090909), LV_PART_MAIN);

    alt_label = lv_label_create(widget->obj);
    lv_label_set_text(alt_label, "󰘵");
    lv_obj_set_style_text_font(alt_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(alt_label, lv_color_hex(0x090909), LV_PART_MAIN);

#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
    lv_label_set_text(gui_label = lv_label_create(widget->obj), "󰌽");
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
    lv_label_set_text(gui_label = lv_label_create(widget->obj), "");
#else
    lv_label_set_text(gui_label = lv_label_create(widget->obj), "󰘳");
#endif
    lv_obj_set_style_text_font(gui_label, &NerdFonts_Regular_40, LV_PART_MAIN);
    lv_obj_set_style_text_color(gui_label, lv_color_hex(0x090909), LV_PART_MAIN);

    update_mod_status(widget);

    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    return 0;
}

lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}