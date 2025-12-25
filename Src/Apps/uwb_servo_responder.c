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
#include <task.h>
#include <queue.h>
/* Add near the top with other includes */
#include "fira_helper.h"
#include "fira_app_config.h"


/* Add external references */
extern struct fira_context fira_ctx;
extern uint32_t session_id;

/**
 * @brief FiRa notification callback for responder
 * Called for each ranging result, including received SP1 payloads
 */

/**
 * @brief FiRa data reception callback for responder
 * Called when proprietary payload is received from initiator
 */

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

static void led_helper_flash_all_blocking(uint32_t flashes, uint32_t ms)
{
    for (uint32_t i = 0; i < flashes; i++)
    {
        led_helper_all_on();
        vTaskDelay(pdMS_TO_TICKS(ms));
        led_helper_all_off();
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

/* Timer handle for returning servo to neutral position */
static TimerHandle_t servo_return_timer = NULL;

/* Servo return timeout in milliseconds */
#define SERVO_RETURN_TIMEOUT_MS 2000

/* Toggle state for servo position */
static bool servo_position_state = false;  /* false -> drive right first, true -> drive left first */
static uint8_t last_button_counter = 0;  /* Track last processed button counter */

/* Async processing of responder actions */
static QueueHandle_t responder_event_queue = NULL; /* queue of uint8_t button counters */
static TaskHandle_t responder_worker_task_handle = NULL;

static void responder_worker_task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t btn_counter;
    for (;;)
    {
        if (xQueueReceive(responder_event_queue, &btn_counter, portMAX_DELAY) == pdTRUE)
        {
            /* Debug output */
            char debug_str[128];
            snprintf(debug_str, sizeof(debug_str),
                "RESP: Worker handling button (counter=%u) at tick=%lu\r\n",
                btn_counter, (unsigned long)xTaskGetTickCount());
            reporter_instance.print(debug_str, strlen(debug_str));

            /* Flash LEDs non-blocking (uses vTaskDelay) */
            led_helper_flash_all_blocking(3, 100);

            if (HAL_servo_is_ready())
            {
                /* Toggle between full left and full right */
                uint16_t target_us = servo_position_state ? SERVO_POS_MIN : SERVO_POS_MAX;
                HAL_servo_set_position(target_us);

                char pos_str[64];
                int len = snprintf(pos_str, sizeof(pos_str),
                    "SERVO: Moving to %s (%u us)\r\n",
                    servo_position_state ? "LEFT" : "RIGHT", (unsigned)target_us);
                reporter_instance.print(pos_str, len);

                /* Flip state so the next signal moves in the opposite direction */
                servo_position_state = !servo_position_state;

                /* Stop any existing timer (stay in position) */
                if (servo_return_timer != NULL)
                {
                    xTimerStop(servo_return_timer, 0);
                }
            }
        }
    }
}

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
        pdFALSE,
        NULL,
        servo_return_timer_callback
    );
    
    /* Create responder worker queue & task */
    responder_event_queue = xQueueCreate(8, sizeof(uint8_t));
    if (responder_event_queue != NULL)
    {
        BaseType_t tr = xTaskCreate(
            responder_worker_task,
            "RespWorker",
            768,
            NULL,
            2,
            &responder_worker_task_handle);
        if (tr != pdPASS)
        {
            char err_str[] = "RESP: ERROR - Worker task create failed\r\n";
            reporter_instance.print(err_str, strlen(err_str));
        }
    }
    
    /* Servo responder initialized */
    char init_str[] = "RESP: Servo responder initialized\r\n";
    reporter_instance.print(init_str, strlen(init_str));

    // NOTE: The notification callback must be registered in fira_helper_open() elsewhere in your startup code:
    // fira_helper_open(&fira_ctx, ..., responder_notification_callback, ...);
}

void uwb_servo_responder_signal_received(uint8_t button_counter)
{
    /* Deduplicate within responder to avoid repeated triggers for same counter */
    if (button_counter == last_button_counter)
    {
        char dedup_log[96];
        snprintf(dedup_log, sizeof(dedup_log),
            "RESP: signal_received dedup BTN=%u (last=%u)\r\n",
            button_counter, last_button_counter);
        reporter_instance.print(dedup_log, strlen(dedup_log));
        return;
    }
    char recv_log[128];
    snprintf(recv_log, sizeof(recv_log),
        "RESP: signal_received BTN=%u, last=%u, tick=%lu\r\n",
        button_counter, last_button_counter, (unsigned long)xTaskGetTickCount());
    reporter_instance.print(recv_log, strlen(recv_log));
    last_button_counter = button_counter;

    /* Enqueue for worker task; keep callback light to avoid stalling FiRa processing */
    if (responder_event_queue != NULL)
    {
        uint8_t bc = button_counter;
        BaseType_t qret = xQueueSend(responder_event_queue, &bc, 0);
        char qlog[96];
        snprintf(qlog, sizeof(qlog),
            "RESP: signal_received queue send ret=%ld\r\n", (long)qret);
        reporter_instance.print(qlog, strlen(qlog));
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
