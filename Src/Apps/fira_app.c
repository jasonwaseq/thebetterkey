/**
 *  @file     fira_app.c
 *
 *  @brief    Fira processes control
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

#include <stdint.h>
#include <inttypes.h>
#include "reporter.h"
#include "deca_error.h"
#include "app.h"
#include "uwbmac.h"
#include "fira_helper.h"
#include "appConfig.h"
#include "default_config.h"
#include "rf_tuning_config.h"
#include "common_fira.h"
#include "cmd_fn.h"
#include "int_priority.h"
#include "task_signal.h"
#include "usb_uart_tx.h"
#include "dw3000_mcps_mcu.h"
#include "HAL_error.h"
#include "HAL_uwb.h"
#include "cmd.h"

#include "fira_app.h"
#include "dw3000_pdoa.h"
#include "create_fira_app_task.h"
#include "uwb_button_initiator.h"
#include "uwb_servo_responder.h"
#include "uwb_signal_monitor.h"

extern void pdoaupdate_lut(void);

#define DATA_TASK_STACK_SIZE_BYTES 1400

/* 0 - no output of PDoA
 * 1 - output of PDoA from uwb_stack, this is supported in 10.x.x
 */
#if CONFIG_PEG_UWB == 1
#define OUTPUT_PDOA_ENABLE (1)
#else
#define OUTPUT_PDOA_ENABLE (1)
#endif

static struct uwbmac_context *uwbmac_ctx = NULL;

/* uwb_stack version >8.x.x allows usage of SP1 RFRAMES on Deferred DS-TWR to transmit IoT data.
 * This mode is not specified in FiRa MAC 1.2, i.e. proprietary implementation.
 * In this mode both Initiator and Responder can form RFRAMES with proprietary PIE.
 * Maximum IoT data excahnge limited to approx 70 bytes bidirectional per TWR.
 * uwb_stack encrypts and guarantee the integrity of the packet, however the flow control, and
 * retransmission shall be implemented on the upper layer.
 */
#define PROPRIETARY_SP1_TWR_EXAMPLE_ENABLE (1)  /* Enabled to support SP1 payloads */


#if (PROPRIETARY_SP1_TWR_EXAMPLE_ENABLE == 1)
/* Below is the example data set to be used for SP1 proprietary frames example */
static struct data_parameters data_params = {
    .data_payload = {0},
    .data_payload_len = 0,
};
#endif

#define STR_SIZE (256)

uint32_t session_id = 42;  /* Made global for button initiator access */
static task_signal_t dataTransferTask;
static bool started = false;
static bool is_controller = false;  /* Track if this board is controller/initiator */
static void report_cb(const struct ranging_results *results, void *user_data);
static struct string_measurement output_result;
struct fira_context fira_ctx;  /* Made global for button initiator access */


/* fira_app_process_init
 */
static error_e fira_app_process_init(bool controller, void const *arg)
{
    fira_param_t *fira_param = (fira_param_t *)arg;

    session_id = fira_param->session_id;
    is_controller = controller;  /* Save for later use */
    
    char role_log[128];
    int role_len = snprintf(role_log, sizeof(role_log), 
                       "FiRA_APP_INIT: controller=%d (0=responder, 1=controller)\r\n", controller);
    reporter_instance.print(role_log, role_len);
    
    /* Log RF configuration at startup */
    char rf_log[256];
    int rlen = snprintf(rf_log, sizeof(rf_log),
        "%s: RF Init chan=%u preamble=%u sfd=%u rframe=%u slot=%u ms=%u\r\n",
        controller ? "INIT" : "RESP",
        fira_param->session.channel_number,
        fira_param->session.preamble_code_index,
        fira_param->session.sfd_id,
        fira_param->session.rframe_config,
        fira_param->session.slot_duration_rstu,
        fira_param->session.block_duration_ms);
    reporter_instance.print(rf_log, rlen);
    
    char addr_log[128];
    int alen = snprintf(addr_log, sizeof(addr_log),
        "%s: Addr short=0x%04x dest=0x%04x device_type=%u\r\n",
        controller ? "INIT" : "RESP",
        fira_param->session.short_addr,
        fira_param->session.destination_short_address,
        fira_param->session.device_type);
    reporter_instance.print(addr_log, alen);
    
    /* Initialize button initiator on controller (initiator) side */
    if (controller)
    {
        uwb_button_initiator_init();
    }
    /* Initialize servo responder on responder side */
    else
    {
        uwb_servo_responder_init();
    }
    
    /* Initialize signal monitoring */
    uwb_signal_monitor_init();
    
    uint16_t string_len = STR_SIZE * (controller ? fira_param->controlees_params.n_controlees : 1);

    output_result.str = malloc(string_len);
    if (!(output_result.str))
    {
        char *err = "not enough memory";
        reporter_instance.print((char *)err, strlen(err));
        return _ERR_Cannot_Alloc_Memory;
    }
    output_result.len = string_len;

    // Update LUT for the current antenna set
    pdoaupdate_lut();

    fira_uwb_mcps_init(fira_param);

    int r = uwbmac_init(&uwbmac_ctx);
    assert(r == UWBMAC_SUCCESS);

    // unset promiscuous to accept only filtered frames.
    uwbmac_set_promiscuous_mode(uwbmac_ctx, true);
    // set local short address.
    uwbmac_set_short_addr(uwbmac_ctx, fira_param->session.short_addr);
    // register report cb
    r = fira_helper_open(&fira_ctx, uwbmac_ctx, &report_cb, "endless", 0, &output_result);
    assert(r == UWBMAC_SUCCESS);
    // Set fira scheduler;
    r = fira_helper_set_scheduler(&fira_ctx);
    assert(r == UWBMAC_SUCCESS);
    // init session;
    r = fira_helper_init_session(&fira_ctx, session_id);
    assert(r == UWBMAC_SUCCESS);
    // Set session parameters;
    r = fira_set_session_parameters(&fira_ctx, session_id, &fira_param->session);
    assert(r == UWBMAC_SUCCESS);

        // If SP1 is selected, allow proprietary data frames for app use.
        // Example payloads are sent only when explicitly enabled.
        if (fira_param->session.rframe_config == FIRA_RFRAME_CONFIG_SP1)
        {
    #if (PROPRIETARY_SP1_TWR_EXAMPLE_ENABLE == 1)
        r = fira_helper_send_data(&fira_ctx, session_id, &data_params);
        assert(r == UWBMAC_SUCCESS);
    #else
        // SP1 enabled: no example data sent. Application modules (e.g., button initiator)
        // may send SP1 payloads as needed.
    #endif
        }
    if (controller)
    {
        // Add controlee session parameters;
        char clee_log[128];
        int clen = snprintf(clee_log, sizeof(clee_log),
            "INIT: Adding %d controlee(s) to session\r\n",
            fira_param->controlees_params.n_controlees);
        reporter_instance.print(clee_log, clen);
        
        r = fira_helper_add_controlees(&fira_ctx, session_id, &fira_param->controlees_params);
        assert(r == UWBMAC_SUCCESS);
    }
    return _NO_ERR;
}

static void fira_app_process_start(void)
{
    char str_start[] = ">>> FiRa process starting <<<\r\n";
    reporter_instance.print(str_start, strlen(str_start));
    
    /* OK, let's start. */
    int r = uwbmac_start(uwbmac_ctx);
    assert(r == UWBMAC_SUCCESS);
    
    char str_mac[] = ">>> uwbmac_start OK <<<\r\n";
    reporter_instance.print(str_mac, strlen(str_mac));
    
    // Start session;
    r = fira_helper_start_session(&fira_ctx, session_id);
    assert(r == UWBMAC_SUCCESS);
    
    char str_sess[] = ">>> fira_helper_start_session OK <<<\r\n";
    reporter_instance.print(str_sess, strlen(str_sess));
    
    started = true;
}

static error_e fira_app_process_terminate(void)
{
    if (started)
    {
        started = false; // do not allow re-entrance

        // Stop session;
        int r = fira_helper_stop_session(&fira_ctx, session_id);
        assert(!r);
        // Stop.
        uwbmac_stop(uwbmac_ctx);
        // Uninit session;
        r = fira_helper_deinit_session(&fira_ctx, session_id);
        assert(!r);
        fira_helper_close(&fira_ctx);

        // unregister driver;
        fira_uwb_mcps_deinit();

        free(output_result.str);
    }
    return _NO_ERR;
}

static float convert_aoa_2pi_q16_to_deg(int16_t aoa_2pi_q16)
{
    return (360.0 * aoa_2pi_q16 / (1 << 16));
}

static void report_cb(const struct ranging_results *results, void *user_data)
{
    fira_param_t *fira_param_local = get_fira_config();
    bool is_responder = (fira_param_local->session.device_type == FIRA_DEVICE_TYPE_CONTROLEE);
    
    /* Always log session events */
    if (results->stopped_reason != 0xFF)
    {
        char stop_log[96];
        int slen = snprintf(stop_log, sizeof(stop_log), "%s: Session stopped (reason=%u)\r\n",
                           is_responder ? "RESP" : "INIT", results->stopped_reason);
        reporter_instance.print(stop_log, slen);
        return;
    }
    
    /* Log all measurements with ultra-verbose status plus diag snapshot */
    float diag_rssi = 0.0f;
    int diag_nlos = 0;
    if (fira_uwb_is_diag_enabled())
    {
        fira_uwb_get_diag(&diag_rssi, &diag_nlos);
    }

    char block_log[196];
    int blog = snprintf(block_log, sizeof(block_log), 
                       "%s: Block %u measurements=%d stopped_reason=%u diag_rssi=%.1f diag_nlos=%d%%\r\n",
                       is_responder ? "RESP" : "INIT", results->block_index, 
                       results->n_measurements, results->stopped_reason,
                       diag_rssi, diag_nlos);
    reporter_instance.print(block_log, blog);
    
    /* Check for button payload from initiator */
    if (results->n_measurements > 0)
    {
        uint16_t expected_peer = fira_param_local->session.destination_short_address;

        for (int i = 0; i < results->n_measurements; i++)
        {
            struct ranging_measurements *rm_local = (struct ranging_measurements *)(&results->measurements[i]);

            /* Gate by peer short address to avoid stray devices */
            if (rm_local->short_addr != expected_peer)
            {
                char skip_log[96];
                int slog = snprintf(skip_log, sizeof(skip_log), 
                                   "%s: [%d] SKIP addr 0x%04x (expected 0x%04x)\r\n",
                                   is_responder ? "RESP" : "INIT", i, rm_local->short_addr, expected_peer);
                reporter_instance.print(skip_log, slog);
                continue;
            }

            /* Detailed measurement status with human-readable error codes */
            const char *status_str;
            switch (rm_local->status) {
                case 0:  status_str = "OK"; break;
                case 1:  status_str = "TX_FAIL"; break;
                case 2:  status_str = "RX_TIMEOUT"; break;
                case 3:  status_str = "RX_PHY_DEC"; break;
                case 4:  status_str = "RX_TOA"; break;
                case 5:  status_str = "RX_STS"; break;
                case 6:  status_str = "RX_MAC_DEC"; break;
                case 7:  status_str = "RX_MAC_IE_DEC"; break;
                case 8:  status_str = "RX_MAC_IE_MISS"; break;
                default: status_str = "UNKNOWN"; break;
            }
            
            char meas_log[224];
            int mlog = snprintf(meas_log, sizeof(meas_log), 
                               "%s: [%d] 0x%04x status=%u(%s) payload_len=%u dist=%ld slot=%u "
                               "nlos=%d los=%d rssi=%u fom=%u diag_rssi=%.1f diag_nlos=%d%%\r\n",
                               is_responder ? "RESP" : "INIT", i,
                               rm_local->short_addr, rm_local->status, status_str,
                               rm_local->sp1_data_len, (long)rm_local->distance_mm,
                               rm_local->slot_index, rm_local->nlos, rm_local->los,
                               rm_local->rssi, rm_local->remote_aoa_azimuth_fom,
                               diag_rssi, diag_nlos);
            reporter_instance.print(meas_log, mlog);
            // Deep debug: always print sp1_data_len and first 8 bytes of sp1_data
            char debug_log[128];
            int dlog = snprintf(debug_log, sizeof(debug_log),
                "[DEBUG] RX: sp1_data_len=%u, sp1_data=[%02X %02X %02X %02X %02X %02X %02X %02X] (first 8 bytes)\r\n",
                rm_local->sp1_data_len,
                rm_local->sp1_data[0], rm_local->sp1_data[1], rm_local->sp1_data[2], rm_local->sp1_data[3],
                rm_local->sp1_data[4], rm_local->sp1_data[5], rm_local->sp1_data[6], rm_local->sp1_data[7]);
            reporter_instance.print(debug_log, dlog);
            
            /* Log successful RX to signal monitor */
            if (rm_local->status == 0)
            {
                uwb_signal_monitor_event(is_controller, rm_local->short_addr, 
                                        SIGNAL_EVENT_RX_SUCCESS, rm_local->distance_mm);
            }
            else
            {
                uwb_signal_monitor_event(is_controller, rm_local->short_addr,
                                        SIGNAL_EVENT_RX_ERROR, rm_local->status);
            }

            /* On responder: check for BTN payload marker even if status is non-zero */
            if (is_responder && rm_local->sp1_data_len >= 4)
            {
                const uint8_t *data = (const uint8_t *)(rm_local->sp1_data);
                char payload_log[96];
                int plog = snprintf(payload_log, sizeof(payload_log), 
                                   "RESP: SP1 data: [0x%02x 0x%02x 0x%02x 0x%02x]\r\n",
                                   data[0], data[1], data[2], data[3]);
                reporter_instance.print(payload_log, plog);
                
                /* Signal payload reception */
                uwb_signal_monitor_event(is_controller, rm_local->short_addr, 
                                        SIGNAL_EVENT_PAYLOAD_RX, 
                                        (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
                
                if (data[0] == 'B' && data[1] == 'T' && data[2] == 'N')
                {
                    uint8_t btn_counter = data[3];
                    char logbuf[96];
                    int l = snprintf(logbuf, sizeof(logbuf), 
                                    "RESP: *** BTN MATCH *** counter=%u SERVO TRIGGER\r\n", 
                                    (unsigned)btn_counter);
                    reporter_instance.print(logbuf, l);
                    
                    /* Trigger servo with button counter */
                    uwb_servo_responder_signal_received(btn_counter);
                    break;
                }
            }
        }
    }
    int len = 0;
    uint32_t seq = 0;
    struct string_measurement *str_result = (struct string_measurement *)user_data;
    struct ranging_measurements *rm;
    fira_param_t *fira_param = get_fira_config();

    if (results->stopped_reason != 0xFF)
    {
        len = sprintf(str_result->str, "{\"Session Stopped\":\"%s\"}\r\n",
                      (results->stopped_reason == 0x0) ? "Stop request" :
                      (results->stopped_reason == 0x1) ? "Inband Stop" :
                      (results->stopped_reason == 0x2) ? "Max attempts" : "Unknown");

        reporter_instance.print(str_result->str, len);
        return;
    }

    len = sprintf(str_result->str, "{\"Block\":%" PRIu32 ", \"results\":[", results->block_index);

    for (int i = 0; i < results->n_measurements; i++)
    {
        if (i > 0)
        {
            len += snprintf(&str_result->str[len], str_result->len - len, ",");
        }

        rm = (struct ranging_measurements *)(&results->measurements[i]);

        len += snprintf(&str_result->str[len], str_result->len - len,
                        "{\"Addr\":\"0x%04x\",\"Status\":\"%s\"",
                        rm->short_addr, (rm->status) ? ("Err") : ("Ok"));

        if (rm->status == 0)
        {
            len += snprintf(&str_result->str[len], str_result->len - len, ",\"D_cm\":%d",
                            (int)(rm->distance_mm / 10));

#if (OUTPUT_PDOA_ENABLE == 1)
            len += snprintf(&str_result->str[len], str_result->len - len,
                            ",\"LPDoA_deg\":%0.2f,\"LAoA_deg\":%0.2f,\"LFoM\":%d,\"RAoA_deg\":%0.2f",
                            convert_aoa_2pi_q16_to_deg(rm->local_aoa_measurements[0].pdoa_2pi),
                            convert_aoa_2pi_q16_to_deg(rm->local_aoa_measurements[0].aoa_2pi),
                            rm->local_aoa_measurements[0].aoa_fom,
                            convert_aoa_2pi_q16_to_deg(rm->remote_aoa_azimuth_2pi));
#endif

            len += snprintf(&str_result->str[len], str_result->len - len, ",\"CFO_100ppm\":%d", (int)fira_uwb_mcps_get_cfo_ppm());

#if (PROPRIETARY_SP1_TWR_EXAMPLE_ENABLE == 1)
            if (fira_param->session.rframe_config == FIRA_RFRAME_CONFIG_SP1)
            {
                if (rm->payload_seq_sent > seq)
                {
                    if (osSignalSet(dataTransferTask.Handle, DATA_TRANSFER) == 0x80000000)
                    {
                        error_handler(1, _ERR_Signal_Bad);
                    }

                    seq = rm->payload_seq_sent;

                    len += snprintf(&str_result->str[len], str_result->len - len, ",\"SEQ\":%" PRIu32 "", seq);

                    if (rm->sp1_data_len > 0)
                    {
                        uint8_t *data = (uint8_t *)(rm->sp1_data);
                        len += snprintf(&str_result->str[len], str_result->len - len,
                                        ",\"DATA\":\"%02X:%02X:%02X\"", data[0], data[1], data[2]); // <- Printing of received data from another device
                    }
                }
            }
#endif
        }
        len += snprintf(&str_result->str[len], str_result->len - len, "}");
    }

    len += snprintf(&str_result->str[len], str_result->len - len, "]");

    /* Display RSSI, CFO and NLOS */
    if (fira_uwb_is_diag_enabled())
    {
        len = fira_uwb_add_diag(str_result->str, len, str_result->len);
    }

    len += snprintf(&str_result->str[len], str_result->len - len, "}\r\n");
    reporter_instance.print((char *)str_result->str, len);
}

#if (PROPRIETARY_SP1_TWR_EXAMPLE_ENABLE == 1)
/* @brief DW3000 RX : RTOS implementation
 *
 * */
static void data_task(void const *arg)
{
    while (dataTransferTask.Exit == 0)
    {
        osEvent evt = osSignalWait(dataTransferTask.SignalMask, osWaitForever);
        if (evt.value.signals & STOP_TASK)
        {
            break;
        }
        fira_helper_send_data(&fira_ctx, session_id, &data_params);
    };
    dataTransferTask.Exit = 2;
    while (dataTransferTask.Exit == 2)
    {
        osDelay(1);
    };
}
#endif

/* @brief Setup TWR tasks and timers for discovery phase.
 *          - twr polling task
 *         - rx task
 * Only setup, do not start.
 * */
static void fira_setup_tasks(fira_param_t *fira_param)
{
#if (PROPRIETARY_SP1_TWR_EXAMPLE_ENABLE == 1)
    if (fira_param->session.rframe_config == FIRA_RFRAME_CONFIG_SP1)
    {
        dataTransferTask.Exit = 0;
        dataTransferTask.Signal = DATA_TRANSFER;
        dataTransferTask.task_stack = NULL;

        error_e ret = create_fira_app_task(data_task, &dataTransferTask, (uint16_t)DATA_TASK_STACK_SIZE_BYTES, fira_param);

        if (ret != _NO_ERR)
        {
            error_handler(1, _ERR_Create_Task_Bad);
        }
    }
    else
#endif
    {
        dataTransferTask.Handle = 0;
    }
}

//-----------------------------------------------------------------------------


/* @fn      fira_app
 * @brief   this is a service function which starts the FiRa TWR
 *          top-level  application.
 * */
static void fira_app(bool controller, void *arg)
{
    error_e tmp;

    enter_critical_section(); /**< When the app will setup RTOS tasks, then if task has a higher priority,
                             the kernel will start it immediately, thus we need to stop the scheduler.*/

    set_local_pavrg_size();

    tmp = fira_app_process_init(controller, arg);

    if (tmp != _NO_ERR)
    {
        error_handler(1, tmp);
    }

    fira_setup_tasks(arg); /**< "RTOS-based" : setup all RTOS tasks for TagN application. */

    /* Always start UWB MAC (required for both initiator and responder) */
    int r = uwbmac_start(uwbmac_ctx);
    assert(r == UWBMAC_SUCCESS);
    
    char startup_log[128];
    int slen = snprintf(startup_log, sizeof(startup_log), 
                       "%s: UWB MAC started, starting FiRa session...\r\n",
                       is_controller ? "INIT" : "RESP");
    reporter_instance.print(startup_log, slen);
    
    /* Both initiator and responder start sessions immediately for proper sync
     * Button controls application behavior (servo), not session lifecycle
     */
    r = fira_helper_start_session(&fira_ctx, session_id);
    assert(r == UWBMAC_SUCCESS);
    started = true;
    
    char session_log[128];
    int slen2 = snprintf(session_log, sizeof(session_log), 
                        "%s: FiRa session %u started!\r\n",
                        is_controller ? "INIT" : "RESP", session_id);
    reporter_instance.print(session_log, slen2);

    leave_critical_section(); /**< all RTOS tasks can be scheduled */
}
// Public Methods

/* @brief
 *      Kill all task and timers related to FiRa
 *      DW3000's RX and IRQ shall be switched off before task termination,
 *      that IRQ will not produce unexpected Signal
 * */
void fira_terminate(void)
{
    fira_app_process_terminate();
    terminate_task(&dataTransferTask);

    uwbmac_exit(uwbmac_ctx);

    hal_uwb.sleep_enter();
}

void fira_helper_controller(const void *arg_fira_param)
{
    void *fira_param = (arg_fira_param) ? (void *)arg_fira_param : (void *)get_fira_config();
    fira_app(true, fira_param);
}

/* @brief start FiRa Controller+Responder
 * @parm arg is a pointer to fira_param_t or NULL if use global config
 */
void fira_helper_controlee(const void *arg_fira_param)
{
    void *fira_param = (arg_fira_param) ? (void *)arg_fira_param : (void *)get_fira_config();
    fira_app(false, fira_param);
}

const app_definition_t helpers_app_fira[] __attribute__((
    section(".known_apps"))) = {
        {"INITF", mAPP | APP_SAVEABLE, fira_helper_controller, fira_terminate, waitForCommand, command_parser, NULL},
        {"RESPF", mAPP | APP_SAVEABLE, fira_helper_controlee, fira_terminate, waitForCommand, command_parser, NULL}
    };
