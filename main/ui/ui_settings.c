#include "ui_settings.h"
#include "ui_theme.h"
#include "ui_timer.h"
#include "ui_history.h"
#include "wifi_manager.h"
#include "nvs_config.h"

#include "lvgl.h"
#include <stdio.h>
#include <stdint.h>

LV_FONT_DECLARE(lv_font_montserrat_16);

// =========================================================
// SETTINGS SCREEN
// =========================================================

// Periodic polling timer to refresh WiFi status label.
static lv_timer_t *s_poll_timer = NULL;
static lv_obj_t   *s_lbl_wifi_status = NULL;
static lv_obj_t   *s_lbl_ip = NULL;
static lv_obj_t   *s_btn_wifi = NULL;

static const char *state_to_str(wifi_state_t st)
{
    switch (st) {
        case WIFI_STATE_IDLE:         return "Idle";
        case WIFI_STATE_CONNECTING:   return "Connecting...";
        case WIFI_STATE_CONNECTED:    return "Connected";
        case WIFI_STATE_DISCONNECTED: return "Disconnected";
        case WIFI_STATE_ERROR:        return "Error";
        default:                      return "Unknown";
    }
}

static void poll_wifi_cb(lv_timer_t *t)
{
    if (!s_lbl_wifi_status) return;

    wifi_state_t st = wifi_manager_get_state();
    lv_label_set_text(s_lbl_wifi_status, state_to_str(st));

    if (st == WIFI_STATE_CONNECTED) {
        lv_obj_set_style_text_color(s_lbl_wifi_status, OVERTEC_STATE_START, 0);
        if (s_lbl_ip) {
            char ip[16];
            wifi_manager_get_ip_str(ip, sizeof(ip));
            char buf[32];
            snprintf(buf, sizeof(buf), "IP: %s", ip);
            lv_label_set_text(s_lbl_ip, buf);
        }
        // Hide connect button once connected
        if (s_btn_wifi) lv_obj_add_flag(s_btn_wifi, LV_OBJ_FLAG_HIDDEN);
    } else if (st == WIFI_STATE_ERROR) {
        lv_obj_set_style_text_color(s_lbl_wifi_status, OVERTEC_STATE_STOP, 0);
        if (s_btn_wifi) lv_obj_remove_flag(s_btn_wifi, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_set_style_text_color(s_lbl_wifi_status, OVERTEC_STATE_PAUSE, 0);
        if (s_btn_wifi) lv_obj_add_flag(s_btn_wifi, LV_OBJ_FLAG_HIDDEN);
    }
}

// Forward declaration — defined below after poll_wifi_cb
static void cleanup_and_nav(void);

// ──────────────── Theme swatch colours ────────────────────
// Each swatch shows the state_start accent of its preset so the user
// can identify themes at a glance.  Matches fill_* helpers in ui_theme.c.
static const uint32_t s_swatch_col[POMO_THEME_COUNT] = {
    0x5E9C60,   // 0 Dark   — muted green
    0x4CAF70,   // 1 Forest — bright forest green
    0x4A9CC7,   // 2 Ocean  — ocean blue
    0xD4865A,   // 3 Warm   — terracotta
};

static void theme_swatch_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    pomo_theme_apply(idx);
    nvs_config_set_theme(idx);
    // Navigate to timer; the freshly created screen will use the new g_theme.
    cleanup_and_nav();
    ui_timer_load();
}

// ──────────────── Navigation callbacks ────────────────────

static void cleanup_and_nav(void)
{
    // Stop poll timer before leaving this screen
    if (s_poll_timer) {
        lv_timer_delete(s_poll_timer);
        s_poll_timer = NULL;
    }
    s_lbl_wifi_status = NULL;
    s_lbl_ip = NULL;
    s_btn_wifi = NULL;
}

static void nav_timer_cb(lv_event_t *e)
{
    cleanup_and_nav();
    ui_timer_load();
}

static void nav_history_cb(lv_event_t *e)
{
    cleanup_and_nav();
    ui_history_load();
}

static void btn_wifi_connect_cb(lv_event_t *e)
{
    wifi_manager_start_sta();
}

// ──────────────── Screen creation ─────────────────────────

void ui_settings_load(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, OVERTEC_BG_MAIN, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Title row ─────────────────────────────────────────
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, LV_SYMBOL_SETTINGS "  Settings");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 12, 12);

    // ── WiFi section heading ───────────────────────────────
    lv_obj_t *lbl_wifi_head = lv_label_create(scr);
    lv_label_set_text(lbl_wifi_head, "WiFi");
    lv_obj_set_style_text_color(lbl_wifi_head, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_align(lbl_wifi_head, LV_ALIGN_TOP_LEFT, 12, 46);

    // WiFi status value
    s_lbl_wifi_status = lv_label_create(scr);
    lv_label_set_text(s_lbl_wifi_status, state_to_str(wifi_manager_get_state()));
    lv_obj_set_style_text_color(s_lbl_wifi_status, OVERTEC_STATE_PAUSE, 0);
    lv_obj_set_style_text_font(s_lbl_wifi_status, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_wifi_status, LV_ALIGN_TOP_RIGHT, -12, 44);

    // ── IP address ────────────────────────────────────────
    s_lbl_ip = lv_label_create(scr);
    lv_label_set_text(s_lbl_ip, "");
    lv_obj_set_style_text_color(s_lbl_ip, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_align(s_lbl_ip, LV_ALIGN_TOP_LEFT, 12, 70);

    // ── Connect button (shown only when not connected) ─────
    s_btn_wifi = lv_button_create(scr);
    lv_obj_set_size(s_btn_wifi, 140, 36);
    lv_obj_align(s_btn_wifi, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_set_style_bg_color(s_btn_wifi, OVERTEC_STATE_START, 0);
    lv_obj_add_event_cb(s_btn_wifi, btn_wifi_connect_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_btn = lv_label_create(s_btn_wifi);
    lv_label_set_text(lbl_btn, LV_SYMBOL_WIFI "  Connect WiFi");
    lv_obj_center(lbl_btn);

    // Hide button if already connected
    if (wifi_manager_get_state() == WIFI_STATE_CONNECTED) {
        lv_obj_add_flag(s_btn_wifi, LV_OBJ_FLAG_HIDDEN);
    }

    // ── Divider (moved up to make room for theme row) ─────
    lv_obj_t *div = lv_obj_create(scr);
    lv_obj_set_size(div, 296, 1);
    lv_obj_set_style_bg_color(div, OVERTEC_BG_SURFACE, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, 136);

    // ── Theme section ─────────────────────────────────────
    lv_obj_t *lbl_theme_head = lv_label_create(scr);
    lv_label_set_text(lbl_theme_head, "Theme");
    lv_obj_set_style_text_color(lbl_theme_head, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_align(lbl_theme_head, LV_ALIGN_TOP_LEFT, 12, 144);

    // 4 coloured swatches — one per theme preset, left to right.
    // A white border marks the currently active theme.
    uint8_t cur_theme = nvs_config_get_theme();
    for (int i = 0; i < POMO_THEME_COUNT; i++) {
        lv_obj_t *swatch = lv_button_create(scr);
        lv_obj_set_size(swatch, 46, 30);
        lv_obj_align(swatch, LV_ALIGN_TOP_LEFT, 70 + i * 54, 140);
        lv_obj_set_style_bg_color(swatch, lv_color_hex(s_swatch_col[i]), 0);
        lv_obj_set_style_radius(swatch, 8, 0);

        // Highlight the currently selected theme with a white border
        if ((uint8_t)i == cur_theme) {
            lv_obj_set_style_border_color(swatch, OVERTEC_TEXT_PRIMARY, 0);
            lv_obj_set_style_border_width(swatch, 2, 0);
        } else {
            lv_obj_set_style_border_width(swatch, 0, 0);
        }

        lv_obj_add_event_cb(swatch, theme_swatch_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)(uint8_t)i);
    }

    // ── Navigation buttons ────────────────────────────────
    lv_obj_t *btn_timer = lv_button_create(scr);
    lv_obj_set_size(btn_timer, 130, 46);
    lv_obj_align(btn_timer, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(btn_timer, OVERTEC_BG_SURFACE, 0);
    lv_obj_add_event_cb(btn_timer, nav_timer_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_timer = lv_label_create(btn_timer);
    lv_label_set_text(lbl_timer, LV_SYMBOL_HOME "  Timer");
    lv_obj_set_style_text_color(lbl_timer, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_center(lbl_timer);

    lv_obj_t *btn_hist = lv_button_create(scr);
    lv_obj_set_size(btn_hist, 130, 46);
    lv_obj_align(btn_hist, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(btn_hist, OVERTEC_BG_SURFACE, 0);
    lv_obj_add_event_cb(btn_hist, nav_history_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_hist = lv_label_create(btn_hist);
    lv_label_set_text(lbl_hist, LV_SYMBOL_LIST "  History");
    lv_obj_set_style_text_color(lbl_hist, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_center(lbl_hist);

    // ── Polling timer (refresh WiFi status every 2 s) ─────
    s_poll_timer = lv_timer_create(poll_wifi_cb, 2000, NULL);
    poll_wifi_cb(NULL); // Initial update

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
}
