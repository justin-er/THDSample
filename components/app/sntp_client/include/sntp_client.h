#ifndef SNTP_CLIENT_H
#define SNTP_CLIENT_H

#include "esp_err.h"
#include <stddef.h>

/**
 * Start SNTP client
 * Begins time synchronization with configured NTP server
 * 
 * @return ESP_OK on success
 */
esp_err_t sntp_client_start(void);

/**
 * Stop SNTP client
 * 
 * @return ESP_OK on success
 */
esp_err_t sntp_client_stop(void);

/**
 * Get formatted local time string
 * 
 * @param buffer Buffer to store time string
 * @param size Size of buffer
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if time not synced
 */
esp_err_t sntp_client_get_time_string(char *buffer, size_t size);

/**
 * Check if time is synchronized
 * 
 * @return true if synced, false otherwise
 */
bool sntp_client_is_synced(void);

#endif // SNTP_CLIENT_H

