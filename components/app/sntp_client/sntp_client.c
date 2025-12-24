#include "sntp_client.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "config.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>

static const char *TAG = "sntp_client";

static bool sntp_initialized = false;
static bool time_synced = false;

/**
 * SNTP sync notification callback
 */
static void sntp_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized");
    time_synced = true;
}

esp_err_t sntp_client_start(void)
{
    if (sntp_initialized) {
        ESP_LOGW(TAG, "SNTP already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting SNTP client");
    
    // Set timezone
    setenv("TZ", SNTP_TIMEZONE, 1);
    tzset();
    
    // Initialize SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, SNTP_SERVER);
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    esp_sntp_set_time_sync_notification_cb(sntp_sync_notification_cb);
    
    esp_sntp_init();
    sntp_initialized = true;
    
    ESP_LOGI(TAG, "SNTP client started, timezone: %s, server: %s", SNTP_TIMEZONE, SNTP_SERVER);
    return ESP_OK;
}

esp_err_t sntp_client_stop(void)
{
    if (!sntp_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping SNTP client");
    esp_sntp_stop();
    sntp_initialized = false;
    time_synced = false;
    
    return ESP_OK;
}

esp_err_t sntp_client_get_time_string(char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!time_synced) {
        snprintf(buffer, size, "Time not synchronized");
        return ESP_ERR_INVALID_STATE;
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Format: "Tuesday, December 24, 2025 3:45 PM"
    strftime(buffer, size, "%A, %B %d, %Y %I:%M %p", &timeinfo);
    
    return ESP_OK;
}

bool sntp_client_is_synced(void)
{
    return time_synced;
}

