/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

#include <fonts.h>

#include "output_status.h"

#define OUTPUT_ICON_USB "\U000F0553"
#define OUTPUT_ICON_BLE "\uF293"

#define OUTPUT_RIGHT_MARGIN 10
#define OUTPUT_PROFILE_WIDTH 14
#define OUTPUT_PROFILE_GAP 3
#define OUTPUT_ICON_RIGHT_X (-(OUTPUT_RIGHT_MARGIN + OUTPUT_PROFILE_WIDTH + OUTPUT_PROFILE_GAP))
#define OUTPUT_ROW_USB_Y 8
#define OUTPUT_ROW_BLE_Y 30

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

lv_point_t selection_line_points[] = {{0, 0}, {13, 0}}; // will be replaced with lv_point_precise_t

struct output_status_state
{
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

static struct output_status_state get_state(const zmk_event_t *_eh)
{
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoint_get_selected(),                  // 0 = USB , 1 = BLE
        .active_profile_index = zmk_ble_active_profile_index(),            // 0-3 BLE profiles
        .active_profile_connected = zmk_ble_active_profile_is_connected(), // 0 = not connected, 1 = connected
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),        // 0 =  BLE not bonded, 1 = bonded
        .usb_is_hid_ready = zmk_usb_is_hid_ready()};                       // 0 = not ready, 1 = ready
}

static void set_status_symbol(struct zmk_widget_output_status *widget, struct output_status_state state)
{
    lv_color_t usb_color = state.usb_is_hid_ready ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xFF0000);
    lv_color_t ble_color;

    if (state.active_profile_connected == 1)
    {
        ble_color = lv_color_hex(0x00FF00);
    }
    else if (state.active_profile_bonded == 1)
    {
        ble_color = lv_color_hex(0x0000FF);
    }
    else
    {
        ble_color = lv_color_hex(0xFFFFFF);
    }

    lv_obj_set_style_text_color(widget->usb_icon, usb_color, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->ble_icon, ble_color, LV_PART_MAIN);

    lv_obj_add_flag(widget->usb_marker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(widget->ble_marker, LV_OBJ_FLAG_HIDDEN);

    switch (state.selected_endpoint.transport)
    {
    case ZMK_TRANSPORT_USB:
        lv_obj_clear_flag(widget->usb_marker, LV_OBJ_FLAG_HIDDEN);
        break;
    case ZMK_TRANSPORT_BLE:
        lv_obj_clear_flag(widget->ble_marker, LV_OBJ_FLAG_HIDDEN);
        break;
    default:
        break;
    }

    lv_label_set_text_fmt(widget->profile_label, "%d", state.active_profile_index + 1);
}

static void output_status_update_cb(struct output_status_state state)
{
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        set_status_symbol(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);

static lv_obj_t *create_selection_marker(lv_obj_t *parent, lv_obj_t *anchor)
{
    lv_obj_t *marker = lv_label_create(parent);

    lv_label_set_text(marker, ">");
    lv_obj_align_to(marker, anchor, LV_ALIGN_OUT_LEFT_MID, -2, 0);
    lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);

    return marker;
}

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, lv_obj_get_width(parent), 77);

    lv_obj_set_style_bg_opa(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    // USB icon
    widget->usb_icon = lv_label_create(widget->obj);
    lv_label_set_text(widget->usb_icon, OUTPUT_ICON_USB);
    lv_obj_set_style_text_font(widget->usb_icon, &NerdFonts_Regular_20, LV_PART_MAIN);
    lv_obj_align(widget->usb_icon, LV_ALIGN_TOP_RIGHT, OUTPUT_ICON_RIGHT_X, OUTPUT_ROW_USB_Y);

    // Bluetooth icon
    widget->ble_icon = lv_label_create(widget->obj);
    lv_label_set_text(widget->ble_icon, OUTPUT_ICON_BLE);
    lv_obj_set_style_text_font(widget->ble_icon, &NerdFonts_Regular_20, LV_PART_MAIN);
    lv_obj_align(widget->ble_icon, LV_ALIGN_TOP_RIGHT, OUTPUT_ICON_RIGHT_X, OUTPUT_ROW_BLE_Y);

    widget->profile_label = lv_label_create(widget->obj);
    lv_obj_set_width(widget->profile_label, OUTPUT_PROFILE_WIDTH);
    lv_obj_set_style_text_align(widget->profile_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align_to(widget->profile_label, widget->ble_icon, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    widget->usb_marker = create_selection_marker(widget->obj, widget->usb_icon);
    widget->ble_marker = create_selection_marker(widget->obj, widget->ble_icon);

    sys_slist_append(&widgets, &widget->node);

    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget)
{
    return widget->obj;
}
