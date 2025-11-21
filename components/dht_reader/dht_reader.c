#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <dht.h>

#include "dht_reader.h"
#include "sdkconfig.h"

#ifdef CONFIG_DHT_TYPE_AM2301
    #define SENSOR_TYPE DHT_TYPE_AM2301
#elif CONFIG_DHT_TYPE_DHT11
    #define SENSOR_TYPE DHT_TYPE_DHT11
#elif ITEAD_SI7021
    #define SENSOR_TYPE DHT_TYPE_SI7021
#else
    #define SENSOR_TYPE DHT_TYPE_AM2301
#endif

#define CONFIG_EXAMPLE_DATA_GPIO CONFIG_DHT_DATA_GPIO

static QueueHandle_t dht_queue = NULL;

static esp_err_t dht_read(dht_data_t *out)
{
    if (out == NULL) 
        return ESP_ERR_INVALID_ARG;
    
    dht_data_t data = {0.0f, 0.0f};
    
    esp_err_t result = dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &data.humidity, &data.temperature);
    *out = data;
    return result;
}

void dht_task(void *pvParameters)
{
    while (1)
    {
        dht_data_t sensor_data;
        
        if (dht_read(&sensor_data) == ESP_OK)
        {
            // Send the data to the queue
            if (dht_queue != NULL)
            {
                xQueueSend(dht_queue, &sensor_data, portMAX_DELAY);
            }
        }
        
        // If you read the sensor data too often, it will heat up
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

QueueHandle_t dht_init(void)
{
#ifdef CONFIG_DHT_INTERNAL_PULLUP
    gpio_set_pull_mode(CONFIG_EXAMPLE_DATA_GPIO, GPIO_PULLUP_ONLY);
#endif
    
    // Create a queue to hold dht_data_t
    dht_queue = xQueueCreate(1, sizeof(dht_data_t));
    
    // Create the DHT reading task
    xTaskCreate(dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    
    return dht_queue;
}


