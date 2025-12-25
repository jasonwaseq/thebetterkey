# UWB Fob Troubleshooting Guide

## Critical Fix Applied

**ISSUE:** The initiator command had TWO responders configured (addresses 1 and 2) but you only have ONE responder board!

**OLD COMMAND (WRONG):**
```
initf 3 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 0 1 2
```

**NEW COMMAND (CORRECT):**
```
initf 3 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 0 1
```

The "2" at the end created a second responder with address 0x0002 which doesn't exist!

## Step-by-Step Debug Process

### Step 1: Rebuild with Debug Logging

```bash
make clean build
```

### Step 2: Flash Both Boards

Flash the newly built firmware to BOTH boards.

### Step 3: Test Basic Ranging First (Without Button)

**Responder terminal:**
```
respf 3 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 0 1
```

**Initiator terminal (NOTE: removed the "2" at end!):**
```
initf 3 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 0 1
```

**Expected output - RESPONDER should show:**
```
RESP: Block X measurements=1 stopped_reason=255
RESP: [0] 0x0000 status=0(OK) payload_len=X dist=XXXX slot=X nlos=0 los=1 rssi=XXX fom=XXX
```

**Expected output - INITIATOR should show:**
```
INIT: Block X measurements=1 stopped_reason=255
INIT: [0] 0x0001 status=0(OK) payload_len=X dist=XXXX slot=X nlos=0 los=1 rssi=XXX fom=XXX
```

**If you see "status=2(RX_TIMEOUT)" continuously:**
- Boards are NOT syncing
- Check they're on the same channel, session_id, and RF parameters
- Make sure both are using BPRF set 3 (first parameter)
- Try moving boards closer together (within 1-2 meters)

**If you see "status=7(RX_MAC_IE_DEC)" continuously:**
- Boards are receiving something but can't decode the MAC layer
- This suggests parameter mismatch (check vupper64, session_id, addresses)

### Step 4: Once Basic Ranging Works, Test Button

**On INITIATOR:**
- Press SW1 or SW2 button
- Should see: `BTN[0] PRESS` or `BTN[1] PRESS`
- Should see: `UWB: Attempting to send BTN payload...`
- Should see: `UWB: send_data returned 0 (0=success)`

**On RESPONDER:**
- Should see: `RESP: SP1 data: [0x42 0x54 0x4e 0xXX]` (0x42='B', 0x54='T', 0x4e='N')
- Should see: `RESP: *** BTN MATCH *** id=X SERVO TRIGGER`
- Should see: `SERVO: Setting position to 1000 us` or `2000 us`
- Servo should physically move!

## Common Issues

### Issue: RX_TIMEOUT on both boards
**Cause:** Boards can't see each other's transmissions
**Fix:**
1. Verify both using BPRF set 3 (first param = 3)
2. Move boards closer (within 1 meter for testing)
3. Check antennas are properly attached
4. Verify both commands were entered correctly

### Issue: Button pressed but no send_data message
**Cause:** send_data might be failing or session not started
**Fix:**
1. Ensure basic ranging works FIRST (step 3)
2. Check for "INIT: FiRa session 42 started!" message
3. Look for send_data return code in new debug output

### Issue: Button payload sent but responder doesn't move servo
**Cause:** Responder not decoding SP1 payload correctly
**Fix:**
1. Check responder shows "SP1 data:" in logs
2. Verify payload bytes match [0x42 0x54 0x4e 0xXX]
3. Check servo wiring to P0.28
4. Verify servo has external power

### Issue: Servo initialized but doesn't move
**Cause:** Hardware issue
**Fix:**
1. Check P0.28 connection with multimeter/oscilloscope
2. Verify servo external power (5V)
3. Verify ground connection between servo and board
4. Test servo with another controller to verify it works

## Correct Commands Summary

**Single responder (your setup):**
```
# Responder:
respf 3 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 0 1

# Initiator (NOTE: only "1" at end, not "1 2"):
initf 3 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 0 1
```

**Parameters explained:**
- 3 = BPRF set 3 (SP1 mode - required for payload)
- 2400 = Slot duration
- 200 = Ranging period (ms)
- 25 = Ranging round slots
- 2 = Ranging round usage (DS-TWR)
- 42 = Session ID
- 01:02:03:04:05:06:07:08 = Vupper64 (must match on both!)
- 1 = Multi-node mode (one-to-many)
- 0 = Round hopping (disabled)
- 0 = Initiator address (for responder) or vice versa
- 1 = Responder address (0x0001)
- (no second number for single responder!)
