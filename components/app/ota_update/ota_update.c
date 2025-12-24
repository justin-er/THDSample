#include "ota_update.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ota_update";

// OTA state
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *update_partition = NULL;
static size_t bytes_written = 0;
static size_t total_size = 0;
static bool ota_in_progress = false;

/**
 * Validate firmware image header
 * Basic validation - checks magic byte and segments
 * 
 * NOTE: For production, implement signature verification:
 * - Generate RSA/ECDSA key pair
 * - Sign firmware with private key
 * - Embed public key in bootloader
 * - Verify signature before flashing
 * - See ESP-IDF secure boot documentation
 */
static esp_err_t validate_firmware_header(const uint8_t *data, size_t size)
{
    if (size < sizeof(esp_image_header_t)) {
        ESP_LOGE(TAG, "Data too small for image header");
        return ESP_ERR_INVALID_SIZE;
    }
    
    esp_image_header_t *header = (esp_image_header_t *)data;
    
    // Check magic byte
    if (header->magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "Invalid magic byte: 0x%02X (expected 0x%02X)", 
                 header->magic, ESP_IMAGE_HEADER_MAGIC);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Firmware header valid");
    return ESP_OK;
}

esp_err_t ota_update_begin(size_t firmware_size)
{
    if (ota_in_progress) {
        ESP_LOGE(TAG, "OTA already in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (firmware_size == 0) {
        ESP_LOGE(TAG, "Invalid firmware size");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting OTA update, size: %zu bytes", firmware_size);
    
    // Get next OTA partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Writing to partition: %s at offset 0x%lx", 
             update_partition->label, update_partition->address);
    
    // Check partition size
    if (firmware_size > update_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %lu", firmware_size, update_partition->size);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Begin OTA
    esp_err_t ret = esp_ota_begin(update_partition, firmware_size, &ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    total_size = firmware_size;
    bytes_written = 0;
    ota_in_progress = true;
    
    ESP_LOGI(TAG, "OTA update started successfully");
    return ESP_OK;
}

esp_err_t ota_update_write(const uint8_t *data, size_t size)
{
    if (!ota_in_progress) {
        ESP_LOGE(TAG, "OTA not in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate first chunk (contains header)
    if (bytes_written == 0) {
        esp_err_t ret = validate_firmware_header(data, size);
        if (ret != ESP_OK) {
            ota_update_abort();
            return ret;
        }
    }
    
    // Write chunk to OTA partition
    esp_err_t ret = esp_ota_write(ota_handle, data, size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(ret));
        ota_update_abort();
        return ret;
    }
    
    bytes_written += size;
    
    // Log progress every 10%
    uint8_t progress = ota_update_get_progress();
    static uint8_t last_progress = 0;
    if (progress >= last_progress + 10) {
        ESP_LOGI(TAG, "OTA progress: %d%%", progress);
        last_progress = progress;
    }
    
    return ESP_OK;
}

esp_err_t ota_update_end(void)
{
    if (!ota_in_progress) {
        ESP_LOGE(TAG, "OTA not in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Finalizing OTA update");
    
    // End OTA and validate
    esp_err_t ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(ret));
        ota_in_progress = false;
        return ret;
    }
    
    // Set boot partition
    ret = esp_ota_set_boot_partition(update_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(ret));
        ota_in_progress = false;
        return ret;
    }
    
    ota_in_progress = false;
    
    ESP_LOGI(TAG, "OTA update completed successfully");
    ESP_LOGI(TAG, "Total bytes written: %zu", bytes_written);
    ESP_LOGI(TAG, "Rebooting to apply new firmware in 2 seconds...");
    
    // Give delay to allow HTTP response to be fully sent before reboot
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Reboot to apply new firmware
    esp_restart();
    
    // Should never reach here, but return OK just in case
    return ESP_OK;
}

void ota_update_abort(void)
{
    if (!ota_in_progress) {
        return;
    }
    
    ESP_LOGW(TAG, "Aborting OTA update");
    
    esp_ota_abort(ota_handle);
    ota_in_progress = false;
    bytes_written = 0;
    total_size = 0;
}

uint8_t ota_update_get_progress(void)
{
    if (total_size == 0) {
        return 0;
    }
    
    return (bytes_written * 100) / total_size;
}

