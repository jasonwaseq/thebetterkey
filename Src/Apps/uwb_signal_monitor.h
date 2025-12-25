/**
 * @file    uwb_signal_monitor.h
 *
 * @brief   UWB bidirectional signal monitor for debugging and verification
 *
 * @author  Development Team
 *
 */

#ifndef UWB_SIGNAL_MONITOR_H
#define UWB_SIGNAL_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Signal event types for monitoring
 */
typedef enum {
    SIGNAL_EVENT_TX_START,       /**< Transmission started */
    SIGNAL_EVENT_TX_SUCCESS,     /**< Transmission successful */
    SIGNAL_EVENT_TX_FAILED,      /**< Transmission failed */
    SIGNAL_EVENT_RX_START,       /**< Reception started */
    SIGNAL_EVENT_RX_SUCCESS,     /**< Reception successful */
    SIGNAL_EVENT_RX_TIMEOUT,     /**< Reception timeout */
    SIGNAL_EVENT_RX_ERROR,       /**< Reception error */
    SIGNAL_EVENT_PAYLOAD_RX,     /**< Payload received */
} uwb_signal_event_t;

/**
 * @brief Initialize signal monitor
 */
void uwb_signal_monitor_init(void);

/**
 * @brief Log a signal event
 */
void uwb_signal_monitor_event(bool is_controller, uint16_t remote_addr, 
                              uwb_signal_event_t event, uint32_t optional_data);

/**
 * @brief Get bidirectional status summary
 */
void uwb_signal_monitor_print_status(bool is_controller);

/**
 * @brief Reset statistics
 */
void uwb_signal_monitor_reset(void);

#endif /* UWB_SIGNAL_MONITOR_H */
