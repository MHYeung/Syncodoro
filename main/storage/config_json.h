#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_JSON_PATH "/sdcard/config.json"

// Load focus duration and class tag from /sdcard/config.json.
// Returns ESP_OK on success. Values unchanged on error.
esp_err_t config_json_load(int *focus_duration, char *class_tag, size_t tag_len);

// Save focus duration and class tag to /sdcard/config.json.
esp_err_t config_json_save(int focus_duration, const char *class_tag);

#ifdef __cplusplus
}
#endif
