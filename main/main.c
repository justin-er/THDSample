#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "DHTReader.h"

void app_main(void)
{
    while (1)
    {
        DHTData sensor_data = readDHT();
        
        // Process the values here
        printf("Humidity: %.1f%% Temp: %.1fC\n", sensor_data.humidity, sensor_data.temperature);
        
        // Add your processing logic here
        // ...
        
        // If you read the sensor data too often, it will heat up
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}