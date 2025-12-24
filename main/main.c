#include "esp_log.h"
#include "led_controller.h"
#include "humidity_indicator.h"
#include "app_wifi.h"
#include "app_coordinator.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 DHT Application");
    
    // Initialize infrastructure services
    esp_err_t ret = led_controller_init_strip(GPIO_NUM_48, 1, LED_CONTROLLER_BACKEND_RMT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize LED controller: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize application components
    ret = app_coordinator_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start app coordinator: %s", esp_err_to_name(ret));
        return;
    }

    // Start WiFi (will start HTTP server and DNS server automatically)
    wifi_app_start();

    // Start humidity indicator
    ret = humidity_indicator_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start humidity indicator: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "All components started successfully");
}
