/**
 * @file INTEGRATION_EXAMPLE.c
 * 
 * @brief Example code snippets for integrating button/servo control with FiRa app
 * 
 * This file shows code snippets that demonstrate how to integrate the
 * button_handler, servo controller, and UWB initiator/responder modules
 * with the existing fira_app.c
 */

/*
 * ==============================================================================
 * INITIATOR BOARD INTEGRATION
 * ==============================================================================
 * 
 * Add this to the top of fira_app.c:
 */

#include "button_handler.h"
#include "uwb_button_initiator.h"

/* Global variables for button-driven ranging */
static bool button_driven_ranging_active = false;

/*
 * Modify fira_app_process_init() to include button initialization:
 */

static error_e fira_app_process_init_with_buttons(bool controller, void const *arg)
{
    fira_param_t *fira_param = (fira_param_t *)arg;
    
    /* ... existing fira_app_process_init code ... */
    
    /* NEW: Initialize button-driven UWB control */
    if (controller)  /* Only on initiator */
    {
        uwb_button_initiator_init();
        deca_dbg_msg("Button-driven UWB Initiator initialized\r\n");
    }
    
    return _NO_ERR;
}

/*
 * Modify fira_app_process_start() to handle button-driven ranging:
 */

static void fira_app_process_start_with_buttons(void)
{
    /* For initiator with button control, don't auto-start */
    if (current_board_is_initiator())
    {
        /* Wait for button press to start ranging */
        button_driven_ranging_active = true;
        deca_dbg_msg("Waiting for button press to start ranging...\r\n");
    }
    else
    {
        /* Original start logic for responder */
        int r = uwbmac_start(uwbmac_ctx);
        assert(r == UWBMAC_SUCCESS);
    }
}

/*
 * Update uwb_button_initiator_start_ranging() to integrate with FiRa:
 */

void uwb_button_initiator_start_ranging_integrated(void)
{
    if (!is_ranging && button_driven_ranging_active)
    {
        is_ranging = true;
        
        /* Start UWB MAC layer */
        int r = uwbmac_start(uwbmac_ctx);
        if (r == UWBMAC_SUCCESS)
        {
            /* Start FiRa session */
            r = fira_helper_start_session(&fira_ctx, session_id);
            if (r == UWBMAC_SUCCESS)
            {
                deca_dbg_msg("UWB Ranging started via button\r\n");
            }
        }
    }
}

/*
 * Update uwb_button_initiator_stop_ranging() to integrate with FiRa:
 */

void uwb_button_initiator_stop_ranging_integrated(void)
{
    if (is_ranging && button_driven_ranging_active)
    {
        is_ranging = false;
        
        /* Stop FiRa session */
        int r = fira_helper_stop_session(&fira_ctx, session_id);
        if (r == UWBMAC_SUCCESS)
        {
            /* Stop UWB MAC */
            uwbmac_stop(uwbmac_ctx);
            deca_dbg_msg("UWB Ranging stopped via button\r\n");
        }
    }
}

/*
 * Add a periodic task to process buttons (every 5-10ms):
 */

void button_processing_task(void const *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  /* 10ms period */
    
    while (1)
    {
        /* Process button debouncing */
        button_handler_process();
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/*
 * ==============================================================================
 * RESPONDER BOARD INTEGRATION
 * ==============================================================================
 * 
 * Add this to the top of fira_app.c:
 */

#include "uwb_servo_responder.h"

/*
 * Modify fira_app_process_init() to include servo initialization:
 */

static error_e fira_app_process_init_responder_with_servo(bool controller, void const *arg)
{
    fira_param_t *fira_param = (fira_param_t *)arg;
    
    /* ... existing fira_app_process_init code ... */
    
    /* NEW: Initialize servo-controlled responder */
    if (!controller)  /* Only on responder */
    {
        uwb_servo_responder_init();
        deca_dbg_msg("Servo-controlled UWB Responder initialized\r\n");
    }
    
    return _NO_ERR;
}

/*
 * Modify fira_app_process_start() for responder:
 */

static void fira_app_process_start_responder(void)
{
    /* Original start logic */
    int r = uwbmac_start(uwbmac_ctx);
    assert(r == UWBMAC_SUCCESS);
    
    /* Start FiRa session on responder */
    r = fira_helper_start_session(&fira_ctx, session_id);
    assert(r == UWBMAC_SUCCESS);
    
    started = true;
    deca_dbg_msg("UWB Responder listening...\r\n");
}

/*
 * Integrate servo trigger into report_cb():
 * 
 * Find the existing report_cb() function and add servo control:
 */

static void report_cb_with_servo(const struct ranging_results *results, void *user_data)
{
    /* Check if we have valid measurements (signal received) */
    if (results->stopped_reason == 0xFF && results->n_measurements > 0)
    {
        /* NEW: Trigger servo movement on successful reception */
        uwb_servo_responder_signal_received();
        
        deca_dbg_msg("Signal received - Servo moving\r\n");
    }
    
    /* ... existing report_cb code for logging results ... */
}

/*
 * ==============================================================================
 * HELPER FUNCTIONS
 * ==============================================================================
 */

/* Determine if this board is initiator or responder */
bool current_board_is_initiator(void)
{
    /* This depends on your board identification logic */
    /* Could be based on:
     * - EEPROM configuration
     * - GPIO pin state at startup
     * - Serial number
     * - Fixed compile-time constant
     */
    extern bool is_initiator_mode;
    return is_initiator_mode;
}

/*
 * ==============================================================================
 * EXAMPLE MAIN APPLICATION INITIALIZATION
 * ==============================================================================
 */

void setup_button_servo_uwb_system(void)
{
    /* Initialize FiRa base system */
    fira_app_process_init(
        current_board_is_initiator(),
        get_fira_config()
    );
    
    /* Start the system */
    if (current_board_is_initiator())
    {
        deca_dbg_msg("=== INITIATOR BOARD ===\r\n");
        deca_dbg_msg("Press button to trigger UWB ranging\r\n");
        fira_app_process_start_with_buttons();
    }
    else
    {
        deca_dbg_msg("=== RESPONDER BOARD ===\r\n");
        deca_dbg_msg("Waiting for UWB signal...\r\n");
        fira_app_process_start_responder();
    }
    
    /* Create button processing task */
    xTaskCreate(
        button_processing_task,
        "ButtonTask",
        256,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
}

/*
 * ==============================================================================
 * SERVO CONTROL EXAMPLES
 * ==============================================================================
 */

/* Move servo through full range (demonstration) */
void demo_servo_sweep(void)
{
    uint16_t positions[] = {
        SERVO_POS_MIN,      /* 0 degrees */
        SERVO_POS_CENTER,   /* 90 degrees */
        SERVO_POS_MAX       /* 180 degrees */
    };
    
    for (int i = 0; i < 3; i++)
    {
        uwb_servo_responder_move_servo(positions[i]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Custom servo position based on signal strength */
void servo_follow_signal_strength(uint8_t signal_strength_db)
{
    /* Map signal strength (-100 to -40 dB) to servo position */
    /* Weaker signal (lower value) = min position */
    /* Stronger signal (higher value) = max position */
    
    uint16_t pulse_width = SERVO_POS_MIN +
        ((signal_strength_db + 100) * (SERVO_POS_MAX - SERVO_POS_MIN)) / 60;
    
    uwb_servo_responder_move_servo(pulse_width);
}

/*
 * ==============================================================================
 * COMPILE-TIME BOARD SELECTION
 * ==============================================================================
 * 
 * Add to your build system or header file:
 */

#ifndef BOARD_MODE
#define BOARD_MODE BOARD_INITIATOR  /* Change to BOARD_RESPONDER for other board */
#endif

#define BOARD_INITIATOR 0
#define BOARD_RESPONDER 1

/* Use in code: */
#if BOARD_MODE == BOARD_INITIATOR
    void main_board_init(void) { setup_button_servo_uwb_system(); }
#else
    void main_board_init(void) { setup_button_servo_uwb_system(); }
#endif

/*
 * ==============================================================================
 * END OF INTEGRATION EXAMPLES
 * ==============================================================================
 */
