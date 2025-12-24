/**
 * @file    uwb_button_initiator.h
 *
 * @brief   UWB button-triggered initiator for FiRa
 *
 * @author  Decawave Applications
 *
 * @attention Copyright (c) 2021 - 2022, Qorvo US, Inc.
 * All rights reserved
 */

#ifndef UWB_BUTTON_INITIATOR_H
#define UWB_BUTTON_INITIATOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @fn void uwb_button_initiator_init(void)
 *
 * @brief Initialize button-triggered UWB initiator
 *
 * Sets up button handler and registers callbacks for UWB transmission
 *
 * @return void
 */
void uwb_button_initiator_init(void);

/**
 * @fn void uwb_button_initiator_start_ranging(void)
 *
 * @brief Start UWB ranging session (called on button press)
 *
 * Initiates ranging frame transmission to responder
 *
 * @return void
 */
void uwb_button_initiator_start_ranging(void);

/**
 * @fn void uwb_button_initiator_stop_ranging(void)
 *
 * @brief Stop UWB ranging session
 *
 * @return void
 */
void uwb_button_initiator_stop_ranging(void);

/**
 * @fn bool uwb_button_initiator_is_ranging(void)
 *
 * @brief Check if currently performing ranging
 *
 * @return true if ranging is active, false otherwise
 */
bool uwb_button_initiator_is_ranging(void);

#endif /* UWB_BUTTON_INITIATOR_H */
