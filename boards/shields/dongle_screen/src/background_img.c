#include <lvgl.h>

const lv_img_dsc_t background_img = {
  .header.w = 240,
  .header.h = 135,
  .data_size = 240 * 135 * LV_COLOR_DEPTH / 8,
  .header.cf = LV_COLOR_FORMAT_RGB565,
  .data = NULL  // LVGL will fill with bg color
};

// Helper: create solid color background without image data
lv_obj_t *create_solid_bg(lv_obj_t *parent, lv_color_t color) {
  lv_obj_t *bg = lv_obj_create(parent);
  lv_obj_set_size(bg, 240, 135);
  lv_obj_set_style_bg_color(bg, color, 0);
  lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bg, 0, 0);
  lv_obj_move_to_index(bg, 0);
  return bg;
}