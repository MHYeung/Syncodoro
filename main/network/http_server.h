#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start the captive-portal HTTP server (AP mode, for WiFi credential entry).
esp_err_t http_server_start_setup_mode(void);

// Start the dashboard HTTP server (STA mode, served to LAN clients).
esp_err_t http_server_start_dashboard(void);

// Stop whichever HTTP server is currently running.
void http_server_stop(void);

#ifdef __cplusplus
}
#endif
