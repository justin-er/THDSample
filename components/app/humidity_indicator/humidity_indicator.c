#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "humidity_indicator.h"
#include "app_coordinator.h"
#include "led_controller.h"

static const char *TAG = "humidity_indicator";

// Colors at 50% intensity
static const led_controller_color_t COLOR_GREEN = {0, 32, 0};
static const led_controller_color_t COLOR_ORANGE = {32, 16, 0};
static const led_controller_color_t COLOR_RED = {32, 0, 0};

static void humidity_indicator_task(void *pvParameters)
{
    app_coordinator_sensor_data_t sensor_data;

    ESP_LOGI(TAG, "Humidity indicator task started");

    while (1)
    {
        // Get sensor data from app_coordinator
        esp_err_t ret = app_coordinator_get_sensor_data(&sensor_data);
        
        if (ret == ESP_OK && sensor_data.valid)
        {
            // printf("Humidity: %.1f%% Temp: %.1fC\n", sensor_data.humidity, sensor_data.temperature);

            // Update LED color based on humidity
            if (sensor_data.humidity < 50.0f)
            {
                led_controller_set_color(COLOR_GREEN);
            }
            else if (sensor_data.humidity < 55.0f)
            {
                led_controller_set_color(COLOR_ORANGE);
            }
            else
            {
                led_controller_set_color(COLOR_RED);
            }
        }
        
        // Poll every 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

esp_err_t humidity_indicator_start(void)
{
    ESP_LOGI(TAG, "Starting humidity indicator...");

    // Create the task that processes sensor data and indicates humidity
    BaseType_t task_created = xTaskCreate(
        humidity_indicator_task,
        "humidity_indicator",
        4096,
        NULL,
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

