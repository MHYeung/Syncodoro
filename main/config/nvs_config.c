#include "nvs_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "nvs_cfg";
static const char *NVS_NS = "syncodoro";

bool nvs_config_get_setup_mode(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Namespace does not exist yet → treat as first boot → needs setup.
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return true;
    }

    uint8_t val = 1;
    err = nvs_get_u8(h, "setup_mode", &val);
    nvs_close(h);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    return (val != 0);
}

esp_err_t nvs_config_set_setup_mode(bool val)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_u8(h, "setup_mode", val ? 1 : 0);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_config_get_wifi_ssid(char *buf, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_str(h, "wifi_ssid", buf, &len);
    nvs_close(h);
    return err;
}

esp_err_t nvs_config_get_wifi_password(char *buf, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_str(h, "wifi_pass", buf, &len);
    nvs_close(h);
    return err;
}

esp_err_t nvs_config_save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, "wifi_ssid", ssid);
    if (err == ESP_OK) err = nvs_set_str(h, "wifi_pass", password ? password : "");
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

/**
 * @brief Read the saved colour theme index from NVS.
 *
 * @return Saved index (0–3), or 0 (Dark) if no value has been written yet.
 */
uint8_t nvs_config_get_theme(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return 0;

    uint8_t val = 0;
    nvs_get_u8(h, "theme_idx", &val);  // silently returns 0 if key absent
    nvs_close(h);
    return val;
}

/**
 * @brief Persist the selected colour theme index to NVS.
 *
 * @param idx  Theme index (0–3).
 * @return ESP_OK on success, or an NVS error code.
 */
esp_err_t nvs_config_set_theme(uint8_t idx)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_u8(h, "theme_idx", idx);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

/**
 * @brief Read user-defined tags from NVS as a newline-separated string.
 *
 * @param buf  Output buffer to receive the '\n'-separated tag list.
 * @param len  Size of buf in bytes (including null terminator).
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no custom tags saved yet,
 *         or another NVS error code.
 */
esp_err_t nvs_config_get_tags(char *buf, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_str(h, "user_tags", buf, &len);
    nvs_close(h);
    return err;
}

/**
 * @brief Persist user-defined tags to NVS as a newline-separated string.
 *
 * @param tags_nl  '\n'-separated tag list (max 511 usable characters).
 * @return ESP_OK on success, or an NVS error code.
 */
esp_err_t nvs_config_set_tags(const char *tags_nl)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, "user_tags", tags_nl);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

#define TAG_LIST_DEFAULT  "Coding\nReading\nWriting\nDesign\nGaming\nStudying"
#define TAG_FALLBACK      "Focus"

/**
 * @brief Copy the first tag from the stored list into buf for boot/default display.
 *
 * Reads the same tag list as the web dashboard (NVS user_tags or built-in list).
 * If the list is empty or the first line is empty, copies TAG_FALLBACK so the
 * device never shows an empty tag and avoids NULL/empty handling in the UI.
 *
 * @param buf  Output buffer for the first tag string.
 * @param len  Size of buf (use at least 32 for classTag).
 */
void nvs_config_get_first_tag(char *buf, size_t len)
{
    if (!buf || len == 0) return;

    char tag_buf[512];
    const char *list = TAG_LIST_DEFAULT;
    if (nvs_config_get_tags(tag_buf, sizeof(tag_buf)) == ESP_OK && tag_buf[0] != '\0') {
        list = tag_buf;
    }

    const char *end = strchr(list, '\n');
    size_t first_len = end ? (size_t)(end - list) : strlen(list);
    if (first_len == 0) {
        strlcpy(buf, TAG_FALLBACK, len);
        return;
    }
    if (first_len >= len) first_len = len - 1;
    memcpy(buf, list, first_len);
    buf[first_len] = '\0';
    if (buf[0] == '\0') strlcpy(buf, TAG_FALLBACK, len);
}
