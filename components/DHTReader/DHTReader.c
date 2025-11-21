#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dht.h>

#include "DHTReader.h"

#define SENSOR_TYPE DHT_TYPE_AM2301
#define CONFIG_EXAMPLE_DATA_GPIO 4

#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(CONFIG_EXAMPLE_DATA_GPIO, GPIO_PULLUP_ONLY);
#endif

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


