#include "session_csv.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "session_csv";

esp_err_t session_csv_append(const pomoTimer_t *session)
{
    if (!session) return ESP_ERR_INVALID_ARG;

    // Open for reading first to check if file exists (to write header).
    bool need_header = false;
    FILE *chk = fopen(SESSION_CSV_PATH, "r");
    if (!chk) {
        need_header = true;
    } else {
        fclose(chk);
    }

    FILE *f = fopen(SESSION_CSV_PATH, "a");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open %s for append", SESSION_CSV_PATH);
        return ESP_FAIL;
    }

    if (need_header) {
        fputs(SESSION_CSV_HEADER, f);
    }

    // timerID,focusDuration,pomoCount,classTag,dateAndTime,isCompleted
    fprintf(f, "%s,%d,%.1f,%s,%s,%d\n",
            session->timerID,
            session->focusDuration,
            session->pomoCount,
            session->classTag,
            session->dateAndTime,
            session->isCompleted ? 1 : 0);

    fflush(f);  // ensure data is written through to the filesystem before close
    fclose(f);
    ESP_LOGI(TAG, "Session saved: %s  tag=%s  dur=%dmin",
             session->timerID, session->classTag, session->focusDuration);
    return ESP_OK;
}

int session_csv_read(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return -1;

    FILE *f = fopen(SESSION_CSV_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "Cannot open %s for reading", SESSION_CSV_PATH);
        return -1;
    }

    size_t n = fread(buf, 1, buf_size - 1, f);
    buf[n] = '\0';
    fclose(f);
    return (int)n;
}
