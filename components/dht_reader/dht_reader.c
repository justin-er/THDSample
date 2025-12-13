#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <driver/gpio.h>

#include "dht_reader.h"
#include "sdkconfig.h"

static const char *TAG = "dht_reader";

#define DHT_DATA_GPIO CONFIG_DHT_DATA_GPIO

static QueueHandle_t dht_queue = NULL;

// Simple DHT22 read - bypassing the library entirely
static esp_err_t simple_dht_read(gpio_num_t pin, float *humidity, float *temperature)
{
    uint8_t data[5] = {0};
    
    // Send start signal: pull low for 20ms, then high
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(pin, 1);
    
    // Switch to input and wait for response
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    
    // Wait for DHT to pull low (response signal)
    int timeout = 100;
    while (gpio_get_level(pin) == 1 && timeout > 0) {
        esp_rom_delay_us(1);
        timeout--;
    }
    if (timeout == 0) return ESP_ERR_TIMEOUT;
    
    // Wait for DHT to release (go high)
    timeout = 100;
    while (gpio_get_level(pin) == 0 && timeout > 0) {
        esp_rom_delay_us(1);
        timeout--;
    }
    if (timeout == 0) return ESP_ERR_TIMEOUT;
    
    // Wait for DHT to pull low again (start of data)
    timeout = 100;
    while (gpio_get_level(pin) == 1 && timeout > 0) {
        esp_rom_delay_us(1);
        timeout--;
    }
    if (timeout == 0) return ESP_ERR_TIMEOUT;
    
    // Read 40 bits of data
    for (int i = 0; i < 40; i++) {
        // Wait for high
        timeout = 100;
        while (gpio_get_level(pin) == 0 && timeout > 0) {
            esp_rom_delay_us(1);
            timeout--;
        }
        if (timeout == 0) return ESP_ERR_TIMEOUT;
        
        // Measure high pulse duration
        int high_time = 0;
        while (gpio_get_level(pin) == 1 && high_time < 100) {
            esp_rom_delay_us(1);
            high_time++;
        }
        
        // High pulse > 40us means 1, otherwise 0
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        if (high_time > 40) {
            data[byte_idx] |= (1 << bit_idx);
        }
    }
    
    // Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGE(TAG, "Checksum failed: %d != %d", checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }
    
    // Parse data (DHT22/AM2301 format)
    *humidity = ((data[0] << 8) | data[1]) / 10.0f;
    int16_t temp_raw = ((data[2] & 0x7F) << 8) | data[3];
    if (data[2] & 0x80) temp_raw = -temp_raw;
    *temperature = temp_raw / 10.0f;
    
    return ESP_OK;
}

void dht_task(void *pvParameters)
{
    gpio_num_t pin = DHT_DATA_GPIO;
    ESP_LOGI(TAG, "DHT task started on GPIO %d", pin);
    
    // Configure GPIO with pull-up
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    gpio_set_level(pin, 1);
    
    // Wait for sensor to stabilize after power-on
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    while (1)
    {
        float humidity = 0, temperature = 0;
        esp_err_t result = simple_dht_read(pin, &humidity, &temperature);
        
        if (result == ESP_OK)
        {
            dht_data_t sensor_data = {humidity, temperature};
            
            if (dht_queue != NULL)
            {
                xQueueSend(dht_queue, &sensor_data, portMAX_DELAY);
            }
        }
        else
        {
            ESP_LOGW(TAG, "DHT read failed: %s", esp_err_to_name(result));
        }
        
        // DHT sensors need at least 2 seconds between reads
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

QueueHandle_t dht_init(void)
{
    ESP_LOGI(TAG, "Initializing DHT reader on GPIO %d", DHT_DATA_GPIO);
    
    // Create a queue to hold dht_data_t
    dht_queue = xQueueCreate(1, sizeof(dht_data_t));
    if (dht_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create DHT queue");
        return NULL;
    }
    
    // Create the DHT reading task
    BaseType_t ret = xTaskCreate(dht_task, "dht_task", 4096, NULL, 5, NULL);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create DHT task");
        vQueueDelete(dht_queue);
        return NULL;
    }
    
    ESP_LOGI(TAG, "DHT reader initialized successfully");
    return dht_queue;
}


