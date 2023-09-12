#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"

static const uint32_t CPU_FREQ = 125000000;
static const uint16_t USB_VALUE_RANGE = 1000;
static const uint8_t GPIO_THROTTLE = 2;
static const uint8_t GPIO_STEERING = 3;
static const uint8_t PWM_SLICE = 1;
static const float PWM_CLKDIV = 38.1875f;
static const uint16_t PWM_WRAP = 65465;
static const uint16_t PWM_SET_LEVEL_FREQ = 30;
static const uint16_t MAX_TIME_WITHOUT_UPDATE_MS = 100;
static const float NEUTRAL_THROTTLE = 0.5f;
static const float NEUTRAL_STEERING = 0.5f;

static const float PWM_MS_LEVEL = CPU_FREQ / PWM_CLKDIV / 1000.f;
static const uint16_t MAX_COUNTER_WITHOUT_UPDATE = MAX_TIME_WITHOUT_UPDATE_MS / 1000.f * PWM_SET_LEVEL_FREQ;

static volatile float throttle = NEUTRAL_THROTTLE;
static volatile float steering = NEUTRAL_STEERING;
static volatile uint16_t set_level_counter = 0;

bool set_levels_callback(struct repeating_timer *t)
{
    uint16_t throttle_level;
    uint16_t steering_level;
    if (set_level_counter < MAX_COUNTER_WITHOUT_UPDATE) {
        throttle_level = PWM_MS_LEVEL + (uint16_t)(throttle * PWM_MS_LEVEL);
        steering_level = PWM_MS_LEVEL + (uint16_t)(steering * PWM_MS_LEVEL);
        set_level_counter++;
    } 
    else {
        throttle_level = 0;
        steering_level = 0;
    }
    pwm_set_both_levels(1, throttle_level, steering_level);
    return true;
}

int main()
{
    gpio_set_function(GPIO_THROTTLE, GPIO_FUNC_PWM);
    gpio_set_function(GPIO_STEERING, GPIO_FUNC_PWM);
    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, PWM_CLKDIV);
    pwm_config_set_wrap(&config, PWM_WRAP);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(PWM_SLICE, &config, true);
    pwm_set_both_levels(PWM_SLICE, PWM_MS_LEVEL + (uint16_t)(NEUTRAL_THROTTLE * PWM_MS_LEVEL),
                                   PWM_MS_LEVEL + (uint16_t)(NEUTRAL_STEERING * PWM_MS_LEVEL));
    struct repeating_timer timer;
    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 500ms later regardless of how long the callback took to execute
    add_repeating_timer_ms(-1000 / PWM_SET_LEVEL_FREQ, set_levels_callback, NULL, &timer);

    stdio_init_all();
    cyw43_arch_init();
    bool led_on = false;

    while (true)
    {
        uint16_t buffer[2];
        size_t n_read = fread(buffer, sizeof(buffer[0]), 2, stdin);
        led_on = !led_on;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        if (n_read != 2) {
            return 1;
        }
        throttle = (float)buffer[0] / USB_VALUE_RANGE;
        steering = (float)buffer[1] / USB_VALUE_RANGE;
        set_level_counter = 0;
    }
    return 0;
}
