#include "background.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

void background_widget_init(lv_obj_t *parent, const void *img_dsc)
{
    if (!parent || !img_dsc) {
        LOG_ERR("Invalid parent or image descriptor");
        return;
    }

    lv_obj_t *bg = lv_image_create(parent);
    lv_image_set_src(bg, img_dsc);
    lv_obj_set_size(bg, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_to_index(bg, 0); // Force to absolute bottom layer

    LOG_INF("Background widget initialized");
}