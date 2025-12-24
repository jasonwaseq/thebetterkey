# Testing Instructions

## Quick Start

1. **Open PowerShell** in the thebetterkey folder
2. **Run the monitor script:**
   ```powershell
   Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
   .\monitor_both.ps1
   ```
   This opens TWO windows - one for initiator (COM12), one for responder (COM13)

3. **Wait 5 seconds** for startup messages

4. **Look for these messages:**

   **Initiator should show:**
   ```
   INIT: RF Init chan=9 preamble=9 sfd=2 rframe=1 slot=2400 ms=200
   INIT: Addr short=0x0000 dest=0x0001 device_type=0
   INIT: Block 0 measurements=1 stopped_reason=255
   INIT: [0] 0x0001 status=0(OK) ...
   ```

   **Responder should show:**
   ```
   RESP: RF Init chan=9 preamble=9 sfd=2 rframe=1 slot=2400 ms=200
   RESP: Addr short=0x0001 dest=0x0000 device_type=1
   RESP: Block 0 measurements=1 stopped_reason=255
   RESP: [0] 0x0000 status=2(RX_TIMEOUT) ...
   ```

5. **Press SW1 on initiator board** and watch responder window for:
   ```
   RESP: SP1 data: [0x42 0x54 0x4e ...]
   RESP: *** BTN MATCH *** id=0 SERVO TRIGGER
   SERVO: Moving RIGHT
   ```

## Interpreting Status Codes

- `status=0(OK)` - Frame received successfully ✅
- `status=1(TX_FAIL)` - Failed to transmit ❌
- `status=2(RX_TIMEOUT)` - No frame heard (RX timeout) ❌
- `status=3(RX_PHY_DEC)` - PHY decode error ❌
- `status=4(RX_TOA)` - Time-of-arrival error ❌
- `status=5(RX_STS)` - STS segment mismatch ❌

## What to Check

1. **Do RF Init messages match on both boards?**
   - Same channel (chan=9)
   - Same preamble (preamble=9)
   - Same SFD (sfd=2)
   - Same frame config (rframe=1)

2. **Do addresses make sense?**
   - Initiator: short=0x0000, dest=0x0001
   - Responder: short=0x0001, dest=0x0000

3. **What are the measurement statuses?**
   - Initiator should show `status=0(OK)` or `status=1(TX_FAIL)`
   - Responder showing `status=2(RX_TIMEOUT)` means it's not hearing the initiator

4. **When button is pressed:**
   - Initiator should send BTN payload
   - Responder should receive it and move servo

## If Things Go Wrong

- **All RX_TIMEOUT on responder:** RF link is completely dead
  - Check physical placement (within 30cm, line-of-sight)
  - Check if boards are powered properly (LEDs blinking?)
  - Verify antenna connections

- **TX_FAIL on initiator:** Board can't transmit
  - Power cycle the initiator board
  - Check USB connection

- **Different RF configs:** Boards are misconfigured
  - Both should auto-start with identical params
  - Check that both are using same session ID (42)

- **No BTN MATCH:** Payload not received or gating not working
  - Check initiator is sending (look for UWB logs)
  - Verify responder can receive (should see some OK statuses first)
