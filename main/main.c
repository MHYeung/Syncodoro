#include "cyd.h"
#include "ui_timer.h"

#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_lvgl_port.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    cyd_handles_t cyd;
    ESP_ERROR_CHECK(cyd_init(&cyd));

    // Create UI under LVGL lock (LVGL is not thread-safe)
    lvgl_port_lock(0);
    ui_timer_create(cyd.disp);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Timer UI ready.");
}