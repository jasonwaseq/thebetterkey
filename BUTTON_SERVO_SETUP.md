/**
 * @file BUTTON_SERVO_SETUP.md
 * 
 * @brief Setup guide for button-controlled UWB with servo
 * 
 * This document describes how to set up and use the button-triggered UWB
 * communication system with servo control on the DWM3001CDK board.
 */

# Button-Controlled UWB System with Servo

## Overview

This implementation creates a two-board UWB system:
- **Initiator Board**: Button presses trigger UWB signal transmission
- **Responder Board**: Receives UWB signals and controls a servo motor

## Hardware Setup

### Initiator Board
- **SW1 Button** (GPIO 2): Trigger UWB ranging
- **SW2 Button** (GPIO 20): Alternative trigger button
- Connection to responder via UWB radio (DW3000)

### Responder Board
- **Servo Motor Connection**:
  - **Servo Signal** → GPIO P0.11 (PWM0 Channel 0)
  - **Servo Power (5V)** → 5V power rail
  - **Servo Ground** → GND

## Software Architecture

### New Modules

#### 1. Button Handler (`button_handler.h/c`)
- Manages SW1 and SW2 button inputs
- Debouncing logic (10ms threshold)
- Event-driven callback system
- GPIOTE interrupt support

**Key Functions:**
- `button_handler_init()` - Initialize button inputs
- `button_handler_register_callback()` - Register event handler
- `button_is_pressed()` - Query button state
- `button_handler_process()` - Debounce processing (call every ~5ms)

#### 2. Servo PWM Controller (`HAL_servo.h/c`)
- PWM-based servo control
- 50Hz servo frequency (20ms period)
- Pulse width range: 1000-2000 microseconds
  - 1000us = 0 degrees (minimum)
  - 1500us = 90 degrees (center)
  - 2000us = 180 degrees (maximum)

**Key Functions:**
- `HAL_servo_init()` - Initialize servo PWM
- `HAL_servo_set_position(uint16_t pulse_width_us)` - Set servo angle
- `HAL_servo_move_to_position(servo_position_e position)` - Move to preset
- `HAL_servo_stop()` - Disable servo
- `HAL_servo_is_ready()` - Check servo status

#### 3. UWB Initiator (`uwb_button_initiator.h/c`)
- Handles button events on initiator board
- Triggers UWB ranging on button press
- Integrates with FiRa MAC layer

**Key Functions:**
- `uwb_button_initiator_init()` - Set up button-triggered UWB
- `uwb_button_initiator_start_ranging()` - Begin UWB transmission
- `uwb_button_initiator_stop_ranging()` - End transmission
- `uwb_button_initiator_is_ranging()` - Query status

#### 4. UWB Responder with Servo (`uwb_servo_responder.h/c`)
- Detects incoming UWB signals
- Controls servo movement on reception
- Auto-return to neutral position after 2 seconds

**Key Functions:**
- `uwb_servo_responder_init()` - Initialize responder
- `uwb_servo_responder_signal_received()` - Called on UWB reception
- `uwb_servo_responder_move_servo(uint16_t position_us)` - Manual servo control
- `uwb_servo_responder_return_to_neutral()` - Reset servo position

## Integration Steps

### Step 1: Add Includes to Your App
```c
#include "button_handler.h"
#include "HAL_servo.h"
#include "uwb_button_initiator.h"
#include "uwb_servo_responder.h"
```

### Step 2: For Initiator Board
Initialize in your main app startup:
```c
// In fira_app.c or your main application
void initialize_initiator(void)
{
    // Initialize button-driven UWB
    uwb_button_initiator_init();
    
    // Note: Integrate with FiRa session management as needed
    // Modify uwb_button_initiator_start_ranging() to call fira_helper_start_session()
    // and uwb_button_initiator_stop_ranging() to call fira_helper_stop_session()
}
```

### Step 3: For Responder Board
Initialize in your main app startup:
```c
// In fira_app.c or your main application
void initialize_responder(void)
{
    // Initialize servo-controlled responder
    uwb_servo_responder_init();
    
    // Register callback to detect received frames
    // In report_cb() or your ranging callback:
    // uwb_servo_responder_signal_received();
}
```

### Step 4: Call Button Handler Process Loop
Add to your periodic task (every 5-10ms):
```c
// In FreeRTOS task or timer callback
void button_task(void)
{
    // Call periodically for debounce processing
    button_handler_process();
}
```

## GPIO Pin Assignments

| Function | GPIO | Pin | Notes |
|----------|------|-----|-------|
| SW1 Button | P0.2 | 2 | Active low with pullup |
| SW2 Button | P0.20 | 20 | Active low with pullup |
| Servo PWM | P0.11 | 11 | PWM0 Channel 0 |
| LED 1 | P0.4 | 4 | Status indicator |
| LED 2 | P0.5 | 5 | Status indicator |

## PWM Configuration

- **Frequency**: 50Hz (standard servo frequency)
- **Period**: 20ms
- **Base Clock**: 125kHz
- **Top Value**: 2500 (125kHz / 2500 = 50Hz)
- **Resolution**: ~4us per count

## Debounce Configuration

- **Threshold**: 10ms
- **Process Interval**: 5-10ms recommended

## FiRa Integration Notes

The button initiator and servo responder modules are designed to be integrated with the existing FiRa application. Some key integration points:

### Initiator Integration
```c
// In uwb_button_initiator_start_ranging()
// Replace the TODO with:
// error_e err = fira_helper_start_session(&fira_ctx, session_id);

// In uwb_button_initiator_stop_ranging()
// Replace the TODO with:
// error_e err = fira_helper_stop_session(&fira_ctx, session_id);
```

### Responder Integration
```c
// In your report_cb() function, add:
if (results->stopped_reason == 0xFF && results->n_measurements > 0)
{
    // Valid measurement received - move servo
    uwb_servo_responder_signal_received();
}
```

## Testing

### Test Procedure
1. **Flash initiator code to one board**
2. **Flash responder code to other board**
3. **Power both boards**
4. **Press SW1 on initiator board**
   - Initiator should start UWB ranging
   - Responder servo should move to maximum position
   - Servo auto-returns to center after 2 seconds
5. **Test SW2 button** (alternative trigger)
6. **Verify servo movement range** (1000-2000us pulse)

### Expected Behavior
- Button press → UWB transmission begins
- UWB reception → Servo moves to 180 degrees (2000us)
- After 2 seconds → Servo returns to 90 degrees (1500us)
- Button release → UWB transmission stops

## Configuration Files Modified

1. **custom_board.h**
   - Added BUTTON_2 definition
   - Added SERVO_PWM_PIN definition

2. **sdk_config.h**
   - Enabled NRFX_PWM_ENABLED
   - Enabled NRFX_PWM0_ENABLED
   - Enabled NRFX_GPIOTE_ENABLED
   - Increased NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS to 2

3. **DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.emProject**
   - Added HAL_servo.c
   - Added button_handler.c
   - Added uwb_button_initiator.c
   - Added uwb_servo_responder.c

## Debugging

Enable debug output to monitor system operation:

```c
// In deca_dbg.h or your debug configuration
#define DEBUG_BUTTONS 1
#define DEBUG_SERVO 1
#define DEBUG_UWB 1
```

Debug messages are sent to the UART/USB interface showing:
- Button press/release events
- Servo position changes
- UWB ranging start/stop
- Signal reception events

## Power Considerations

- **Button interrupts**: Minimal power (uses GPIOTE)
- **PWM servo**: ~10-20mA depending on servo motor
- **Ensure adequate power supply** for servo motor (2-3A peak current recommended)

## Troubleshooting

### Servo not moving
- Check PWM pin connection (P0.11)
- Verify servo power and ground
- Check HAL_servo_init() is called
- Verify PWM is enabled in sdk_config.h

### Buttons not responding
- Check GPIO pins (P0.2, P0.20)
- Verify GPIOTE is enabled
- Check button_handler_process() is called periodically
- Verify button pins have proper pullup

### UWB not responding to button
- Ensure initiator FiRa session is started
- Verify uwb_button_initiator_start_ranging() integration with FiRa
- Check ranging session parameters
- Verify responder is listening on correct channel

## Future Enhancements

- Integrate button state directly into FiRa ranging trigger
- Add multi-position servo control based on button patterns
- Implement servo feedback using ADC
- Add wireless servo control commands
- Support multiple responders with servo arrays

---

**Author**: Decawave Applications  
**Version**: 1.0  
**Date**: December 2025
