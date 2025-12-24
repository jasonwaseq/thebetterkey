/**
 * @file    uwb_servo_responder.c
 *
 * @brief   UWB responder that controls servo on received signals
 *
 * @author  Decawave Applications
 *
 * @attention Copyright (c) 2021 - 2022, Qorvo US, Inc.
 * All rights reserved
 * Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 3. You may only use this software, with or without any modification, with an
 *  integrated circuit developed by Qorvo US, Inc. or any of its affiliates
 *  (collectively, "Qorvo"), or any module that contains such integrated circuit.
 * 4. You may not reverse engineer, disassemble, decompile, decode, adapt, or
 *  otherwise attempt to derive or gain access to the source code to any software
 *  distributed under this license in binary or object code form, in whole or in
 *  part.
 * 5. You may not use any Qorvo name, trademarks, service marks, trade dress,
 *  logos, trade names, or other symbols or insignia identifying the source of
 *  Qorvo's products or services, or the names of any of Qorvo's developers to
 *  endorse or promote products derived from this software without specific prior
 *  written permission from Qorvo US, Inc. You must not call products derived from
 *  this software "Qorvo", you must not have "Qorvo" appear in their name, without
 *  the prior permission from Qorvo US, Inc.
 * 6. Qorvo may publish revised or new version of this license from time to time.
 *  No one other than Qorvo US, Inc. has the right to modify the terms applicable
 *  to the software provided under this license.
 * THIS SOFTWARE IS PROVIDED BY QORVO US, INC. "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. NEITHER
 *  QORVO, NOR ANY PERSON ASSOCIATED WITH QORVO MAKES ANY WARRANTY OR
 *  REPRESENTATION WITH RESPECT TO THE COMPLETENESS, SECURITY, RELIABILITY, OR
 *  ACCURACY OF THE SOFTWARE, THAT IT IS ERROR FREE OR THAT ANY DEFECTS WILL BE
 *  CORRECTED, OR THAT THE SOFTWARE WILL OTHERWISE MEET YOUR NEEDS OR EXPECTATIONS.
 * IN NO EVENT SHALL QORVO OR ANYBODY ASSOCIATED WITH QORVO BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include "uwb_servo_responder.h"
#include "HAL_servo.h"
#include "deca_dbg.h"
#include "nrf_gpio.h"
#include "custom_board.h"
#include "reporter.h"
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <timers.h>

/* LED control helper functions */
static const uint32_t led_pins[] = {LED_1, LED_2, LED_3, LED_4};
static const uint32_t num_leds = 4;

static void led_helper_init(void)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        nrf_gpio_cfg_output(led_pins[i]);
        nrf_gpio_pin_set(led_pins[i]);  /* Set LED off (active LOW) */
    }
}

static void led_helper_all_on(void)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        nrf_gpio_pin_clear(led_pins[i]);  /* active LOW */
    }
}

static void led_helper_all_off(void)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        nrf_gpio_pin_set(led_pins[i]);  /* active LOW */
    }
}

static void led_helper_flash_all(void)
{
    /* Turn on all LEDs */
    led_helper_all_on();
    
    /* Delay ~100ms */
    for (volatile uint32_t i = 0; i < 6400000; i++);
    
    /* Turn off all LEDs */
    led_helper_all_off();
    
    /* Delay ~100ms */
    for (volatile uint32_t i = 0; i < 6400000; i++);
}

/* Timer handle for returning servo to neutral position */
static TimerHandle_t servo_return_timer = NULL;

/* Servo return timeout in milliseconds */
#define SERVO_RETURN_TIMEOUT_MS 2000

/* Cooldown to prevent rapid triggering */
static uint32_t last_trigger_time = 0;
#define SERVO_COOLDOWN_MS 1500  /* 1.5 second cooldown between servo triggers */

/* Toggle state for servo position */
static bool servo_position_state = false;  /* false = left, true = right */

/**
 * @brief Timer callback to return servo to neutral position
 */
static void servo_return_timer_callback(TimerHandle_t xTimer)
{
    uwb_servo_responder_return_to_neutral();
}

void uwb_servo_responder_init(void)
{
    /* Initialize LED helper for debugging */
    led_helper_init();
    
    /* Initialize servo control */
    HAL_servo_init();
    
    /* Return servo to neutral position initially */
    HAL_servo_move_to_position(SERVO_POS_CENTER);
    
    /* Create timer for automatic return to neutral */
    servo_return_timer = xTimerCreate(
        "ServoReturnTimer",
        pdMS_TO_TICKS(SERVO_RETURN_TIMEOUT_MS),
        pdFALSE,  /* Not auto-reload */
        NULL,     /* Timer ID */
        servo_return_timer_callback
    );
    
    /* Servo responder initialized */
    char init_str[] = "RESP: Servo responder initialized\r\n";
    reporter_instance.print(init_str, strlen(init_str));
}

void uwb_servo_responder_signal_received(void)
{
    /* Check cooldown to prevent rapid triggering */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((current_time - last_trigger_time) < SERVO_COOLDOWN_MS)
    {
        return;  /* Still in cooldown period, ignore */
    }
    last_trigger_time = current_time;
    
    /* Debug output */
    char debug_str[] = "RESP: Signal received! Flashing LEDs\r\n";
    reporter_instance.print(debug_str, strlen(debug_str));
    
    /* Flash all LEDs multiple times to indicate signal received */
    for (int i = 0; i < 3; i++)
    {
        led_helper_flash_all();
    }
    
    if (HAL_servo_is_ready())
    {
        /* Toggle between two positions */
        if (servo_position_state)
        {
            /* Move to left position */
            HAL_servo_move_to_position(SERVO_POS_MIN);  /* 1100us */
            char pos_str[] = "SERVO: Moving LEFT\r\n";
            reporter_instance.print(pos_str, strlen(pos_str));
        }
        else
        {
            /* Move to right position */
            HAL_servo_move_to_position(SERVO_POS_MAX);  /* 1650us */
            char pos_str[] = "SERVO: Moving RIGHT\r\n";
            reporter_instance.print(pos_str, strlen(pos_str));
        }
        
        /* Toggle state for next trigger */
        servo_position_state = !servo_position_state;
        
        /* Don't auto-return to center - stay in position */
        /* Stop any existing timer */
        if (servo_return_timer != NULL)
        {
            xTimerStop(servo_return_timer, 0);
        }
    }
}

void uwb_servo_responder_move_servo(uint16_t position_us)
{
    if (HAL_servo_is_ready())
    {
        HAL_servo_set_position(position_us);
        
        /* Stop any existing timer */
        if (servo_return_timer != NULL)
        {
            xTimerStop(servo_return_timer, 0);
        }
        
        /* Start timer to return to neutral position after timeout */
        if (servo_return_timer != NULL)
        {
            xTimerStart(servo_return_timer, 0);
        }
    }
}

void uwb_servo_responder_return_to_neutral(void)
{
    if (HAL_servo_is_ready())
    {
        HAL_servo_move_to_position(SERVO_POS_CENTER);
        
        /* Stop timer if it's running */
        if (servo_return_timer != NULL)
        {
            xTimerStop(servo_return_timer, 0);
        }
    }
}
