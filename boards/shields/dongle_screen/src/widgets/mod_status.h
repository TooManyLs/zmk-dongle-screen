#pragma once

#include <lvgl.h>
#include <zmk/display.h>

struct zmk_widget_mod_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *ctrl_label;
    lv_obj_t *shift_label;
    lv_obj_t *alt_label;
    lv_obj_t *gui_label;
};

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);