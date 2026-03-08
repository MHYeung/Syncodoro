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

#include <string.h>

static const char *TAG = "main";

// Called from WiFi event (IP_EVENT_STA_GOT_IP) – starts the dashboard server.
static void on_wifi_connected(void)
{
    ESP_LOGI(TAG, "WiFi connected → starting dashboard HTTP server");
    http_server_start_dashboard();
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

        // Load user config from /data/config.json and apply to active session
        {
            int focus_dur = active_session.focusDuration;
            char class_tag[32];
            strlcpy(class_tag, active_session.classTag, sizeof(class_tag));

            if (config_json_load(&focus_dur, class_tag, sizeof(class_tag)) == ESP_OK) {
                active_session.focusDuration = focus_dur;
                strlcpy(active_session.classTag, class_tag, sizeof(active_session.classTag));
                active_session.remaining_secs = focus_dur * 60;
                ESP_LOGI(TAG, "Config applied: %d min / %s", focus_dur, class_tag);
            }
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
