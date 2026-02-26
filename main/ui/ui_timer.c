#include "ui_timer.h"
#include "ui_theme.h"
#include "pomo_model.h"
#include <stdio.h>

// Ensure the required fonts are pulled in (Enable these in menuconfig!)
LV_FONT_DECLARE(lv_font_montserrat_48);
LV_FONT_DECLARE(lv_font_montserrat_16);

// =========================================================
// UI STATE
// =========================================================
typedef struct {
    int current_time_secs;

    lv_obj_t *arc_progress;  // The new circular progress bar
    lv_obj_t *label_time;
    lv_obj_t *label_tag;     // New: Tag display (e.g. "Coding")
    lv_obj_t *label_status;
    lv_obj_t *arrow_left;
    lv_obj_t *arrow_right;
    lv_timer_t *lv_tick;
} pomo_ui_t;

static pomo_ui_t pui = {0};
static lv_obj_t * modal_overlay = NULL;

// Forward declarations
static void update_time_display(void);
static void update_ui_state(void);

// =========================================================
// TAG SELECTION MODAL LOGIC (Drop-Down Alternative)
// =========================================================
static void modal_tag_cancel_cb(lv_event_t * e)
{
    if (modal_overlay) {
        lv_obj_delete(modal_overlay);
        modal_overlay = NULL;
    }
}

static void modal_tag_select_cb(lv_event_t * e)
{
    // Retrieve the roller object we passed via user_data
    lv_obj_t * roller = lv_event_get_user_data(e);
    if (roller) {
        // Extract the selected tag and save it to our Data Model
        lv_roller_get_selected_str(roller, active_session.classTag, sizeof(active_session.classTag));
        // Update the UI label to reflect the change
        lv_label_set_text(pui.label_tag, active_session.classTag);
    }
    
    // Close the modal
    if (modal_overlay) {
        lv_obj_delete(modal_overlay);
        modal_overlay = NULL;
    }
}

static void show_tag_modal(void)
{
    // 1. Create dark overlay
    modal_overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(modal_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(modal_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(modal_overlay, 0, 0);
    lv_obj_set_style_radius(modal_overlay, 0, 0);
    lv_obj_add_flag(modal_overlay, LV_OBJ_FLAG_CLICKABLE);

    // 2. Create Modal Panel (Made Larger)
    lv_obj_t * panel = lv_obj_create(modal_overlay);
    lv_obj_set_size(panel, 260, 210); 
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, OVERTEC_BG_SURFACE, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 12, 0);

    // 3. Modal Title
    lv_obj_t * label_title = lv_label_create(panel);
    lv_label_set_text(label_title, "Select Tag");
    lv_obj_set_style_text_color(label_title, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 0);

    // 4. Roller (Touch-friendly dropdown list)
    lv_obj_t * roller = lv_roller_create(panel);
    lv_roller_set_options(roller, 
        "Coding\n"
        "Reading\n"
        "Writing\n"
        "Design\n"
        "Gaming\n"
        "Studying", 
        LV_ROLLER_MODE_NORMAL);
    
    // Style the roller to match the theme and use smaller font
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(roller, OVERTEC_BG_MAIN, 0);
    lv_obj_set_style_text_color(roller, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_color(roller, OVERTEC_TEXT_PRIMARY, LV_PART_SELECTED);
    lv_obj_set_style_bg_color(roller, OVERTEC_STATE_START, LV_PART_SELECTED);
    
    lv_roller_set_visible_row_count(roller, 4); // Show more rows now that font is smaller
    lv_obj_set_width(roller, 200);
    lv_obj_align(roller, LV_ALIGN_TOP_MID, 0, 25);

    // 5. Cancel Button
    lv_obj_t * btn_cancel = lv_button_create(panel);
    lv_obj_set_size(btn_cancel, 85, 40);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(btn_cancel, OVERTEC_STATE_STOP, 0);
    lv_obj_add_event_cb(btn_cancel, modal_tag_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "Cancel");
    lv_obj_center(label_cancel);

    // 6. Select/Save Button
    lv_obj_t * btn_select = lv_button_create(panel);
    lv_obj_set_size(btn_select, 85, 40);
    lv_obj_align(btn_select, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btn_select, OVERTEC_STATE_START, 0);
    
    // Pass the roller to the callback so we can extract the chosen text
    lv_obj_add_event_cb(btn_select, modal_tag_select_cb, LV_EVENT_CLICKED, roller);

    lv_obj_t * label_select = lv_label_create(btn_select);
    lv_label_set_text(label_select, "Save");
    lv_obj_center(label_select);
}

static void tag_clicked_cb(lv_event_t * e)
{
    // Only allow changing the tag if the timer is completely stopped
    if (active_session.timerState != pomoTimer_IDLE) return;
    show_tag_modal();
}

// =========================================================
// PAUSE MODAL POP-UP LOGIC
// =========================================================
static void modal_resume_cb(lv_event_t * e)
{
    if (modal_overlay) {
        lv_obj_delete(modal_overlay);
        modal_overlay = NULL;
    }
    active_session.timerState = pomoTimer_COUNTING;
    update_ui_state();
}

static void modal_giveup_cb(lv_event_t * e)
{
    if (modal_overlay) {
        lv_obj_delete(modal_overlay);
        modal_overlay = NULL;
    }
    active_session.timerState = pomoTimer_IDLE;
    pui.current_time_secs = active_session.focusDuration * 60;
    
    update_time_display();
    update_ui_state();
}

static void show_pause_modal(void)
{
    modal_overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(modal_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(modal_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(modal_overlay, 0, 0);
    lv_obj_set_style_radius(modal_overlay, 0, 0);
    lv_obj_add_flag(modal_overlay, LV_OBJ_FLAG_CLICKABLE);

    // Matched width to the tag modal for consistency
    lv_obj_t * panel = lv_obj_create(modal_overlay);
    lv_obj_set_size(panel, 260, 160); 
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, OVERTEC_BG_SURFACE, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 12, 0);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, "Session Paused\nFinish early?");
    lv_obj_set_style_text_color(label, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t * btn_resume = lv_button_create(panel);
    lv_obj_set_size(btn_resume, 100, 45); // Made buttons slightly larger
    lv_obj_align(btn_resume, LV_ALIGN_BOTTOM_LEFT, 0, -10);
    lv_obj_set_style_bg_color(btn_resume, OVERTEC_STATE_START, 0);
    lv_obj_add_event_cb(btn_resume, modal_resume_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_resume = lv_label_create(btn_resume);
    lv_label_set_text(label_resume, "Resume");
    lv_obj_center(label_resume);

    lv_obj_t * btn_giveup = lv_button_create(panel);
    lv_obj_set_size(btn_giveup, 100, 45); // Made buttons slightly larger
    lv_obj_align(btn_giveup, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
    lv_obj_set_style_bg_color(btn_giveup, OVERTEC_STATE_STOP, 0);
    lv_obj_add_event_cb(btn_giveup, modal_giveup_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_giveup = lv_label_create(btn_giveup);
    lv_label_set_text(label_giveup, "Give Up");
    lv_obj_center(label_giveup);
}

// =========================================================
// UI UPDATE LOGIC
// =========================================================
static void update_time_display(void)
{
    // Update numerical text
    int mm = pui.current_time_secs / 60;
    int ss = pui.current_time_secs % 60;
    lv_label_set_text_fmt(pui.label_time, "%02d:%02d", mm, ss);

    // Update Arc Progress
    int total_secs = active_session.focusDuration * 60;
    lv_arc_set_range(pui.arc_progress, 0, total_secs);
    lv_arc_set_value(pui.arc_progress, pui.current_time_secs);
}

static void update_ui_state(void)
{
    switch(active_session.timerState) {
        case pomoTimer_IDLE:
            lv_label_set_text(pui.label_status, "IDLE");
            lv_obj_set_style_text_color(pui.label_status, OVERTEC_TEXT_SECONDARY, 0);
            lv_obj_set_style_arc_color(pui.arc_progress, OVERTEC_TEXT_SECONDARY, LV_PART_INDICATOR);
            
            lv_obj_remove_flag(pui.arrow_left, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(pui.arrow_right, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case pomoTimer_COUNTING:
            lv_label_set_text(pui.label_status, "FOCUS");
            lv_obj_set_style_text_color(pui.label_status, OVERTEC_STATE_START, 0);
            lv_obj_set_style_arc_color(pui.arc_progress, OVERTEC_STATE_START, LV_PART_INDICATOR);

            lv_obj_add_flag(pui.arrow_left, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(pui.arrow_right, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case pomoTimer_STOP:
            lv_label_set_text(pui.label_status, "PAUSED");
            lv_obj_set_style_text_color(pui.label_status, OVERTEC_STATE_PAUSE, 0);
            lv_obj_set_style_arc_color(pui.arc_progress, OVERTEC_STATE_PAUSE, LV_PART_INDICATOR);

            lv_obj_add_flag(pui.arrow_left, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(pui.arrow_right, LV_OBJ_FLAG_HIDDEN);
            break;
            
        case pomoTimer_COMPLETED:
            lv_label_set_text(pui.label_status, "DONE");
            lv_obj_set_style_text_color(pui.label_status, OVERTEC_STATE_STOP, 0);
            lv_obj_set_style_arc_color(pui.arc_progress, OVERTEC_STATE_STOP, LV_PART_INDICATOR);

            lv_obj_add_flag(pui.arrow_left, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(pui.arrow_right, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

// =========================================================
// TOUCH EVENT CALLBACKS
// =========================================================
static void scr_clicked_cb(lv_event_t *e)
{
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t * current_target = lv_event_get_current_target(e);
    if (target != current_target) return;

    switch(active_session.timerState) {
        case pomoTimer_IDLE:
            active_session.timerState = pomoTimer_COUNTING;
            active_session.isCompleted = false;
            break;
        case pomoTimer_COUNTING:
            active_session.timerState = pomoTimer_STOP;
            show_pause_modal();
            break;
        case pomoTimer_STOP:
            // Handled by modal
            break;
        case pomoTimer_COMPLETED:
            active_session.timerState = pomoTimer_IDLE;
            pui.current_time_secs = active_session.focusDuration * 60;
            update_time_display();
            break;
    }
    update_ui_state();
}

static void arrow_left_cb(lv_event_t *e)
{
    if (active_session.timerState != pomoTimer_IDLE) return;
    
    if (active_session.focusDuration > 5) {
        active_session.focusDuration -= 5;
        pui.current_time_secs = active_session.focusDuration * 60;
        update_time_display();
    }
}

static void arrow_right_cb(lv_event_t *e)
{
    if (active_session.timerState != pomoTimer_IDLE) return;
    
    // Limits max duration to 120 minutes (2 hours)
    if (active_session.focusDuration < 120) {
        active_session.focusDuration += 5;
        pui.current_time_secs = active_session.focusDuration * 60;
        update_time_display();
    }
}

// =========================================================
// BACKGROUND TICKER
// =========================================================
static void lv_tick_cb(lv_timer_t *t)
{
    if (active_session.timerState == pomoTimer_COUNTING) {
        if (pui.current_time_secs > 0) {
            pui.current_time_secs--;
            update_time_display();
        } 
        
        if (pui.current_time_secs <= 0) {
            active_session.timerState = pomoTimer_COMPLETED;
            active_session.isCompleted = true;
            active_session.pomoCount += 1.0; 
            update_ui_state();
        }
    }
}

// =========================================================
// INITIALIZATION
// =========================================================
void ui_timer_create(lv_disp_t *disp)
{
    // Adjusted rotation here
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, OVERTEC_BG_MAIN, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(scr, scr_clicked_cb, LV_EVENT_CLICKED, NULL);

    // 1. Sleek Circular Progress Bar
    pui.arc_progress = lv_arc_create(scr);
    lv_obj_set_size(pui.arc_progress, 180, 180); // Adjusted size for landscape screen (320x240)
    lv_arc_set_rotation(pui.arc_progress, 270);  // Start drawing from Top center
    lv_arc_set_bg_angles(pui.arc_progress, 0, 360); // Full circle
    lv_obj_remove_style(pui.arc_progress, NULL, LV_PART_KNOB); // Hide ugly knob
    lv_obj_remove_flag(pui.arc_progress, LV_OBJ_FLAG_CLICKABLE); // Pass clicks to background
    
    // Style the arc tracks
    lv_obj_set_style_arc_width(pui.arc_progress, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(pui.arc_progress, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(pui.arc_progress, OVERTEC_BG_SURFACE, LV_PART_MAIN);
    lv_obj_align(pui.arc_progress, LV_ALIGN_CENTER, 0, -10); // Shifted up slightly to fit legend

    // 2. Large Centered Timer (Using new larger size 48 font)
    pui.label_time = lv_label_create(scr);
    lv_obj_set_style_text_font(pui.label_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(pui.label_time, OVERTEC_TEXT_PRIMARY, 0);
    lv_obj_set_width(pui.label_time, 180);
    lv_obj_set_style_text_align(pui.label_time, LV_TEXT_ALIGN_CENTER, 0);
    // Shifted down slightly from the very center so the TAG fits above it
    lv_obj_align_to(pui.label_time, pui.arc_progress, LV_ALIGN_CENTER, 0, 10);

    // 3. New Tag Label (e.g., "Coding")
    pui.label_tag = lv_label_create(scr);
    lv_label_set_text(pui.label_tag, active_session.classTag);
    lv_obj_set_width(pui.label_tag, 180);
    lv_obj_set_style_text_align(pui.label_tag, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(pui.label_tag, OVERTEC_ACCENT, 0);
    // Positioned dead center right above the main countdown timer
    lv_obj_align_to(pui.label_tag, pui.label_time, LV_ALIGN_OUT_TOP_MID, 0, -5);
    
    // Make Tag Label Clickable!
    lv_obj_add_flag(pui.label_tag, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pui.label_tag, tag_clicked_cb, LV_EVENT_CLICKED, NULL);

    // 4. Small Status Legend (Now underneath the arc!)
    pui.label_status = lv_label_create(scr);
    lv_obj_set_width(pui.label_status, 180);
    lv_obj_set_style_text_align(pui.label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(pui.label_status, pui.arc_progress, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    
    // 5. Left Arrow
    pui.arrow_left = lv_label_create(scr);
    lv_label_set_text(pui.arrow_left, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(pui.arrow_left, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_add_flag(pui.arrow_left, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pui.arrow_left, arrow_left_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(pui.arrow_left, 30, 0);
    lv_obj_align(pui.arrow_left, LV_ALIGN_LEFT_MID, 20, -10);

    // 6. Right Arrow
    pui.arrow_right = lv_label_create(scr);
    lv_label_set_text(pui.arrow_right, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(pui.arrow_right, OVERTEC_TEXT_SECONDARY, 0);
    lv_obj_add_flag(pui.arrow_right, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(pui.arrow_right, arrow_right_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(pui.arrow_right, 30, 0);
    lv_obj_align(pui.arrow_right, LV_ALIGN_RIGHT_MID, -20, -10);

    // 7. Initialize default startup values to 20 Minutes
    active_session.focusDuration = 20;
    pui.current_time_secs = 20 * 60;

    // Prepare initial state
    update_time_display();
    update_ui_state();

    // Start Background Ticker
    pui.lv_tick = lv_timer_create(lv_tick_cb, 1000, NULL);
}