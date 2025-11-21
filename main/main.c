#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "DHTReader.h"

void app_main(void)
{
    // Initialize DHT task and get queue handle
    QueueHandle_t dht_queue = dht_init();
    
    DHTData sensor_data;
    
    while (1)
    {
        // Receive data from the DHT task
        if (xQueueReceive(dht_queue, &sensor_data, portMAX_DELAY))
            printf("Humidity: %.1f%% Temp: %.1fC\n", sensor_data.humidity, sensor_data.temperature);
    }
}