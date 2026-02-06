/*
Copyright (c) 2020 The ZMK Contributors
SPDX-License-Identifier: MIT
*/
#pragma once

#include <zephyr/kernel.h>
#include <lvgl.h>

struct zmk_widget_layer_roller {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_widget_layer_roller_init(struct zmk_widget_layer_roller *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_layer_roller_obj(struct zmk_widget_layer_roller *widget);