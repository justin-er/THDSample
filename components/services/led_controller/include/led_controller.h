#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

typedef enum {
    LED_CONTROLLER_BACKEND_RMT,
    LED_CONTROLLER_BACKEND_SPI
} led_controller_backend_t;

typedef struct led_controller_color_t {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
} led_controller_color_t;

// GPIO mode initialization
esp_err_t led_controller_init_gpio(gpio_num_t gpio_num);

// LED strip mode initialization
esp_err_t led_controller_init_strip(gpio_num_t gpio_num, uint32_t num_leds, led_controller_backend_t backend);

/**
 * @brief Turn LED on (simple on/off control)
 * 
 * For GPIO mode: Sets GPIO to HIGH (turns LED on)
 * For LED strip mode: Sets LED to white color (255, 255, 255)
 * 
 * Use this function when you just want to turn the LED on without specifying a color.
 * For GPIO mode, this is the simplest way to turn the LED on.
 */
void led_controller_set(void);

/**
 * @brief Set LED to a specific RGB color
 * 
 * For GPIO mode: Uses red component to determine on/off (red > 0 = on, red = 0 = off)
 * For LED strip mode: Sets LED to the specified RGB color
 * 
 * Use this function when you need to set a specific color (e.g., red, green, blue, or custom colors).
 * For LED strip mode, this is the primary way to set colors.
 * 
 * @param color RGB color values (0-255 for each component)
 */
void led_controller_set_color(led_controller_color_t color);

/**
 * @brief Turn LED off
 * 
 * For GPIO mode: Sets GPIO to LOW (turns LED off)
 * For LED strip mode: Clears all LEDs (turns them off)
 * 
 * Stops any active blinking before clearing the LED.
 */
void led_controller_clear(void);

/**
 * @brief Start blinking LED strip with a specific color (LED strip mode only)
 * 
 * This function can only be used when initialized in LED strip mode.
 * For GPIO mode, use led_controller_start_blink_gpio() instead.
 * 
 * Creates a FreeRTOS task that toggles the LED strip on/off at the specified period.
 * The LED will blink with the specified RGB color when on.
 * If blinking is already active, it will stop the current blink and start a new one.
 * 
 * @param period Blink period in milliseconds (time for one on+off cycle)
 * @param color RGB color to use when LED is on
 */
void led_controller_start_blink(uint32_t period, led_controller_color_t color);

/**
 * @brief Start blinking GPIO LED (GPIO mode only)
 * 
 * Simplified blink function for GPIO mode that doesn't require a color parameter.
 * The LED will blink on/off at the specified period.
 * 
 * This function can only be used when initialized in GPIO mode.
 * For LED strip mode, use led_controller_start_blink() instead.
 * 
 * @param period Blink period in milliseconds (time for one on+off cycle)
 */
void led_controller_start_blink_gpio(uint32_t period);

/**
 * @brief Stop blinking LED
 * 
 * Stops the active blink task and clears the LED (turns it off).
 * Safe to call even if blinking is not active.
 */
void led_controller_stop_blink(void);

extern led_controller_color_t led_controller_red;
extern led_controller_color_t led_controller_green;
extern led_controller_color_t led_controller_blue;
