/**
 * @file      HAL_led_helper.h
 *
 * @brief     Simple LED control helper functions
 *
 * @author    Debug Helper
 *
 */

#ifndef HAL_LED_HELPER_H
#define HAL_LED_HELPER_H

#include "nrf_gpio.h"
#include "custom_board.h"

/**
 * @brief Initialize all LEDs as GPIO outputs
 */
void led_helper_init(void);

/**
 * @brief Turn on a specific LED (LED 1-4)
 */
void led_helper_on(int led_num);

/**
 * @brief Turn off a specific LED (LED 1-4)
 */
void led_helper_off(int led_num);

/**
 * @brief Turn on all LEDs
 */
void led_helper_all_on(void);

/**
 * @brief Turn off all LEDs
 */
void led_helper_all_off(void);

/**
 * @brief Flash all LEDs (on for 100ms, off for 100ms)
 */
void led_helper_flash_all(void);

#endif // HAL_LED_HELPER_H
