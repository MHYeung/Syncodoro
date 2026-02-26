#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_panel_io_handle_t lcd_io;
    esp_lcd_panel_handle_t    lcd_panel;
    esp_lcd_panel_io_handle_t touch_io;
    esp_lcd_touch_handle_t    touch;

    lv_disp_t  *disp;
    lv_indev_t *indev;
} cyd_handles_t;

esp_err_t cyd_init(cyd_handles_t *out);

#ifdef __cplusplus
}
#endif