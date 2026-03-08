#include "ui_history.h"
#include "ui_theme.h"
#include "ui_timer.h"
#include "session_csv.h"

#include "lvgl.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

LV_FONT_DECLARE(lv_font_montserrat_16);

// =========================================================
// HISTORY SCREEN
// Reads sessions.csv from SD card and shows each row in a
// scrollable list with Tag, Duration, Date.
// =========================================================

static void nav_back_cb(lv_event_t *e)
{
    ui_timer_load();
}

void ui_history_load(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, OVERTEC_BG_MAIN, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Title ─────────────────────────────────────────────
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, LV_SYMBOL_LIST "  Session History");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 10, 10);

    // ── Back button ───────────────────────────────────────
    lv_obj_t *btn_back = lv_button_create(scr);
    lv_obj_set_size(btn_back, 70, 32);
    lv_obj_align(btn_back, LV_ALIGN_TOP_RIGHT, -8, 6);
    lv_obj_set_style_bg_color(btn_back, OVERTEC_BG_SURFACE, 0);
    lv_obj_add_event_cb(btn_back, nav_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(lbl_back, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_center(lbl_back);

    // ── Scrollable list container ─────────────────────────
    lv_obj_t *list_cont = lv_obj_create(scr);
    lv_obj_set_size(list_cont, 320, 200);
    lv_obj_align(list_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list_cont, OVERTEC_BG_MAIN, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_style_pad_all(list_cont, 4, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list_cont, LV_DIR_VER);

    // ── Read CSV ──────────────────────────────────────────
    {
        char line[256];
        char timerID[32], classTag[32], dateAndTime[32];
        int focusDuration, isCompleted, count = 0;
        double pomoCount;
        bool first = true;

        FILE *f = fopen(SESSION_CSV_PATH, "r");
        if (!f) {
            lv_obj_t *lbl_empty = lv_label_create(list_cont);
            lv_label_set_text(lbl_empty, "No sessions yet.\nComplete a session to see it here.");
            lv_obj_set_style_text_color(lbl_empty, OVERTEC_TEXT_SECONDARY, 0);
            lv_obj_set_style_text_align(lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(lbl_empty);
        } else {
            while (fgets(line, sizeof(line), f)) {
                // Skip header row
                if (first) { first = false; continue; }

                line[strcspn(line, "\r\n")] = '\0';
                if (line[0] == '\0') continue;

                timerID[0] = classTag[0] = dateAndTime[0] = '\0';
                focusDuration = isCompleted = 0;
                pomoCount = 0.0;

                int parsed = sscanf(line, "%31[^,],%d,%lf,%31[^,],%31[^,],%d",
                                    timerID, &focusDuration, &pomoCount,
                                    classTag, dateAndTime, &isCompleted);
                if (parsed < 5) continue;

                // ── Row card ──────────────────────────────
                lv_obj_t *card = lv_obj_create(list_cont);
                lv_obj_set_width(card, lv_pct(100));
                lv_obj_set_height(card, LV_SIZE_CONTENT);
                lv_obj_set_style_bg_color(card, OVERTEC_BG_SURFACE, 0);
                lv_obj_set_style_border_width(card, 0, 0);
                lv_obj_set_style_radius(card, 6, 0);
                lv_obj_set_style_pad_all(card, 8, 0);
                lv_obj_set_style_margin_bottom(card, 4, 0);
                lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
                lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN,
                                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t *lbl_tag = lv_label_create(card);
                lv_label_set_text(lbl_tag, classTag);
                lv_obj_set_style_text_color(lbl_tag, OVERTEC_ACCENT, 0);
                lv_obj_set_width(lbl_tag, 80);

                char dur_str[16];
                snprintf(dur_str, sizeof(dur_str), "%d min", focusDuration);
                lv_obj_t *lbl_dur = lv_label_create(card);
                lv_label_set_text(lbl_dur, dur_str);
                lv_obj_set_style_text_color(lbl_dur, OVERTEC_TEXT_PRIMARY, 0);
                lv_obj_set_width(lbl_dur, 55);

                lv_obj_t *lbl_dt = lv_label_create(card);
                lv_label_set_text(lbl_dt, dateAndTime);
                lv_obj_set_style_text_color(lbl_dt, OVERTEC_TEXT_SECONDARY, 0);
                lv_obj_set_width(lbl_dt, 100);
                lv_obj_set_style_text_align(lbl_dt, LV_TEXT_ALIGN_RIGHT, 0);

                count++;
                if (count >= 50) break; // cap to protect RAM
            }
            fclose(f);

            if (count == 0) {
                lv_obj_t *lbl_empty = lv_label_create(list_cont);
                lv_label_set_text(lbl_empty, "No sessions yet.\nComplete a session to see it here.");
                lv_obj_set_style_text_color(lbl_empty, OVERTEC_TEXT_SECONDARY, 0);
                lv_obj_set_style_text_align(lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
            }
        }
    }

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
}
