#include "esp_log.h"
#include "led_controller.h"
#include "humidity_indicator.h"
#include "app_wifi.h"

static const char *TAG = "main";

void app_main(void)
{
    // Initialize shared services first
    esp_err_t ret = led_controller_init_strip(GPIO_NUM_48, 1, LED_CONTROLLER_BACKEND_RMT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize LED controller: %s", esp_err_to_name(ret));
        return;
    }

    // Start WiFi Access Point
    wifi_app_start();

    // Start application components
    ret = humidity_indicator_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start humidity indicator: %s", esp_err_to_name(ret));
    }
}
