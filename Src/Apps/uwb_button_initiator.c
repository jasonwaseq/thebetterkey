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
#include "reporter.h"
#include <FreeRTOS.h>
#include <timers.h>
#include <stdio.h>
#include <string.h>

/* External FiRa context and session from fira_app.c */
extern struct fira_context fira_ctx;
extern uint32_t session_id;

static bool is_ranging = false;
static TimerHandle_t ranging_burst_timer = NULL;

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
 * @brief Button event callback for initiator
 *
 * Sends short ranging burst on button press
 */
static void button_event_callback(button_id_e button_id, bool is_pressed)
{
    char debug_str[64];
    int len = snprintf(debug_str, sizeof(debug_str),
        "BTN_CB[%d] %s\r\n", button_id, is_pressed ? "PRESS" : "RELEASE");
    reporter_instance.print(debug_str, len);
    
    if (is_pressed)
    {
        /* Button pressed - send short ranging burst */
        if (!is_ranging)
        {
            uwb_button_initiator_start_ranging();

            /* Send explicit SP1 payload marking button press */
            /* Payload format: 'B','T','N', <button_id> */
            struct data_parameters btn_params = {0};
            btn_params.data_payload[0] = 'B';
            btn_params.data_payload[1] = 'T';
            btn_params.data_payload[2] = 'N';
            btn_params.data_payload[3] = (uint8_t)button_id;
            btn_params.data_payload_len = 4;

            int r = fira_helper_send_data(&fira_ctx, session_id, &btn_params);
            (void)r; /* ignore return in this context */
            char send_str[] = "UWB: Sent BTN payload\r\n";
            reporter_instance.print(send_str, strlen(send_str));

            /* Start timer to stop ranging after 300ms */
            if (ranging_burst_timer != NULL)
            {
                xTimerStart(ranging_burst_timer, 0);
            }
        }
    }
}

void uwb_button_initiator_init(void)
{
    char init_str[] = "INIT: Button Initiator starting\r\n";
    reporter_instance.print(init_str, strlen(init_str));
    
    /* Initialize button handler */
    button_handler_init();
    
    char btn_str[] = "INIT: Button handler initialized\r\n";
    reporter_instance.print(btn_str, strlen(btn_str));
    
    /* Register button callback */
    button_handler_register_callback(button_event_callback);
    
    char cb_str[] = "INIT: Button callback registered\r\n";
    reporter_instance.print(cb_str, strlen(cb_str));
    
    /* Initialize servo (responder will move servo on received signals) */
    HAL_servo_init();
    
    /* Create timer for ranging burst timeout (300ms) */
    ranging_burst_timer = xTimerCreate(
        "RangingBurst",
        pdMS_TO_TICKS(300),
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
    if (!is_ranging)
    {
        char str[] = "UWB: Button pressed, sending data...\r\n";
        reporter_instance.print(str, strlen(str));
        
        is_ranging = true;
        /* Session already running - no need to start/stop */
    }
}

void uwb_button_initiator_stop_ranging(void)
{
    if (is_ranging)
    {
        char str[] = "UWB: Burst complete\r\n";
        reporter_instance.print(str, strlen(str));
        
        is_ranging = false;
        /* Session keeps running - just stop sending burst */
    }
}

bool uwb_button_initiator_is_ranging(void)
{
    return is_ranging;
}
