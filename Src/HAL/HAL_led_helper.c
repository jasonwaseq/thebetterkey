/**
 * @file      HAL_led_helper.c
 *
 * @brief     Simple LED control helper functions
 *
 * @author    Debug Helper
 *
 */

#include "HAL_led_helper.h"
#include "nrf_gpio.h"
#include "custom_board.h"

// LED pins array for easy access
static const uint32_t led_pins[] = {LED_1, LED_2, LED_3, LED_4};
static const uint32_t num_leds = 4;

void led_helper_init(void)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        nrf_gpio_cfg_output(led_pins[i]);
        // Set LED to off (LEDS_ACTIVE_STATE = 0 means active LOW)
        nrf_gpio_pin_set(led_pins[i]);
    }
}

void led_helper_on(int led_num)
{
    if (led_num >= 1 && led_num <= num_leds)
    {
        // LEDS_ACTIVE_STATE = 0 means active LOW, so write 0 to turn on
        nrf_gpio_pin_clear(led_pins[led_num - 1]);
    }
}

void led_helper_off(int led_num)
{
    if (led_num >= 1 && led_num <= num_leds)
    {
        // LEDS_ACTIVE_STATE = 0 means active LOW, so write 1 to turn off
        nrf_gpio_pin_set(led_pins[led_num - 1]);
    }
}

void led_helper_all_on(void)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        nrf_gpio_pin_clear(led_pins[i]);  // active LOW
    }
}

void led_helper_all_off(void)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        nrf_gpio_pin_set(led_pins[i]);  // active LOW
    }
}

void led_helper_flash_all(void)
{
    // Turn on all LEDs
    led_helper_all_on();
    
    // Delay ~100ms (using simple busy-wait for now)
    // At ~64MHz, roughly 6.4M cycles = 100ms
    for (volatile uint32_t i = 0; i < 6400000; i++);
    
    // Turn off all LEDs
    led_helper_all_off();
    
    // Delay ~100ms
    for (volatile uint32_t i = 0; i < 6400000; i++);
}
