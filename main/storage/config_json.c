#include "config_json.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "config_json";

// Very simple JSON key extractor – no external library needed for two fields.
static bool json_get_int(const char *json, const char *key, int *out)
{
    // Search for "key": <number>
    const char *p = strstr(json, key);
    if (!p) return false;
    p += strlen(key);
    while (*p == '"' || *p == ':' || *p == ' ') p++;
    if (*p < '0' || *p > '9') return false;
    *out = (int)strtol(p, NULL, 10);
    return true;
}

static bool json_get_str(const char *json, const char *key, char *out, size_t out_len)
{
    const char *p = strstr(json, key);
    if (!p) return false;
    p += strlen(key);
    while (*p == '"' || *p == ':' || *p == ' ') p++;
    if (*p != '"') return false;
    p++; // skip opening quote
    size_t i = 0;
    while (*p && *p != '"' && i < out_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return (i > 0);
}

esp_err_t config_json_load(int *focus_duration, char *class_tag, size_t tag_len)
{
    FILE *f = fopen(CONFIG_JSON_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "config.json not found at %s", CONFIG_JSON_PATH);
        return ESP_ERR_NOT_FOUND;
    }

    char buf[256] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    bool ok = false;
    if (focus_duration) {
        ok |= json_get_int(buf, "\"focusDuration\"", focus_duration);
    }
    if (class_tag && tag_len > 0) {
        ok |= json_get_str(buf, "\"classTag\"", class_tag, tag_len);
    }

    if (!ok) {
        ESP_LOGW(TAG, "config.json parse failed or keys missing");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Config loaded: focusDuration=%d  classTag=%s",
             focus_duration ? *focus_duration : -1,
             class_tag ? class_tag : "?");
    return ESP_OK;
}

esp_err_t config_json_save(int focus_duration, const char *class_tag)
{
    FILE *f = fopen(CONFIG_JSON_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot write config.json");
        return ESP_FAIL;
    }
    fprintf(f, "{\"focusDuration\":%d,\"classTag\":\"%s\"}\n",
            focus_duration, class_tag ? class_tag : "Coding");
    fclose(f);
    ESP_LOGI(TAG, "config.json saved");
    return ESP_OK;
}
