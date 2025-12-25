/**
 * @file    HAL_servo.c
 *
 * @brief   HAL functions for servo control via PWM
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

#include "HAL_servo.h"
#include "custom_board.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "reporter.h"
#include <string.h>

/* Servo state */
static bool servo_initialized = false;
static uint16_t current_position = SERVO_POS_CENTER;

/* PWM timing constants */
#define SERVO_PWM_FREQ_HZ         50                 /* 50Hz servo frequency */
#define SERVO_PERIOD_MS           20                 /* 20ms period */
#define SERVO_MIN_PULSE_US        1000               /* 1ms minimum pulse width */
#define SERVO_MAX_PULSE_US        2000               /* 2ms maximum pulse width */

/* Servo PWM task handle */
/* Servo PWM task handle */
static TaskHandle_t servo_pwm_task_handle = NULL;

/**
 * @brief FreeRTOS task for servo PWM generation
 * Generates 50Hz PWM with variable pulse width on GPIO pin
 */
static void servo_pwm_task(void *p_arg)
{
    uint16_t pulse_width_us;
    static uint32_t cycle_count = 0;
    
    (void)p_arg;
    
    for (;;)
    {
        /* Get current position safely */
        taskENTER_CRITICAL();
        pulse_width_us = current_position;
        taskEXIT_CRITICAL();
        
            /* Generate PWM pulse using FreeRTOS delays instead of blocking delays */
            nrf_gpio_pin_set(SERVO_PWM_PIN);
            /* Convert microseconds to milliseconds + ticks, with minimum 1ms */
            uint32_t pulse_ms = (pulse_width_us + 500) / 1000;  /* Round to nearest ms */
            if (pulse_ms < 1) pulse_ms = 1;
            vTaskDelay(pdMS_TO_TICKS(pulse_ms));
        
            nrf_gpio_pin_clear(SERVO_PWM_PIN);
            /* Calculate remaining time in 20ms period */
            uint32_t remaining_ms = SERVO_PERIOD_MS - pulse_ms;
            if (remaining_ms < 1) remaining_ms = 1;
            vTaskDelay(pdMS_TO_TICKS(remaining_ms));
        
        /* Debug output every 50 cycles (once per second) */
        cycle_count++;
        if (cycle_count >= 50)
        {
            char pwm_str[64];
            int len = snprintf(pwm_str, sizeof(pwm_str),
                "SERVO_PWM: Pin=%u, Pulse=%u us\r\n", SERVO_PWM_PIN, pulse_width_us);
            reporter_instance.print(pwm_str, len);
            cycle_count = 0;
        }
        
        /* Yield to other tasks briefly */
        vTaskDelay(1);
    }
}

void HAL_servo_init(void)
{
    /* Configure GPIO for servo output */
    nrf_gpio_cfg_output(SERVO_PWM_PIN);
    nrf_gpio_pin_clear(SERVO_PWM_PIN);
    
    char pin_str[64];
    int len = snprintf(pin_str, sizeof(pin_str),
        "SERVO: Configuring GPIO pin %u as output\r\n", SERVO_PWM_PIN);
    reporter_instance.print(pin_str, len);
    
    current_position = SERVO_POS_CENTER;
    
    char init_str[] = "SERVO: Initializing PWM task\r\n";
    reporter_instance.print(init_str, strlen(init_str));
    
    /* Create PWM task */
    BaseType_t xReturned = xTaskCreate(
        servo_pwm_task,
        "ServosPWM",
        128,  /* Stack size in words */
        NULL,
            1,    /* Priority - lower than UWB (3) but higher than idle */
        &servo_pwm_task_handle
    );
    
    if (xReturned == pdPASS)
    {
        servo_initialized = true;
        char ok_str[] = "SERVO: PWM task created successfully\r\n";
        reporter_instance.print(ok_str, strlen(ok_str));
    }
    else
    {
        char fail_str[] = "SERVO: Failed to create PWM task!\r\n";
        reporter_instance.print(fail_str, strlen(fail_str));
    }
}

void HAL_servo_set_position(uint16_t pulse_width_us)
{
    if (!servo_initialized)
    {
        char err_str[] = "SERVO: ERROR - Not initialized!\r\n";
        reporter_instance.print(err_str, strlen(err_str));
        return;
    }
    
    /* Clamp to valid range */
    if (pulse_width_us < SERVO_MIN_PULSE_US)
        pulse_width_us = SERVO_MIN_PULSE_US;
    if (pulse_width_us > SERVO_MAX_PULSE_US)
        pulse_width_us = SERVO_MAX_PULSE_US;
    
    /* Log position change */
    char log_str[64];
    int len = snprintf(log_str, sizeof(log_str),
        "SERVO: Setting position to %u us\r\n", pulse_width_us);
    reporter_instance.print(log_str, len);
    
    /* Update position atomically */
    taskENTER_CRITICAL();
    current_position = pulse_width_us;
    taskEXIT_CRITICAL();
}

void HAL_servo_move_to_position(servo_position_e position)
{
    char pos_str[64];
    int len = snprintf(pos_str, sizeof(pos_str),
        "SERVO: Moving to position %u us\r\n", (uint16_t)position);
    reporter_instance.print(pos_str, len);
    
    HAL_servo_set_position((uint16_t)position);
}

void HAL_servo_stop(void)
{
    if (servo_initialized && servo_pwm_task_handle != NULL)
    {
        vTaskDelete(servo_pwm_task_handle);
        servo_pwm_task_handle = NULL;
        nrf_gpio_pin_clear(SERVO_PWM_PIN);
    }
}

bool HAL_servo_is_ready(void)
{
    return servo_initialized;
}
