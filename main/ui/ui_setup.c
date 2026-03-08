#include "ui_setup.h"
#include "ui_theme.h"
#include "wifi_manager.h"

#include "lvgl.h"

LV_FONT_DECLARE(lv_font_montserrat_16);

// =========================================================
// SETUP SCREEN
// Shown when SETUP_MODE is true (first boot or credential reset).
// The actual credential entry happens via the captive-portal web page
// served at http://192.168.4.1 when the user connects to "Syncodoro-Setup".
// This screen simply shows the instructions.
// =========================================================

void ui_setup_load(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, OVERTEC_BG_MAIN, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Title ────────────────────────────────────────────
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "WiFi Setup");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 12);

    // ── Step 1 label ─────────────────────────────────────
    lv_obj_t *lbl_step1 = lv_label_create(scr);
    lv_label_set_text(lbl_step1, "1. Connect phone/PC to WiFi:");
    lv_obj_set_style_text_font(lbl_step1, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_step1, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_set_width(lbl_step1, 300);
    lv_obj_align(lbl_step1, LV_ALIGN_TOP_MID, 0, 50);

    // ── AP SSID highlight ─────────────────────────────────
    lv_obj_t *lbl_ssid = lv_label_create(scr);
    lv_label_set_text(lbl_ssid, "Syncodoro-Setup");
    lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_ssid, OVERTEC_STATE_START, 0);
    lv_obj_align(lbl_ssid, LV_ALIGN_TOP_MID, 0, 78);

    // ── Step 2 label ─────────────────────────────────────
    lv_obj_t *lbl_step2 = lv_label_create(scr);
    lv_label_set_text(lbl_step2, "2. Open browser and go to:");
    lv_obj_set_style_text_font(lbl_step2, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_step2, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_set_width(lbl_step2, 300);
    lv_obj_align(lbl_step2, LV_ALIGN_TOP_MID, 0, 112);

    // ── URL highlight ─────────────────────────────────────
    lv_obj_t *lbl_url = lv_label_create(scr);
    lv_label_set_text(lbl_url, "http://192.168.4.1");
    lv_obj_set_style_text_font(lbl_url, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_url, OVERTEC_STATE_START, 0);
    lv_obj_align(lbl_url, LV_ALIGN_TOP_MID, 0, 140);

    // ── Step 3 label ─────────────────────────────────────
    lv_obj_t *lbl_step3 = lv_label_create(scr);
    lv_label_set_text(lbl_step3, "3. Enter your WiFi credentials\n   and tap Save & Connect.");
    lv_obj_set_style_text_font(lbl_step3, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_step3, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_set_width(lbl_step3, 300);
    lv_obj_align(lbl_step3, LV_ALIGN_TOP_MID, 0, 175);

    // ── Waiting indicator ─────────────────────────────────
    lv_obj_t *lbl_wait = lv_label_create(scr);
    lv_label_set_text(lbl_wait, "Waiting for setup...");
    lv_obj_set_style_text_color(lbl_wait, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_align(lbl_wait, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
}
