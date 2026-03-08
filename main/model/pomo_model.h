#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =========================================================
// STATE MACHINE (Aligned with Flowchart)
// =========================================================
typedef enum {
    pomoTimer_IDLE,
    pomoTimer_COUNTING,
    pomoTimer_STOP,       // Flowchart names the paused state as "STOP"
    pomoTimer_COMPLETED
} pomotimerState_t;

// =========================================================
// DATA MODEL (Aligned with Flowchart)
// =========================================================
typedef struct {
    char timerID[32];          // Unique ID for session tracking
    int focusDuration;         // Target duration in minutes
    int remaining_secs;        // Live countdown (persists across screen navigation)
    double pomoCount;          // Total accumulated pomodoros
    char classTag[32];         // "Coding", "Reading", etc.
    char dateAndTime[32];      // Timestamp string
    bool isCompleted;          // Flag for SD Card save trigger
    pomotimerState_t timerState;
} pomoTimer_t;

// Global active session data exposed to the rest of the application
extern pomoTimer_t active_session;

#ifdef __cplusplus
}
#endif