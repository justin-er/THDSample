#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Begin OTA update
 * Prepares for firmware upload
 * 
 * @param firmware_size Expected size of firmware
 * @return ESP_OK on success
 */
esp_err_t ota_update_begin(size_t firmware_size);

/**
 * Write firmware data chunk
 * Streams firmware data to OTA partition
 * 
 * @param data Firmware data chunk
 * @param size Size of chunk
 * @return ESP_OK on success
 */
esp_err_t ota_update_write(const uint8_t *data, size_t size);

/**
 * End OTA update
 * Finalizes and validates firmware
 * 
 * @return ESP_OK on success
 */
esp_err_t ota_update_end(void);

/**
 * Abort OTA update
 * Cancels ongoing update
 */
void ota_update_abort(void);

/**
 * Get OTA update progress
 * 
 * @return Percentage complete (0-100)
 */
uint8_t ota_update_get_progress(void);

#endif // OTA_UPDATE_H

