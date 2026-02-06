#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h> // <-- Wichtig für LV_FONT_DECLARE
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    char text[256] = "";
    int idx = 0;

    // Ctrl symbol (always shown)
    const char *ctrl_color = (mods & (MOD_LCTL | MOD_RCTL)) ? "#00FFFF" : "#090909";
    idx += snprintf(&text[idx], sizeof(text) - idx, "[color=%s]󰘴[/color]", ctrl_color);
    
    // Shift symbol (always shown)
    const char *shift_color = (mods & (MOD_LSFT | MOD_RSFT)) ? "#00FFFF" : "#090909";
    idx += snprintf(&text[idx], sizeof(text) - idx, " [color=%s]󰘶[/color]", shift_color);
    
    // Alt symbol (always shown)
    const char *alt_color = (mods & (MOD_LALT | MOD_RALT)) ? "#00FFFF" : "#090909";
    idx += snprintf(&text[idx], sizeof(text) - idx, " [color=%s]󰘵[/color]", alt_color);
    
    // GUI symbol (always shown, config-dependent icon)
    const char *gui_color = (mods & (MOD_LGUI | MOD_RGUI)) ? "#00FFFF" : "#090909";
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
    idx += snprintf(&text[idx], sizeof(text) - idx, " [color=%s]󰌽[/color]", gui_color);
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
    idx += snprintf(&text[idx], sizeof(text) - idx, " [color=%s][/color]", gui_color);
#else
    idx += snprintf(&text[idx], sizeof(text) - idx, " [color=%s]󰘳[/color]", gui_color);
#endif

    lv_label_set_text(widget->label, text);
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
    
    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 0);
    
    // CRITICAL: Set default text color to dim (#090909) so spaces blend with background
    lv_obj_set_style_text_color(widget->label, lv_color_hex(0x090909), 0);
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);
    
    // Initialize with all symbols dimmed (no placeholder "-")
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