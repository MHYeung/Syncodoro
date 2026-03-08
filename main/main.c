#include "cyd.h"
#include "ui_timer.h"
#include "ui_setup.h"
#include "ui_theme.h"
#include "nvs_config.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "config_json.h"
#include "pomo_model.h"

#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_lvgl_port.h"
#include "esp_netif_sntp.h"

#include <string.h>

static const char *TAG = "main";

// Called from WiFi event (IP_EVENT_STA_GOT_IP) – starts the dashboard server
// and kicks off NTP time sync so sessions get accurate timestamps.
static void on_wifi_connected(void)
{
    ESP_LOGI(TAG, "WiFi connected → starting dashboard HTTP server");
    http_server_start_dashboard();

    // Initialise SNTP once — subsequent calls are ignored if already running.
    // System clock (time/gmtime) becomes accurate within a few seconds after
    // the NTP response arrives; the timer screen checks for a valid year before
    // stamping a session, so there is no race condition to handle.
    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&sntp_cfg);
    ESP_LOGI(TAG, "SNTP init → syncing time from pool.ntp.org");
}

void app_main(void)
{
    // ── 1. NVS init (always first) ────────────────────────
    ESP_ERROR_CHECK(nvs_flash_init());

    // ── 2. Read SETUP_MODE before anything else ───────────
    bool setup_mode = nvs_config_get_setup_mode();
    ESP_LOGI(TAG, "Boot: setup_mode=%d", setup_mode);

    // ── 2b. Apply saved colour theme before UI is created ─
    pomo_theme_apply(nvs_config_get_theme());

    // ── 3. Init BSP (display + touch + LVGL) ─────────────
    cyd_handles_t cyd;
    ESP_ERROR_CHECK(cyd_init(&cyd));

    // ── 4. Branch: setup mode vs normal operation ─────────
    if (setup_mode) {
        // ── Setup mode ──────────────────────────────────────
        // WiFi AP (open, SSID "Syncodoro-Setup") + captive portal HTTP server.
        // The user opens 192.168.4.1 in their browser, fills in credentials.
        // On save the device reboots into normal mode.
        wifi_manager_init();
        wifi_manager_start_ap();
        http_server_start_setup_mode();

        lvgl_port_lock(0);
        lv_display_set_rotation(cyd.disp, LV_DISPLAY_ROTATION_270);
        ui_setup_load();
        lvgl_port_unlock();

        ESP_LOGI(TAG, "Setup mode ready. Connect to 'Syncodoro-Setup' and open http://192.168.4.1");
    } else {
        // ── Normal operation ────────────────────────────────

        // Mount internal SPIFFS for session history and user config
        ESP_ERROR_CHECK(cyd_init_spiffs());

        // Load user config from /data/config.json (duration; tag is set from tag list below)
        {
            int focus_dur = active_session.focusDuration;
            char class_tag[32];
            strlcpy(class_tag, active_session.classTag, sizeof(class_tag));

            if (config_json_load(&focus_dur, class_tag, sizeof(class_tag)) == ESP_OK) {
                active_session.focusDuration = focus_dur;
                active_session.remaining_secs = focus_dur * 60;
                ESP_LOGI(TAG, "Config applied: %d min", focus_dur);
            }
        }

        // Align displayed tag with the first item on the web dashboard (NVS or default list)
        {
            char first_tag[32];
            nvs_config_get_first_tag(first_tag, sizeof(first_tag));
            strlcpy(active_session.classTag, first_tag, sizeof(active_session.classTag));
            ESP_LOGI(TAG, "Tag set to first in list: %s", active_session.classTag);
        }

        // Start WiFi STA (non-blocking; callback fires on IP assignment)
        wifi_manager_set_connected_cb(on_wifi_connected);
        wifi_manager_init();
        wifi_manager_start_sta();

        // Load the main timer UI
        lvgl_port_lock(0);
        lv_display_set_rotation(cyd.disp, LV_DISPLAY_ROTATION_270);
        ui_timer_load();
        lvgl_port_unlock();

        ESP_LOGI(TAG, "Timer UI ready.");
    }
}
