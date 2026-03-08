#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// =========================================================
// RUNTIME COLOUR THEME
// All UI files reference g_theme.* fields through these macros
// so that switching themes at runtime automatically applies
// to every newly created screen without touching UI source code.
// =========================================================

typedef struct {
    lv_color_t bg_main;         // Deep background
    lv_color_t bg_surface;      // Panels, buttons, elevated surfaces
    lv_color_t text_primary;    // Primary label text
    lv_color_t text_secondary;  // Muted / secondary text
    lv_color_t accent;          // Tag labels, decorative accent
    lv_color_t state_start;     // FOCUS / running / connected (green-family)
    lv_color_t state_pause;     // PAUSED / connecting (orange-family)
    lv_color_t state_stop;      // DONE / error / give-up (red-family)
} pomo_theme_t;

// Number of built-in presets (Dark, Forest, Ocean, Warm)
#define POMO_THEME_COUNT 4

// Global active theme — populated by pomo_theme_apply() before any UI is created
extern pomo_theme_t g_theme;

/**
 * @brief Copy preset[idx] into g_theme.  Safe to call before LVGL init.
 *        Callers must persist the index to NVS themselves if desired.
 *
 * @param idx  Theme index 0–(POMO_THEME_COUNT-1); clamped to 0 if out of range.
 */
void pomo_theme_apply(uint8_t idx);

// ── Convenience macros (identical names to the original palette) ──────────
// UI source files use these macros unchanged; they now resolve to g_theme fields.
#define OVERTEC_BG_MAIN         g_theme.bg_main
#define OVERTEC_BG_SURFACE      g_theme.bg_surface
#define OVERTEC_TEXT_PRIMARY    g_theme.text_primary
#define OVERTEC_TEXT_SECONDARY  g_theme.text_secondary
#define OVERTEC_ACCENT          g_theme.accent
#define OVERTEC_STATE_START     g_theme.state_start
#define OVERTEC_STATE_PAUSE     g_theme.state_pause
#define OVERTEC_STATE_STOP      g_theme.state_stop

#ifdef __cplusplus
}
#endif
