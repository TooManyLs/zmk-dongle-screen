#include <lvgl.h>

struct background_widget {
    lv_obj_t *obj;
};

struct background_widget *background_widget_init(lv_obj_t *parent, const lv_img_dsc_t *img_dsc);
void background_widget_free(struct background_widget *widget);