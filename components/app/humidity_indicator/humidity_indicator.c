#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_log.h"

#include "humidity_indicator.h"
#include "dht_reader.h"
#include "led_controller.h"

static const char *TAG = "humidity_indicator";

// Colors at 50% intensity
static const led_controller_color_t COLOR_GREEN = {0, 32, 0};
static const led_controller_color_t COLOR_ORANGE = {32, 16, 0};
static const led_controller_color_t COLOR_RED = {32, 0, 0};

static void humidity_indicator_task(void *pvParameters)
{
    QueueHandle_t dht_queue = (QueueHandle_t)pvParameters;
    dht_data_t dht_data;

    ESP_LOGI(TAG, "Humidity indicator task started");

    while (1)
    {
        // Wait for data from the DHT reader
        if (xQueueReceive(dht_queue, &dht_data, portMAX_DELAY))
        {
            printf("Humidity: %.1f%% Temp: %.1fC\n", dht_data.humidity, dht_data.temperature);

            // Update LED color based on humidity
            if (dht_data.humidity < 50.0f)
            {
                led_controller_set_color(COLOR_GREEN);
            }
            else if (dht_data.humidity < 55.0f)
            {
                led_controller_set_color(COLOR_ORANGE);
            }
            else
            {
                led_controller_set_color(COLOR_RED);
            }
        }
    }
}

esp_err_t humidity_indicator_start(void)
{
    ESP_LOGI(TAG, "Starting humidity indicator...");

    // Initialize DHT reader and get queue handle
    QueueHandle_t dht_queue = dht_init();
    if (dht_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize DHT reader");
        return ESP_FAIL;
    }

    // Create the task that processes sensor data and indicates humidity
    BaseType_t task_created = xTaskCreate(
        humidity_indicator_task,
        "humidity_indicator",
        4096,
        (void *)dht_queue,
        5,
        NULL
    );

    if (task_created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create humidity indicator task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Humidity indicator started successfully");
    return ESP_OK;
}

