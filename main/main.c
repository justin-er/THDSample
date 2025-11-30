#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_log.h"

#include "dht_reader.h"
#include "led_controller.h"

static const char *TAG = "main";

void app_main(void)
{
    // Initialize LED controller for strip mode with RMT backend
    esp_err_t ret = led_controller_init_strip(GPIO_NUM_48, 1, LED_CONTROLLER_BACKEND_RMT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize LED controller: %s", esp_err_to_name(ret));
        return;
    }

    led_controller_color_t light_white = {2, 2, 2};
    led_controller_start_blink(2000, light_white);

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
