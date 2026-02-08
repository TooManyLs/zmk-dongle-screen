#include "background.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct background_widget *background_widget_init(lv_obj_t *parent, const lv_img_dsc_t *img_dsc)
{
    struct background_widget *widget = k_malloc(sizeof(struct background_widget));
    if (!widget) {
        LOG_ERR("Failed to allocate background widget");
        return NULL;
    }

    widget->obj = lv_image_create(parent);
    lv_image_set_src(widget->obj, img_dsc);
    lv_obj_set_size(widget->obj, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(widget->obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_to_index(widget->obj, 0); // Force to bottom layer

    LOG_INF("Background widget initialized");
    return widget;
}