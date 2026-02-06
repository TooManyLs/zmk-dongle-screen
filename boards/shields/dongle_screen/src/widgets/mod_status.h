#pragma once

#include <lvgl.h>
#include <zmk/display.h>

struct zmk_widget_mod_status {
    lv_obj_t *obj;
};

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);