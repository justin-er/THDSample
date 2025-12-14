#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "led_strip.h"
#include "led_controller.h"

static const char *TAG = "led_controller";

// State variables
static led_strip_handle_t led_strip = NULL;
static gpio_num_t gpio_num = GPIO_NUM_NC;
static uint32_t num_leds = 0;
static led_controller_backend_t backend = LED_CONTROLLER_BACKEND_RMT;

static TaskHandle_t blink_task_handle = NULL;
static uint32_t blink_period_ms = 0;
static led_controller_color_t blink_color = {0, 0, 0};
static bool blink_active = false;
static bool initialized = false;

// Color constants
led_controller_color_t led_controller_red = {255, 0, 0};
led_controller_color_t led_controller_green = {0, 255, 0};
led_controller_color_t led_controller_blue = {0, 0, 255};

// Static function declarations
static void blink_task_strip(void *pvParameters);
static void blink_task_gpio(void *pvParameters);

static bool is_strip_mode(void)
    {
    return (led_strip != NULL);
}

static void set_led_strip_color(led_controller_color_t color)
{
    if (led_strip != NULL)
    {
        led_strip_set_pixel(led_strip, 0, color.red, color.green, color.blue);
        led_strip_refresh(led_strip);
    }
}

static void clear_led_strip(void)
{
    if (led_strip != NULL)
    {
        led_strip_clear(led_strip);
    }
}

static void set_gpio_level(uint8_t level)
{
    if (gpio_num != GPIO_NUM_NC)
    {
        gpio_set_level(gpio_num, level);
    }
}

void led_controller_deinit(void)
{
    if (!initialized)
    {
        return;
    }

    // Stop blinking if active
    if (blink_active)
    {
        blink_active = false;
        if (blink_task_handle != NULL)
        {
            vTaskDelete(blink_task_handle);
            blink_task_handle = NULL;
        }
    }

    // Clean up LED strip if initialized
    if (led_strip != NULL)
    {
        led_strip_del(led_strip);
        led_strip = NULL;
    }

    // Reset GPIO if it was configured
    if (gpio_num != GPIO_NUM_NC)
    {
        gpio_reset_pin(gpio_num);
        gpio_num = GPIO_NUM_NC;
    }

    num_leds = 0;
    initialized = false;
    ESP_LOGI(TAG, "LED controller deinitialized");
}

esp_err_t led_controller_init_gpio(gpio_num_t gpio)
{
    // If already initialized, just return success (singleton pattern)
    if (initialized)
    {
        ESP_LOGD(TAG, "LED controller already initialized, skipping");
        return ESP_OK;
    }

    // Validate GPIO number
    if (gpio < GPIO_NUM_0 || gpio >= GPIO_NUM_MAX)
    {
        ESP_LOGE(TAG, "Invalid GPIO number: %d", gpio);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing GPIO LED on GPIO %d", gpio);
    
    gpio_num = gpio;
    esp_err_t ret = gpio_reset_pin(gpio_num);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to reset GPIO %d: %s", gpio_num, esp_err_to_name(ret));
        gpio_num = GPIO_NUM_NC;
        return ret;
    }

    ret = gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set GPIO %d direction: %s", gpio_num, esp_err_to_name(ret));
        gpio_num = GPIO_NUM_NC;
        return ret;
    }

    set_gpio_level(0);

    initialized = true;
    ESP_LOGI(TAG, "LED controller initialized (GPIO mode)");
    return ESP_OK;
}

esp_err_t led_controller_init_strip(gpio_num_t gpio, uint32_t leds, led_controller_backend_t back)
{
    // If already initialized, just return success (singleton pattern)
    if (initialized)
    {
        ESP_LOGD(TAG, "LED controller already initialized, skipping");
        return ESP_OK;
    }

    // Validate GPIO number
    if (gpio < GPIO_NUM_0 || gpio >= GPIO_NUM_MAX)
    {
        ESP_LOGE(TAG, "Invalid GPIO number: %d", gpio);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate number of LEDs
    if (leds == 0 || leds > 1024)
    {
        ESP_LOGE(TAG, "Invalid number of LEDs: %lu (must be 1-1024)", leds);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate backend
    if (back != LED_CONTROLLER_BACKEND_RMT && back != LED_CONTROLLER_BACKEND_SPI)
    {
        ESP_LOGE(TAG, "Invalid backend: %d", back);
        return ESP_ERR_INVALID_ARG;
    }

    // Check if RMT is supported when RMT backend is selected
    #ifdef SOC_RMT_SUPPORTED
    // RMT is supported
    #else
    if (back == LED_CONTROLLER_BACKEND_RMT)
    {
        ESP_LOGE(TAG, "RMT backend not supported on this chip");
        return ESP_ERR_NOT_SUPPORTED;
    }
    #endif

    ESP_LOGI(TAG, "Initializing LED strip: GPIO %d, %lu LEDs, backend %s", 
             gpio, leds, (back == LED_CONTROLLER_BACKEND_RMT) ? "RMT" : "SPI");

    gpio_num = gpio;
    num_leds = leds;
    backend = back;

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_num,
        .max_leds = num_leds,
    };

    esp_err_t ret;
    if (backend == LED_CONTROLLER_BACKEND_RMT)
    {
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
        ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create RMT LED strip device: %s", esp_err_to_name(ret));
            gpio_num = GPIO_NUM_NC;
            num_leds = 0;
            return ret;
        }
    }
    else // SPI
    {
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
        ret = led_strip_new_spi_device(&strip_config, &spi_config, &led_strip);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create SPI LED strip device: %s", esp_err_to_name(ret));
            gpio_num = GPIO_NUM_NC;
            num_leds = 0;
            return ret;
        }
    }

    clear_led_strip();

    initialized = true;
    ESP_LOGI(TAG, "LED controller initialized (LED strip mode)");
    return ESP_OK;
}

void led_controller_set_color(led_controller_color_t color)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "LED controller not initialized. Call led_controller_init_gpio() or led_controller_init_strip() first.");
        return;
    }

    // Stop blinking if active
    if (blink_active)
    {
        led_controller_stop_blink();
    }

    if (is_strip_mode())
    {
        set_led_strip_color(color);
}
    else
    {
        // For GPIO mode, use red component to determine on/off
        set_gpio_level(color.red > 0 ? 1 : 0);
    }
}

void led_controller_set(void)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "LED controller not initialized. Call led_controller_init_gpio() or led_controller_init_strip() first.");
        return;
    }

    // Stop blinking if active
    if (blink_active)
    {
        led_controller_stop_blink();
    }

    if (is_strip_mode())
    {
        // For LED strip mode, use white for on
        led_controller_color_t white = {255, 255, 255};
        set_led_strip_color(white);
    }
    else
    {
        // For GPIO mode, turn on
        set_gpio_level(1);
    }
}

void led_controller_clear(void)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "LED controller not initialized. Call led_controller_init_gpio() or led_controller_init_strip() first.");
        return;
    }

    // Stop blinking if active
    if (blink_active)
    {
        led_controller_stop_blink();
}

    if (is_strip_mode())
    {
        clear_led_strip();
    }
    else
    {
        set_gpio_level(0);
    }
}

void led_controller_start_blink(uint32_t period, led_controller_color_t color)
{
    if (!initialized)
{
        ESP_LOGE(TAG, "LED controller not initialized. Call led_controller_init_gpio() or led_controller_init_strip() first.");
        return;
    }

    if (!is_strip_mode())
    {
        ESP_LOGE(TAG, "led_controller_start_blink() can only be used in LED strip mode. Use led_controller_start_blink_gpio() for GPIO mode.");
        return;
    }

    // Stop existing blink task if running
    if (blink_active)
    {
        led_controller_stop_blink();
    }

    // Update blink parameters
    blink_period_ms = period;
    blink_color = color;
    blink_active = true;

    // Create new blink task for LED strip
    BaseType_t result = xTaskCreate(
        blink_task_strip,
        "led_blink_strip_task",
        2048,
        NULL,
        5,
        &blink_task_handle
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create blink task");
        blink_active = false;
        blink_task_handle = NULL;
}
    else
    {
        ESP_LOGI(TAG, "Started blinking with period %lu ms", period);
    }
}

void led_controller_start_blink_gpio(uint32_t period)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "LED controller not initialized. Call led_controller_init_gpio() or led_controller_init_strip() first.");
        return;
    }

    if (is_strip_mode())
    {
        ESP_LOGE(TAG, "led_controller_start_blink_gpio() can only be used in GPIO mode. Use led_controller_start_blink() for LED strip mode.");
        return;
    }

    // Stop existing blink task if running
    if (blink_active)
    {
        led_controller_stop_blink();
    }

    // Update blink parameters
    blink_period_ms = period;
    blink_active = true;

    // Create new blink task for GPIO
    BaseType_t result = xTaskCreate(
        blink_task_gpio,
        "led_blink_gpio_task",
        2048,
        NULL,
        5,
        &blink_task_handle
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create blink task");
        blink_active = false;
        blink_task_handle = NULL;
    }
    else
    {
        ESP_LOGI(TAG, "Started GPIO blinking with period %lu ms", period);
    }
}

void led_controller_stop_blink(void)
{
    if (!blink_active)
    {
        return;
    }

    blink_active = false;

    // Delete the task immediately (vTaskDelete returns immediately)
    if (blink_task_handle != NULL)
    {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
    }

    // Clear LED directly (avoid calling led_controller_clear() to prevent circular call)
    if (is_strip_mode())
    {
        clear_led_strip();
    }
    else
    {
        set_gpio_level(0);
    }

    ESP_LOGI(TAG, "Stopped blinking");
}

static void blink_task_strip(void *pvParameters)
{
    bool led_state = false;

    while (blink_active)
    {
        if (led_state)
        {
            set_led_strip_color(blink_color);
        }
        else
        {
            clear_led_strip();
        }

        led_state = !led_state;
        vTaskDelay(pdMS_TO_TICKS(blink_period_ms));
    }

    // Task cleanup
    blink_task_handle = NULL;
    vTaskDelete(NULL);
}

static void blink_task_gpio(void *pvParameters)
{
    bool led_state = false;

    while (blink_active)
    {
        if (led_state)
        {
            set_gpio_level(1);
        }
        else
        {
            set_gpio_level(0);
        }

        led_state = !led_state;
        vTaskDelay(pdMS_TO_TICKS(blink_period_ms));
    }

    // Task cleanup
    blink_task_handle = NULL;
    vTaskDelete(NULL);
}
