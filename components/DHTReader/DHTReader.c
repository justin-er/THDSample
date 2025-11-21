#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <dht.h>

#include "DHTReader.h"

#define SENSOR_TYPE DHT_TYPE_AM2301
#define CONFIG_EXAMPLE_DATA_GPIO 4

static QueueHandle_t dht_queue = NULL;

DHTData readDHT(void)
{
    DHTData data = {0.0f, 0.0f};
    
#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(CONFIG_EXAMPLE_DATA_GPIO, GPIO_PULLUP_ONLY);
#endif
    
    if (dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &data.humidity, &data.temperature) == ESP_OK)
    {
        return data;
    }
    else
    {
        printf("Could not read data from sensor\n");
        return data;
    }
}

void dht_task(void *pvParameters)
{
    while (1)
    {
        DHTData sensor_data = readDHT();
        
        // Send the data to the queue
        if (dht_queue != NULL)
        {
            xQueueSend(dht_queue, &sensor_data, portMAX_DELAY);
        }
        
        // If you read the sensor data too often, it will heat up
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

QueueHandle_t dht_init(void)
{
    // Create a queue to hold DHTData
    dht_queue = xQueueCreate(1, sizeof(DHTData));
    
    // Create the DHT reading task
    xTaskCreate(dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    
    return dht_queue;
}


