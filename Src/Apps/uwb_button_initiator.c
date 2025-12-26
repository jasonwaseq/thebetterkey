/**
 * @file    uwb_button_initiator.c
 *
 * @brief   UWB button-triggered initiator for FiRa
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

#include "uwb_button_initiator.h"
#include "button_handler.h"
#include "HAL_servo.h"
#include "deca_dbg.h"
#include "fira_helper.h"
#include "fira_app_config.h"
#include "reporter.h"
#include <FreeRTOS.h>
#include <timers.h>
#include <task.h>
#include <stdio.h>
#include <string.h>

/* External FiRa context and session from fira_app.c */
extern struct fira_context fira_ctx;
extern uint32_t session_id;

static bool is_ranging = false;
static TimerHandle_t ranging_burst_timer = NULL;
static uint8_t button_press_counter = 0;  /* Increments on each button press */
static bool payload_sent_this_press = false;  /* Tracks if payload was sent for current press */
static TaskHandle_t button_send_task_handle = NULL;  /* Task for sending button data */
static volatile bool pending_button_press = false;  /* Flag set by ISR, cleared by task */

/**
 * @brief Timer callback to stop ranging after burst
 */
static void ranging_burst_timeout(TimerHandle_t xTimer)
{
    (void)xTimer;
    uwb_button_initiator_stop_ranging();
    char str[] = "UWB: Burst complete\r\n";
    reporter_instance.print(str, strlen(str));
}

/**
 * @brief Task to send button data (runs in task context, not ISR)
 */
static void button_send_task(void *pvParameters)
{
    (void)pvParameters;
    
    for (;;)
    {
        /* Wait for button press notification */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        char task_log[] = "BTN_TASK: Processing button press\r\n";
        reporter_instance.print(task_log, strlen(task_log));
        
        /* Process pending button press */
        if (pending_button_press)
        {
            pending_button_press = false;
            
            /* Always send SP1 payload on button press, regardless of RFRAME */


            /* Create data parameters structure (libuwbstack signature) */
            uint8_t payload_data[4] = {'B', 'T', 'N', button_press_counter};
            char data_log[128];
            snprintf(data_log, sizeof(data_log),
                "BTN_TASK: Creating SP1 data_parameters: BTN=%u, session_id=%u, payload_len=%d\r\n",
                button_press_counter, session_id, (int)sizeof(payload_data));
            reporter_instance.print(data_log, strlen(data_log));

            struct data_parameters data_params;
            memset(&data_params, 0, sizeof(data_params));
            memcpy(data_params.data_payload, payload_data, sizeof(payload_data));
            data_params.data_payload_len = (int)sizeof(payload_data);

            char send_log[128];
            int slen = snprintf(send_log, sizeof(send_log),
                "BTN_TASK: Sending BTN payload [B,T,N,%u] to session=%u\r\n",
                button_press_counter, session_id);
            reporter_instance.print(send_log, slen);

            /* Send data via FiRa stack (task context) */
            int ret = fira_helper_send_data(&fira_ctx, session_id, &data_params);
            char result_log[128];
            snprintf(result_log, sizeof(result_log),
                "BTN_TASK: fira_helper_send_data returned %d, session_id=%u, payload=[%02X %02X %02X %02X] %s\r\n",
                ret, session_id,
                data_params.data_payload[0], data_params.data_payload[1],
                data_params.data_payload[2], data_params.data_payload[3],
                (ret == 0) ? "(SUCCESS)" : "(FAILED)");
            reporter_instance.print(result_log, strlen(result_log));
            // Deep debug: print full data_params after send
            char debug_log[128];
            snprintf(debug_log, sizeof(debug_log),
                "[DEBUG] After send: data_payload_len=%d, data_payload=[%02X %02X %02X %02X] (first 4 bytes)\r\n",
                data_params.data_payload_len,
                data_params.data_payload[0], data_params.data_payload[1],
                data_params.data_payload[2], data_params.data_payload[3]);
            reporter_instance.print(debug_log, strlen(debug_log));
        }
    }
}

/**
 * @brief Button event callback for initiator (task context)
 *
 * Invoked from debounce timer processing (not an IRQ). Set a flag and
 * notify the sender task using non-ISR FreeRTOS API.
 */
static void button_event_callback(button_id_e button_id, bool is_pressed)
{
    char debug_str[64];
    int len = snprintf(debug_str, sizeof(debug_str),
        "BTN_CB[%d] %s\r\n", button_id, is_pressed ? "PRESS" : "RELEASE");
    reporter_instance.print(debug_str, len);
    
    if (is_pressed)
    {
        /* Increment button counter for new press */
        button_press_counter++;
        payload_sent_this_press = false;
        
        char btn_press_log[96];
        int blen = snprintf(btn_press_log, sizeof(btn_press_log),
            "BTN_ISR: Button %d pressed (counter=%u)\r\n", button_id, button_press_counter);
        reporter_instance.print(btn_press_log, blen);
        
        /* Set flag and notify task to send data (task context) */
        pending_button_press = true;
        if (button_send_task_handle != NULL)
        {
            /* Use non-ISR notify since we're in timer/task context */
            xTaskNotifyGive(button_send_task_handle);
            
            char notified_str[] = "BTN: Task notified\r\n";
            reporter_instance.print(notified_str, strlen(notified_str));
        }
    }
}

void uwb_button_initiator_init(void)
{
    char init_str[] = "INIT: Button Initiator starting\r\n";
    reporter_instance.print(init_str, strlen(init_str));
    
    /* Create task to handle button data sending (must run in task context, not ISR) */
    BaseType_t task_result = xTaskCreate(
        button_send_task,
        "ButtonSend",
        512,  /* Stack size */
        NULL,
        2,    /* Priority */
        &button_send_task_handle
    );
    
    if (task_result != pdPASS)
    {
        char err_str[] = "INIT: ERROR - Failed to create button send task\r\n";
        reporter_instance.print(err_str, strlen(err_str));
    }
    else
    {
        char task_str[] = "INIT: Button send task created\r\n";
        reporter_instance.print(task_str, strlen(task_str));
    }
    
    /* Initialize button handler */
    button_handler_init();
    
    char btn_str[] = "INIT: Button handler initialized\r\n";
    reporter_instance.print(btn_str, strlen(btn_str));
    
    /* Register button callback */
    button_handler_register_callback(button_event_callback);
    
    char cb_str[] = "INIT: Button callback registered\r\n";
    reporter_instance.print(cb_str, strlen(cb_str));
    
    /* No servo on initiator: responder board handles servo movement */
    
    /* Create timer for ranging burst timeout (5000ms) */
    ranging_burst_timer = xTimerCreate(
        "RangingBurst",
        pdMS_TO_TICKS(5000),
        pdFALSE,  /* One-shot timer */
        NULL,
        ranging_burst_timeout
    );
    
    is_ranging = false;
    
    char done_str[] = "INIT: Button Initiator ready\r\n";
    reporter_instance.print(done_str, strlen(done_str));
}

void uwb_button_initiator_start_ranging(void)
{
    /* Ranging is always on for proper sync. Button triggers servo movement. */
    char str[] = "UWB: Button pressed! Triggering servo on responder...\r\n";
    reporter_instance.print(str, strlen(str));
    
    is_ranging = true;
    /* Initiator only sends BTN payload; responder moves its servo upon receipt */
    /* Do not call responder functions locally on initiator */
    
    /* Start the timer after 5 seconds */
    if (ranging_burst_timer != NULL) {
        xTimerStart(ranging_burst_timer, 0);
    }
}

void uwb_button_initiator_stop_ranging(void)
{
    if (is_ranging)
    {
        char str[] = "UWB: Timer complete.\r\n";
        reporter_instance.print(str, strlen(str));
        
        is_ranging = false;
    }
}

bool uwb_button_initiator_is_ranging(void)
{
    return is_ranging;
}
