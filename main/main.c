#include <stdio.h>
#include "esp_log.h"
#include "led_controller.h"
#include "sample_interactor.h"

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

    // Start interactors
    ret = sample_interactor_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start sample interactor: %s", esp_err_to_name(ret));
    }
}
