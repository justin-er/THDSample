#include "app_coordinator.h"
#include "dht_reader.h"
#include "ota_update.h"
#include "app_nvs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "app_coordinator";

// Firmware version
#define FIRMWARE_VERSION "1.0.0"

// Cached sensor data
static app_coordinator_sensor_data_t cached_sensor_data = {0};
static SemaphoreHandle_t sensor_mutex = NULL;

// Cached system info
static app_coordinator_system_info_t cached_system_info = {0};
static SemaphoreHandle_t system_mutex = NULL;

// DHT queue handle
static QueueHandle_t dht_queue = NULL;

// System start time
static uint64_t system_start_time = 0;

/**
 * Sensor monitoring task
 * Subscribes to DHT reader queue and caches latest readings
 */
static void sensor_monitor_task(void *pvParameters)
{
    dht_data_t dht_data;
    
    ESP_LOGI(TAG, "Sensor monitor task started");
    
    while (1) {
        if (xQueueReceive(dht_queue, &dht_data, portMAX_DELAY)) {
            // Update cached sensor data (thread-safe)
            if (xSemaphoreTake(sensor_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                cached_sensor_data.temperature = dht_data.temperature;
                cached_sensor_data.humidity = dht_data.humidity;
                cached_sensor_data.timestamp = time(NULL);
                cached_sensor_data.valid = true;
                xSemaphoreGive(sensor_mutex);
                
                ESP_LOGD(TAG, "Sensor data updated: %.1f°C, %.1f%%", 
                         dht_data.temperature, dht_data.humidity);
            }
        }
    }
}

/**
 * System monitoring task
 * Tracks heap, uptime, and other system metrics
 */
static void system_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "System monitor task started");
    
    while (1) {
        // Update system info (thread-safe)
        if (xSemaphoreTake(system_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cached_system_info.heap_free = esp_get_free_heap_size();
            cached_system_info.heap_min = esp_get_minimum_free_heap_size();
            cached_system_info.uptime_seconds = (esp_timer_get_time() - system_start_time) / 1000000;
            cached_system_info.firmware_version = FIRMWARE_VERSION;
            cached_system_info.compile_date = __DATE__;
            cached_system_info.compile_time = __TIME__;
            // WiFi status will be updated by app_wifi
            xSemaphoreGive(system_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t app_coordinator_start(void)
{
    ESP_LOGI(TAG, "Starting application coordinator");
    
    // Record system start time
    system_start_time = esp_timer_get_time();
    
    // Initialize DHT reader and get queue handle
    dht_queue = dht_init();
    if (dht_queue == NULL) {
        ESP_LOGE(TAG, "Failed to initialize DHT reader");
        return ESP_FAIL;
    }
    
    // Create mutexes for thread-safe access
    sensor_mutex = xSemaphoreCreateMutex();
    if (sensor_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor mutex");
        return ESP_FAIL;
    }
    
    system_mutex = xSemaphoreCreateMutex();
    if (system_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create system mutex");
        return ESP_FAIL;
    }
    
    // Initialize cached system info
    cached_system_info.firmware_version = FIRMWARE_VERSION;
    cached_system_info.compile_date = __DATE__;
    cached_system_info.compile_time = __TIME__;
    
    // Create sensor monitoring task
    BaseType_t ret = xTaskCreate(
        sensor_monitor_task,
        "sensor_mon",
        4096,
        NULL,
        5,
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor monitor task");
        return ESP_FAIL;
    }
    
    // Create system monitoring task
    ret = xTaskCreate(
        system_monitor_task,
        "system_mon",
        2048,
        NULL,
        5,
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create system monitor task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Application coordinator started successfully");
    return ESP_OK;
}

esp_err_t app_coordinator_get_sensor_data(app_coordinator_sensor_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(sensor_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        *data = cached_sensor_data;
        xSemaphoreGive(sensor_mutex);
        return cached_sensor_data.valid ? ESP_OK : ESP_ERR_NOT_FOUND;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t app_coordinator_get_system_info(app_coordinator_system_info_t *info)
{
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(system_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        *info = cached_system_info;
        xSemaphoreGive(system_mutex);
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t app_coordinator_trigger_ota(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Triggering OTA update, size: %zu bytes", size);
    
    // This will be implemented when ota_update component is created
    // For now, return not supported
    ESP_LOGW(TAG, "OTA update not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t app_coordinator_get_ota_status(app_coordinator_ota_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Return current firmware info
    status->status = 0;  // Idle
    status->compile_date = __DATE__;
    status->compile_time = __TIME__;
    
    return ESP_OK;
}

esp_err_t app_coordinator_backup_config(char **json_out)
{
    if (json_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Backing up configuration");
    
    // This will be implemented when NVS export is added
    // For now, return not supported
    ESP_LOGW(TAG, "Config backup not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t app_coordinator_restore_config(const char *json_in)
{
    if (json_in == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Restoring configuration");
    
    // This will be implemented when NVS import is added
    // For now, return not supported
    ESP_LOGW(TAG, "Config restore not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

