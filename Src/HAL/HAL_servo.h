/**
 * @file      HAL_servo.h
 *
 * @brief     Header for servo control via PWM
 *
 * @author    Decawave Applications
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

#ifndef HAL_SERVO_H
#define HAL_SERVO_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Servo position enumeration
 */
typedef enum {
    SERVO_POS_MIN = 1100,      // Minimum pulse width (safe left position, ~20 degrees)
    SERVO_POS_CENTER = 1500,   // Center position (90 degrees)
    SERVO_POS_MAX = 1650       // Maximum pulse width (safe right position, ~110 degrees, prevents stall)
} servo_position_e;

/**
 * @fn void HAL_servo_init(void)
 *
 * @brief Initialize servo PWM control
 *
 * Configures PWM peripheral with appropriate frequency and duty cycle
 * for standard servo control (50Hz, 1-2ms pulse width).
 *
 * @return void
 */
void HAL_servo_init(void);

/**
 * @fn void HAL_servo_set_position(uint16_t pulse_width_us)
 *
 * @brief Set servo position based on pulse width
 *
 * Sets the servo position by controlling the PWM duty cycle.
 * Standard servo pulse width range is 1000-2000 microseconds.
 * 1000us = 0 degrees, 1500us = 90 degrees, 2000us = 180 degrees
 *
 * @param pulse_width_us Pulse width in microseconds (1000-2000)
 * @return void
 */
void HAL_servo_set_position(uint16_t pulse_width_us);

/**
 * @fn void HAL_servo_move_to_position(servo_position_e position)
 *
 * @brief Move servo to predefined position
 *
 * @param position Servo position (MIN, CENTER, or MAX)
 * @return void
 */
void HAL_servo_move_to_position(servo_position_e position);

/**
 * @fn void HAL_servo_stop(void)
 *
 * @brief Stop servo (disable PWM output)
 *
 * @return void
 */
void HAL_servo_stop(void);

/**
 * @fn bool HAL_servo_is_ready(void)
 *
 * @brief Check if servo is initialized and ready
 *
 * @return true if servo is ready, false otherwise
 */
bool HAL_servo_is_ready(void);

#endif /* HAL_SERVO_H */
