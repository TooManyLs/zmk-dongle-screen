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
    char text[64] = "";

    // Determine colors for each modifier (6-digit hex without # prefix)
    const char *ctrl_color  = (mods & (MOD_LCTL | MOD_RCTL)) ? "00ffff" : "090909";
    const char *shift_color = (mods & (MOD_LSFT | MOD_RSFT)) ? "00ffff" : "090909";
    const char *alt_color   = (mods & (MOD_LALT | MOD_RALT)) ? "00ffff" : "090909";
    const char *gui_color   = (mods & (MOD_LGUI | MOD_RGUI)) ? "00ffff" : "090909";

    // Select GUI symbol based on config
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
    const char *gui_sym = "󰌽";
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
    const char *gui_sym = "";
#else
    const char *gui_sym = "󰘳";
#endif

    // Build rich text string with ALL symbols always visible
    // LVGL syntax: "#rrggbb text#" - space after color code is required
    snprintf(text, sizeof(text),
        "#%s 󰘴##%s 󰘶##%s 󰘵##%s %s#",
        ctrl_color, shift_color, alt_color, gui_color, gui_sym);

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
    
    // CRITICAL: Do NOT set a default text color style - rich text controls all coloring
    // lv_obj_set_style_text_color(widget->label, ..., 0); // <-- REMOVE THIS IF PRESENT
    
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);
    
    // Initialize with current state (no placeholder "-")
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