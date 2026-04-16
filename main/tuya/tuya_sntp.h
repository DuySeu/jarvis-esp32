#ifndef TUYA_SNTP_H
#define TUYA_SNTP_H

#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <sys/time.h>

static const char* TUYA_SNTP_TAG = "TuyaSNTP";

// TuyaSNTP — Handles NTP time synchronization required for Tuya API request signing.
//
// Tuya signatures require a 13-digit UTC timestamp (milliseconds since epoch).
// Call TuyaSntp::Sync() once after WiFi connects, before any Tuya API call.
class TuyaSntp {
public:
    // Call once after WiFi is connected. Blocks until sync completes (max 10s).
    // Returns true if sync succeeded, false on timeout.
    static bool Sync() {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            return true; // Already synced
        }
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();

        int retries = 0;
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && retries < 20) {
            vTaskDelay(pdMS_TO_TICKS(500));
            retries++;
        }
        bool synced = (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED);
        if (!synced) {
            ESP_LOGE(TUYA_SNTP_TAG, "SNTP sync timed out after 10s");
        } else {
            ESP_LOGI(TUYA_SNTP_TAG, "SNTP sync completed");
        }
        return synced;
    }

    // Returns current UTC time in milliseconds (13 digits).
    // Required for Tuya request signing — call AFTER Sync() succeeds.
    static int64_t GetTimestampMs() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (int64_t)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    }
};

#endif // TUYA_SNTP_H
