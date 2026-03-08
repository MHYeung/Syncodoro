#include "nvs_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

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
