/**
 * @file    uwb_signal_monitor.c
 *
 * @brief   UWB bidirectional signal monitor implementation
 *
 * @author  Development Team
 *
 */

#include "uwb_signal_monitor.h"
#include "reporter.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief Signal statistics tracker
 */
typedef struct {
    uint32_t tx_packets;
    uint32_t tx_success;
    uint32_t tx_failed;
    uint32_t rx_packets;
    uint32_t rx_success;
    uint32_t rx_timeout;
    uint32_t rx_error;
    uint32_t payload_packets;
} signal_stats_t;

static signal_stats_t tx_stats = {0};
static signal_stats_t rx_stats = {0};
static bool monitor_initialized = false;

void uwb_signal_monitor_init(void)
{
    memset(&tx_stats, 0, sizeof(signal_stats_t));
    memset(&rx_stats, 0, sizeof(signal_stats_t));
    monitor_initialized = true;
    
    char msg[] = "UWB_SIGNAL_MONITOR: Initialized\r\n";
    reporter_instance.print(msg, strlen(msg));
}

void uwb_signal_monitor_event(bool is_controller, uint16_t remote_addr,
                              uwb_signal_event_t event, uint32_t optional_data)
{
    if (!monitor_initialized)
        return;

    char event_str[96];
    const char *role = is_controller ? "INIT" : "RESP";
    const char *dir_str = "???";
    
    signal_stats_t *stats = NULL;
    
    /* Update statistics based on event type */
    switch (event) {
        case SIGNAL_EVENT_TX_START:
            dir_str = "TX_START";
            stats = &tx_stats;
            stats->tx_packets++;
            break;
        case SIGNAL_EVENT_TX_SUCCESS:
            dir_str = "TX_OK";
            stats = &tx_stats;
            stats->tx_success++;
            break;
        case SIGNAL_EVENT_TX_FAILED:
            dir_str = "TX_FAIL";
            stats = &tx_stats;
            stats->tx_failed++;
            break;
        case SIGNAL_EVENT_RX_START:
            dir_str = "RX_START";
            stats = &rx_stats;
            stats->rx_packets++;
            break;
        case SIGNAL_EVENT_RX_SUCCESS:
            dir_str = "RX_OK";
            stats = &rx_stats;
            stats->rx_success++;
            break;
        case SIGNAL_EVENT_RX_TIMEOUT:
            dir_str = "RX_TIMEOUT";
            stats = &rx_stats;
            stats->rx_timeout++;
            break;
        case SIGNAL_EVENT_RX_ERROR:
            dir_str = "RX_ERROR";
            stats = &rx_stats;
            stats->rx_error++;
            break;
        case SIGNAL_EVENT_PAYLOAD_RX:
            dir_str = "PAYLOAD_RX";
            stats = &rx_stats;
            stats->payload_packets++;
            break;
    }
    
    int len = snprintf(event_str, sizeof(event_str),
                      "UWB_MON: %s [%s] 0x%04x data=0x%08lx\r\n",
                      role, dir_str, remote_addr, optional_data);
    reporter_instance.print(event_str, len);
}

void uwb_signal_monitor_print_status(bool is_controller)
{
    char status[256];
    const char *role = is_controller ? "INITIATOR" : "RESPONDER";
    
    int len = snprintf(status, sizeof(status),
        "\r\n===== UWB BIDIRECTIONAL SIGNAL STATUS =====\r\n"
        "%s ROLE STATISTICS:\r\n"
        "  TX: packets=%lu success=%lu failed=%lu\r\n"
        "  RX: packets=%lu success=%lu timeout=%lu error=%lu payloads=%lu\r\n"
        "  LINK: %s\r\n"
        "===========================================\r\n",
        role,
        (unsigned long)tx_stats.tx_packets, (unsigned long)tx_stats.tx_success, (unsigned long)tx_stats.tx_failed,
        (unsigned long)rx_stats.rx_packets, (unsigned long)rx_stats.rx_success, 
        (unsigned long)rx_stats.rx_timeout, (unsigned long)rx_stats.rx_error,
        (unsigned long)rx_stats.payload_packets,
        (rx_stats.rx_success > 0) ? "BIDIRECTIONAL OK" : "WAITING FOR SIGNALS");
    
    reporter_instance.print(status, len);
}

void uwb_signal_monitor_reset(void)
{
    memset(&tx_stats, 0, sizeof(signal_stats_t));
    memset(&rx_stats, 0, sizeof(signal_stats_t));
    char msg[] = "UWB_SIGNAL_MONITOR: Stats reset\r\n";
    reporter_instance.print(msg, strlen(msg));
}
