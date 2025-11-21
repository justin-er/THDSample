#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "dht_reader.h"

void app_main(void)
{
    // Initialize DHT task and get queue handle
    QueueHandle_t dht_queue = dht_init();
    
    dht_data_t dht_data;
    
    while (1)
    {
        // Receive data from the DHT task
        if (xQueueReceive(dht_queue, &dht_data, portMAX_DELAY))
            printf("Humidity: %.1f%% Temp: %.1fC\n", dht_data.humidity, dht_data.temperature);
    }
}
