#include <stdio.h>

typedef struct led_controllerl_color_t
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
} led_controllerl_color_t;

void led_controller_init(void);
void led_controller_set(struct led_controllerl_color_t color);
void led_controller_clear(void);
void led_controller_start_blink(uint32_t period, struct led_controllerl_color_t color);
void led_controller_stop_blink();

extern led_controllerl_color_t led_controllerl_red;
extern led_controllerl_color_t led_controllerl_green;
extern led_controllerl_color_t led_controllerl_blue;
