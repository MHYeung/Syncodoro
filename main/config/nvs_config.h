#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if WiFi setup has never been completed (first boot or reset).
bool nvs_config_get_setup_mode(void);
esp_err_t nvs_config_set_setup_mode(bool val);

esp_err_t nvs_config_get_wifi_ssid(char *buf, size_t len);
esp_err_t nvs_config_get_wifi_password(char *buf, size_t len);
esp_err_t nvs_config_save_wifi_credentials(const char *ssid, const char *password);

// Colour theme index (0 = Dark, 1 = Forest, 2 = Ocean, 3 = Warm).
// Returns 0 (Dark) when no value has been saved yet.
uint8_t nvs_config_get_theme(void);
esp_err_t nvs_config_set_theme(uint8_t idx);

// User-defined tags stored as a newline-separated string (max 512 bytes).
// nvs_config_get_tags returns ESP_ERR_NVS_NOT_FOUND when no custom tags are saved.
esp_err_t nvs_config_get_tags(char *buf, size_t len);
esp_err_t nvs_config_set_tags(const char *tags_nl);

#ifdef __cplusplus
}
#endif
