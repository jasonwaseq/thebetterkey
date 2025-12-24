/**
 * @file    uwb_servo_responder.h
 *
 * @brief   UWB responder that controls servo on received signals
 *
 * @author  Decawave Applications
 *
 * @attention Copyright (c) 2021 - 2022, Qorvo US, Inc.
 * All rights reserved
 */

#ifndef UWB_SERVO_RESPONDER_H
#define UWB_SERVO_RESPONDER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @fn void uwb_servo_responder_init(void)
 *
 * @brief Initialize servo-controlled UWB responder
 *
 * Sets up servo control and registers callbacks for UWB reception
 *
 * @return void
 */
void uwb_servo_responder_init(void);

/**
 * @fn void uwb_servo_responder_signal_received(void)
 *
 * @brief Called when UWB signal is received from initiator
 *
 * Triggers servo movement to indicate reception
 *
 * @return void
 */
void uwb_servo_responder_signal_received(void);

/**
 * @fn void uwb_servo_responder_move_servo(uint16_t position_us)
 *
 * @brief Move servo to specified position
 *
 * @param position_us Servo pulse width in microseconds (1000-2000)
 * @return void
 */
void uwb_servo_responder_move_servo(uint16_t position_us);

/**
 * @fn void uwb_servo_responder_return_to_neutral(void)
 *
 * @brief Return servo to neutral/center position
 *
 * @return void
 */
void uwb_servo_responder_return_to_neutral(void);

#endif /* UWB_SERVO_RESPONDER_H */
