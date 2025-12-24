#ifndef APP_COORDINATOR_H
#define APP_COORDINATOR_H

#include "esp_err.h"
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Sensor data structure
 */
typedef struct {
    float temperature;
    float humidity;
    time_t timestamp;
    bool valid;
} app_coordinator_sensor_data_t;

/**
 * System information structure
 */
typedef struct {
    size_t heap_free;
    size_t heap_min;
    uint32_t uptime_seconds;
    const char *firmware_version;
    const char *compile_date;
    const char *compile_time;
    bool wifi_sta_connected;
    uint8_t wifi_ap_clients;
} app_coordinator_system_info_t;

/**
 * OTA status structure
 */
typedef struct {
    int8_t status;           // -1: error, 0: idle, 1: complete
    const char *compile_date;
    const char *compile_time;
} app_coordinator_ota_status_t;

/**
 * Start application coordinator
 * Initializes DHT monitoring, system tracking, etc.
 * 
 * @return ESP_OK on success
 */
esp_err_t app_coordinator_start(void);

/**
 * Get latest sensor reading (thread-safe)
 * 
 * @param data Pointer to sensor data structure
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if data is NULL,
 *         ESP_ERR_NOT_FOUND if no valid data available
 */
esp_err_t app_coordinator_get_sensor_data(app_coordinator_sensor_data_t *data);

/**
 * Get system status information
 * 
 * @param info Pointer to system info structure
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if info is NULL
 */
esp_err_t app_coordinator_get_system_info(app_coordinator_system_info_t *info);

/**
 * Trigger OTA firmware update
 * 
 * @param data Firmware data buffer
 * @param size Size of firmware data
 * @return ESP_OK on success
 */
esp_err_t app_coordinator_trigger_ota(const uint8_t *data, size_t size);

/**
 * Get OTA update status
 * 
 * @param status Pointer to OTA status structure
 * @return ESP_OK on success
 */
esp_err_t app_coordinator_get_ota_status(app_coordinator_ota_status_t *status);

/**
 * Backup configuration to JSON string
 * 
 * @param json_out Pointer to receive allocated JSON string (caller must free)
 * @return ESP_OK on success
 */
esp_err_t app_coordinator_backup_config(char **json_out);

/**
 * Restore configuration from JSON string
 * 
 * @param json_in JSON string containing configuration
 * @return ESP_OK on success
 */
esp_err_t app_coordinator_restore_config(const char *json_in);

#endif // APP_COORDINATOR_H

