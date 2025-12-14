#ifndef SAMPLE_INTERACTOR_H
#define SAMPLE_INTERACTOR_H

#include <esp_err.h>

/**
 * @brief Initialize and start the sample interactor
 * 
 * This starts a FreeRTOS task that:
 * - Initializes the LED controller
 * - Initializes the DHT reader
 * - Reads temperature and humidity periodically
 * - Prints the sensor data
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t sample_interactor_start(void);

#endif // SAMPLE_INTERACTOR_H

