#include <stdio.h>
#include "esp_log.h"
#include "sample_interactor.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t ret = sample_interactor_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start sample interactor: %s", esp_err_to_name(ret));
    }
}
