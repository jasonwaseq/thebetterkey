/**
 * @file    button_handler.c
 *
 * @brief   Button event handler for SW1 and SW2
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

#include "button_handler.h"
#include "custom_board.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include <FreeRTOS.h>
#include <timers.h>
#include <string.h>
#include "reporter.h"

/* Button configuration */
static const uint32_t button_pins[BUTTON_NUM] = {
    BUTTON_1,  /* SW1 */
    BUTTON_2   /* SW2 */
};

/* Button state tracking */
static struct {
    bool pressed;
    uint8_t debounce_count;
} button_state[BUTTON_NUM];

/* Periodic timer to run debouncing */
static TimerHandle_t button_timer = NULL;

/* Debounce counter threshold (in ms, assuming process called every 5ms) */
#define DEBOUNCE_THRESHOLD 10

/* Event callback */
static button_event_callback_t button_callback = NULL;

/**
 * @brief GPIOTE event handler for buttons
 */
static void gpiote_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    /* Find which button triggered the interrupt */
    for (uint8_t i = 0; i < BUTTON_NUM; i++)
    {
        if (button_pins[i] == pin)
        {
            /* Reset debounce counter on edge */
            button_state[i].debounce_count = 0;
            break;
        }
    }
}

static void button_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    button_handler_process();
}

void button_handler_init(void)
{
    nrfx_err_t err;
    
    /* Initialize GPIOTE if not already initialized */
    if (!nrfx_gpiote_is_init())
    {
        /* Newer nrfx uses compile-time priority; init takes no args */
        err = nrfx_gpiote_init();
        if (err != NRFX_SUCCESS)
            return;
    }
    
    /* Configure each button */
    for (uint8_t i = 0; i < BUTTON_NUM; i++)
    {
        nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
        in_config.pull = NRF_GPIO_PIN_PULLUP;
        
        err = nrfx_gpiote_in_init(button_pins[i], &in_config, gpiote_event_handler);
        if (err == NRFX_SUCCESS)
        {
            /* Enable interrupt for this pin */
            nrfx_gpiote_in_event_enable(button_pins[i], true);
        }
        
        /* Initialize button state */
        button_state[i].pressed = false;
        button_state[i].debounce_count = 0;
    }

    /* Start periodic debounce timer (runs every 5ms) */
    if (button_timer == NULL)
    {
        button_timer = xTimerCreate(
            "BtnDebounce",
            pdMS_TO_TICKS(5),
            pdTRUE,
            NULL,
            button_timer_callback
        );
        if (button_timer != NULL)
        {
            xTimerStart(button_timer, 0);
        }
    }
}

void button_handler_register_callback(button_event_callback_t callback)
{
    button_callback = callback;
}

bool button_is_pressed(button_id_e button_id)
{
    if (button_id >= BUTTON_NUM)
        return false;
    return button_state[button_id].pressed;
}

void button_handler_process(void)
{
    for (uint8_t i = 0; i < BUTTON_NUM; i++)
    {
        /* Read current pin state (buttons are active low with pullup) */
        bool pin_state = (nrf_gpio_pin_read(button_pins[i]) == 0);
        
        /* Debounce logic */
        if (pin_state != button_state[i].pressed)
        {
            button_state[i].debounce_count++;
            
            if (button_state[i].debounce_count >= DEBOUNCE_THRESHOLD)
            {
                /* State change confirmed after debounce period */
                button_state[i].pressed = pin_state;
                button_state[i].debounce_count = 0;
                
                /* Debug output */
                char debug_str[64];
                int len = snprintf(debug_str, sizeof(debug_str), 
                    "BTN[%d] %s\r\n", i, pin_state ? "PRESS" : "RELEASE");
                reporter_instance.print(debug_str, len);
                
                /* Invoke callback if registered */
                if (button_callback != NULL)
                {
                    button_callback((button_id_e)i, button_state[i].pressed);
                }
            }
        }
        else
        {
            /* State stable, reset debounce counter */
            button_state[i].debounce_count = 0;
        }
    }
}
