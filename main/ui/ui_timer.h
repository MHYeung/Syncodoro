#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create and load the main Pomodoro timer screen.
// Restores countdown state from active_session.remaining_secs.
void ui_timer_load(void);

#ifdef __cplusplus
}
#endif
