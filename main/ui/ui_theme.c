#include "ui_theme.h"
#include "esp_log.h"

static const char *TAG = "ui_theme";

// Global active theme — zero-initialised; populated at boot by pomo_theme_apply().
pomo_theme_t g_theme;

// =========================================================
// PRESET FILL HELPERS
// lv_color_hex() is a runtime call in LVGL v9 (not constexpr),
// so themes are filled via functions rather than static initialisers.
// =========================================================

/**
 * @brief Preset 0: Dark — original Overtec palette (deep charcoal + muted greens).
 */
static void fill_dark(pomo_theme_t *t)
{
    t->bg_main         = lv_color_hex(0x1C1C1C);
    t->bg_surface      = lv_color_hex(0x2D2D2D);
    t->text_primary    = lv_color_hex(0xEBECE4);
    t->text_secondary  = lv_color_hex(0x8A8A8A);
    t->accent          = lv_color_hex(0xD6D8C9);
    t->state_start     = lv_color_hex(0x5E9C60); // muted green
    t->state_pause     = lv_color_hex(0xD18B47); // muted orange
    t->state_stop      = lv_color_hex(0xC05A5A); // muted red
}

/**
 * @brief Preset 1: Forest — deep forest greens with warm golden accents.
 */
static void fill_forest(pomo_theme_t *t)
{
    t->bg_main         = lv_color_hex(0x1A2B1D); // very dark forest green
    t->bg_surface      = lv_color_hex(0x263829); // slightly lighter forest
    t->text_primary    = lv_color_hex(0xD8EDD8); // pale green-white
    t->text_secondary  = lv_color_hex(0x7A9A7A); // muted sage
    t->accent          = lv_color_hex(0xA8D4A8); // soft leaf green
    t->state_start     = lv_color_hex(0x4CAF70); // bright forest green
    t->state_pause     = lv_color_hex(0xC8A042); // golden amber
    t->state_stop      = lv_color_hex(0xB55C5C); // muted red
}

/**
 * @brief Preset 2: Ocean — deep navy blues with cyan highlights.
 */
static void fill_ocean(pomo_theme_t *t)
{
    t->bg_main         = lv_color_hex(0x0E1E2E); // deep navy
    t->bg_surface      = lv_color_hex(0x1A2E44); // dark ocean blue
    t->text_primary    = lv_color_hex(0xD4E8F0); // ice blue-white
    t->text_secondary  = lv_color_hex(0x6A8EA8); // muted steel blue
    t->accent          = lv_color_hex(0x8EC8E8); // soft cyan
    t->state_start     = lv_color_hex(0x4A9CC7); // ocean blue
    t->state_pause     = lv_color_hex(0xC89640); // warm amber (contrast on blue)
    t->state_stop      = lv_color_hex(0xC06868); // soft coral red
}

/**
 * @brief Preset 3: Warm — dark amber earth tones with olive and terracotta.
 */
static void fill_warm(pomo_theme_t *t)
{
    t->bg_main         = lv_color_hex(0x261A0E); // very dark warm brown
    t->bg_surface      = lv_color_hex(0x3A2818); // medium dark amber
    t->text_primary    = lv_color_hex(0xF0E0CC); // warm cream
    t->text_secondary  = lv_color_hex(0xA08060); // muted tan
    t->accent          = lv_color_hex(0xD4B090); // warm sand
    t->state_start     = lv_color_hex(0x8AAA50); // olive green
    t->state_pause     = lv_color_hex(0xD4865A); // terracotta orange
    t->state_stop      = lv_color_hex(0xC05A5A); // muted red
}

// =========================================================
// PUBLIC API
// =========================================================

void pomo_theme_apply(uint8_t idx)
{
    if (idx >= POMO_THEME_COUNT) {
        idx = 0;
    }

    switch (idx) {
        case 0: fill_dark(&g_theme);   break;
        case 1: fill_forest(&g_theme); break;
        case 2: fill_ocean(&g_theme);  break;
        case 3: fill_warm(&g_theme);   break;
        default: fill_dark(&g_theme);  break;
    }

    ESP_LOGI(TAG, "Theme applied: %d", idx);
}
