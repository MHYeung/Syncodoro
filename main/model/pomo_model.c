#include "pomo_model.h"

// =========================================================
// GLOBAL SESSION STATE
// =========================================================
// This holds the actual data in RAM during a timer session.
// When the timer hits 0, this struct will be passed to your 
// SD card logic (and eventually Notion sync).

pomoTimer_t active_session = {
    .timerID = "session_001",
    .focusDuration = 20,
    .remaining_secs = 20 * 60,
    .pomoCount = 0.0,
    .classTag = "Coding",
    .dateAndTime = "--:--",
    .isCompleted = false,
    .timerState = pomoTimer_IDLE
};