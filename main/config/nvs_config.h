#pragma once

#include <stdbool.h>
#include <stddef.h>
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

#ifdef __cplusplus
}
#endif
