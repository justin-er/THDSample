#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_log.h"

#include "sample_interactor.h"
#include "dht_reader.h"
#include "led_controller.h"

static const char *TAG = "sample_interactor";

static void sample_interactor_task(void *pvParameters)
{
    QueueHandle_t dht_queue = (QueueHandle_t)pvParameters;
    dht_data_t dht_data;

    ESP_LOGI(TAG, "Interactor task started");

    while (1)
    {
        // Wait for data from the DHT reader
        if (xQueueReceive(dht_queue, &dht_data, portMAX_DELAY))
        {
            printf("Humidity: %.1f%% Temp: %.1fC\n", dht_data.humidity, dht_data.temperature);
        }
    }
}

esp_err_t sample_interactor_start(void)
{
    ESP_LOGI(TAG, "Starting sample interactor...");

    // Initialize LED controller for strip mode with RMT backend
    esp_err_t ret = led_controller_init_strip(GPIO_NUM_48, 1, LED_CONTROLLER_BACKEND_RMT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize LED controller: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start LED blinking
    led_controller_color_t light_white = {2, 2, 2};
    led_controller_start_blink(2000, light_white);

    // Initialize DHT reader and get queue handle
    QueueHandle_t dht_queue = dht_init();
    if (dht_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize DHT reader");
        return ESP_FAIL;
    }

    // Create the interactor task that processes sensor data
    BaseType_t task_created = xTaskCreate(
        sample_interactor_task,
        "sample_interactor",
        4096,
        (void *)dht_queue,
        5,
        NULL
    );

    if (task_created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create sample interactor task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sample interactor started successfully");
    return ESP_OK;
}

