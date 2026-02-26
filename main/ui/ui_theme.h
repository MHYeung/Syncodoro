#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// =========================================================
// OVERTEC BRAND COLOR PALETTE
// =========================================================
// Minimalist, premium dark theme inspired by the brand logo.

// --- Background Colors ---
// The deep dark background seen in the brand image
#define OVERTEC_BG_MAIN         lv_color_hex(0x1C1C1C) 
// A slightly lighter shade for buttons, panels, or elevated surfaces
#define OVERTEC_BG_SURFACE      lv_color_hex(0x2D2D2D) 

// --- Typography Colors ---
// The warm off-white / bone color used for the OVERTEC lettering
#define OVERTEC_TEXT_PRIMARY    lv_color_hex(0xEBECE4) 
// A muted gray for secondary text (like "Free . Your . Imagination")
#define OVERTEC_TEXT_SECONDARY  lv_color_hex(0x8A8A8A) 

// --- Accent & State Colors ---
// A subtle, minimalist brand accent (matching the warm tone of the text)
#define OVERTEC_ACCENT          lv_color_hex(0xD6D8C9) 

// Semantic colors for the timer, desaturated slightly to fit the premium vibe
#define OVERTEC_STATE_START     lv_color_hex(0x5E9C60) // Muted Green
#define OVERTEC_STATE_PAUSE     lv_color_hex(0xD18B47) // Muted Orange/Sand
#define OVERTEC_STATE_STOP      lv_color_hex(0xC05A5A) // Muted Red

#ifdef __cplusplus
}
#endif