#ifndef HUMIDITY_INDICATOR_H
#define HUMIDITY_INDICATOR_H

#include <esp_err.h>

/**
 * @brief Initialize and start the humidity indicator
 * 
 * This starts a FreeRTOS task that:
 * - Initializes the DHT reader
 * - Reads temperature and humidity periodically
 * - Indicates humidity level via LED color:
 *   - Green: humidity < 50%
 *   - Red: humidity >= 50%
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t humidity_indicator_start(void);

#endif // HUMIDITY_INDICATOR_H

