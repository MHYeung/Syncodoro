#pragma once

#include "esp_err.h"
#include "pomo_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_CSV_PATH    "/sdcard/sessions.csv"
#define SESSION_CSV_HEADER  "timerID,focusDuration,pomoCount,classTag,dateAndTime,isCompleted\n"

// Append one completed session row to /sdcard/sessions.csv.
// Creates the file with a header if it doesn't exist yet.
esp_err_t session_csv_append(const pomoTimer_t *session);

// Read at most buf_size-1 bytes of the CSV file into buf (null-terminated).
// Returns the number of bytes read, or -1 on error.
int session_csv_read(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
