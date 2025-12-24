# Quick Reference: Button-Servo UWB System

## Files Created

### Hardware Abstraction Layer (HAL)
- **HAL_servo.h / HAL_servo.c** - PWM servo control
  - GPIO P0.11 (PWM0 Ch0)
  - 50Hz servo frequency
  - 1000-2000us pulse range

### Button Control
- **button_handler.h / button_handler.c** - Button interrupt & debounce
  - SW1: GPIO P0.2 (BUTTON_1)
  - SW2: GPIO P0.20 (BUTTON_2)
  - 10ms debounce threshold

### UWB Integration
- **uwb_button_initiator.h / uwb_button_initiator.c** - Button-triggered ranging
  - Initiator board integration
  - Starts/stops UWB on button press

- **uwb_servo_responder.h / uwb_servo_responder.c** - Signal-triggered servo
  - Responder board integration
  - Moves servo on UWB reception
  - Auto-return after 2 seconds

### Documentation
- **BUTTON_SERVO_SETUP.md** - Complete setup guide
- **INTEGRATION_EXAMPLE.c** - Code integration examples

## Configuration Changes

### sdk_config.h
```
NRFX_PWM_ENABLED = 1
NRFX_PWM0_ENABLED = 1
NRFX_GPIOTE_ENABLED = 1
NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS = 2
```

### custom_board.h
```
BUTTON_1 = GPIO 2 (SW1)
BUTTON_2 = GPIO 20 (SW2)
SERVO_PWM_PIN = GPIO 11 (P0.11)
BUTTONS_NUMBER = 2
```

### Project File
Added to DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.emProject:
- HAL_servo.c
- button_handler.c
- uwb_button_initiator.c
- uwb_servo_responder.c

## Hardware Wiring

### Servo Connection (Responder Board)
```
Servo Brown (GND)  → GND
Servo Red (5V)     → 5V Power
Servo Orange (PWM) → GPIO P0.11
```

### Button Connections
- SW1 & SW2 are already on the board (GPIO 2 & 20)

## Quick Start Code

### Initiator Setup
```c
#include "uwb_button_initiator.h"

// In init function:
uwb_button_initiator_init();

// Create 10ms periodic task:
for (;;) {
    button_handler_process();
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

### Responder Setup
```c
#include "uwb_servo_responder.h"

// In init function:
uwb_servo_responder_init();

// In your UWB reception callback:
if (signal_received) {
    uwb_servo_responder_signal_received();
}
```

## Servo Positions

| Position | Pulse (us) | Angle |
|----------|-----------|-------|
| SERVO_POS_MIN | 1000 | 0° |
| SERVO_POS_CENTER | 1500 | 90° |
| SERVO_POS_MAX | 2000 | 180° |

## Key Functions

### Button Handler
- `button_handler_init()` - Setup buttons
- `button_handler_process()` - Call every 5-10ms
- `button_is_pressed(id)` - Check state
- `button_handler_register_callback(cb)` - Register handler

### Servo Control
- `HAL_servo_init()` - Initialize PWM
- `HAL_servo_set_position(us)` - Set pulse width (1000-2000)
- `HAL_servo_move_to_position(pos)` - Move to preset
- `HAL_servo_stop()` - Disable

### UWB Initiator
- `uwb_button_initiator_init()` - Setup
- `uwb_button_initiator_start_ranging()` - Begin transmission
- `uwb_button_initiator_stop_ranging()` - End transmission
- `uwb_button_initiator_is_ranging()` - Check status

### UWB Responder
- `uwb_servo_responder_init()` - Setup
- `uwb_servo_responder_signal_received()` - Trigger servo on signal
- `uwb_servo_responder_move_servo(us)` - Manual control
- `uwb_servo_responder_return_to_neutral()` - Reset position

## Testing Checklist

- [ ] Board 1 flashed as initiator
- [ ] Board 2 flashed as responder
- [ ] Servo connected to P0.11 on responder
- [ ] Press SW1 on initiator
- [ ] Responder servo moves to 180°
- [ ] After 2s servo returns to 90°
- [ ] Release button stops UWB transmission
- [ ] Press SW2 to test second button
- [ ] Check UART debug messages

## Troubleshooting

**Servo not moving:**
- Check connection to P0.11
- Verify servo power (5V) and ground
- Confirm HAL_servo_init() called
- Check PWM enabled in config

**Buttons not working:**
- Verify GPIO pins 2 and 20
- Check GPIOTE enabled
- Ensure button_handler_process() called
- Verify pullup configuration

**UWB not responding:**
- Check both boards configured for FiRa
- Verify channel and session settings
- Ensure responder is listening
- Check UWB antenna connections

## Debug Output

Enable debug messages in console:
```
Button X pressed/released
Servo moving to position
UWB Ranging started/stopped
Signal received
```

## Power Requirements

- **Servo stall current**: 2-3A (ensure adequate power supply)
- **Board current**: ~100mA active
- **Total**: ~500mA minimum recommended

---

**Ready to test!** Press the button on the initiator to start the UWB ranging sequence.
