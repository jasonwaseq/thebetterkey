h
```
lock:
# Responder
respf 3 2400 300 30 1 42 01:02:03:04:05:06:07:08 0 0 0x0001 0x0002
# Initiator
initf 3 2400 300 30 1 42 01:02:03:04:05:06:07:08 0 0 0x0001 0x0002

Responder: 
respf 4 2400 300 30 1 42 01:02:03:04:05:06:07:08 0 0 0x0001 0x0002

Initiator: 
initf 4 2400 300 30 1 42 01:02:03:04:05:06:07:08 0 0 0x0001 0x0002

760216246
760216253
> **Note**: To find your COM ports, use `python -m serial.tools.list_ports`

### Step 3: Configure Responder First (on Responder terminal)

Parameter order reminder: `respf <set> <slot_rstu> <block_ms> <round_slots> <rr_usage> <session_id> <vupper64> <multi_node> <round_hop> <init_addr> <resp0> [resp1 ...]`.

Type the following command on the **Responder terminal** (single-responder, no hopping):
```
respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 0 0 0x0001 0x0002
```

**Expected Output on Responder**:
```
>>> FiRa process starting <<<
>>> uwbmac_start OK <<<
>>> fira_helper_start_session OK <<<
RESP: RF Init chan=9 preamble=9 sfd=2 rframe=3 slot=2400 ms=200
RESP: Addr short=0x0002 dest=0x0001 device_type=0
UWB_SIGNAL_MONITOR: Initialized
>>> FiRa session 42 started!
```

The responder is now **listening for signals** from the initiator.

### Step 4: Configure Initiator Second (on Initiator terminal)

Type the following command on the **Initiator terminal** (single-responder, no hopping):
```
initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 0 0 0x0001 0x0002
```

**Expected Output on Initiator**:
```
>>> FiRa process starting <<<
>>> uwbmac_start OK <<<
>>> fira_helper_start_session OK <<<
INIT: RF Init chan=9 preamble=9 sfd=2 rframe=3 slot=2400 ms=200
INIT: Addr short=0x0001 dest=0x0002 device_type=1
INIT: Adding 1 controlee(s) to session
UWB_SIGNAL_MONITOR: Initialized
>>> FiRa session 42 started!
```

---

## Observing Bidirectional Communication

### Phase 1: Automatic Ranging (First 5-10 seconds)

Both boards will start periodic ranging cycles. You should see:

**On Initiator Terminal**:
```
INIT: Block 0 measurements=1 stopped_reason=255
INIT: [0] 0x0002 status=0(OK) payload_len=0 dist=1234 slot=0 nlos=0 los=1 rssi=210 fom=20
INIT: TX_OK 0x0002 data=0x000004d2
```

**On Responder Terminal**:
```
RESP: Block 0 measurements=1 stopped_reason=255
RESP: [0] 0x0001 status=0(OK) payload_len=0 dist=1234 slot=0 nlos=0 los=1 rssi=210 fom=20
RESP: RX_OK 0x0001 data=0x000004d2
```

### Critical Indicators of Success

Look for these **key success indicators**:

1. **Status = 0 (OK)**
   - `status=0(OK)` means the signal was successfully received
   - Any other status means RX failure (see status codes below)

2. **Distance Measurement**
   - `dist=1234` shows the measured distance in millimeters
   - This confirms RF signal propagation and time-of-arrival measurement

3. **Signal Quality Parameters**
   - `los=1` = Line-of-Sight (good signal)
   - `rssi=210` = Received Signal Strength (higher is better, typically 180-240)
   - `fom=20` = Figure of Merit (signal quality indicator)

4. **RSSI (Received Signal Strength Indicator)**
   - Typical range: 160-240
   - Values below 160 indicate weak signal (boards might be too far apart)
   - Values above 240 indicate strong signal (boards might be too close)

### Phase 2: Manual Button Trigger (Optional - for SP1 Payload Testing)

If you want to test sending custom payloads with button presses:

**On Initiator Terminal**:
1. Press SW1 or SW2 button on the initiator board
2. You should see:
```
INIT: TX_START 0x0002 data=0x...
INIT: TX_OK 0x0002 data=0x...
```

**On Responder Terminal**:
```
RESP: [0] 0x0001 status=0(OK) payload_len=4 dist=1234 slot=0
RESP: SP1 data: [0x42 0x54 0x4e XX]
RESP: *** BTN MATCH *** id=XX SERVO TRIGGER
```

---

## Understanding Status Codes

When you see `status=X` in the output, here's what each value means:

| Status | Meaning | Action |
|--------|---------|--------|
| **0** | OK | ✅ Signal received successfully |
| **1** | TX_FAIL | ❌ Transmission failed on initiator side |
| **2** | RX_TIMEOUT | ❌ Responder didn't receive signal (timeout) |
| **3** | RX_PHY_DEC | ❌ Physical layer decode error |
| **4** | RX_TOA | ❌ Time-of-arrival extraction failed |
| **5** | RX_STS | ❌ STS (Scrambled Timestamp Sequence) error |
| **6** | RX_MAC_DEC | ❌ MAC frame decode error |
| **7** | RX_MAC_IE_DEC | ❌ MAC Information Element decode error |
| **8** | RX_MAC_IE_MISS | ❌ MAC Information Element missing |

### Troubleshooting Status Errors

**If you see `status=2 (RX_TIMEOUT)`**:
- ❌ Responder is not receiving initiator signals
- Check board placement (should be 1-10 meters apart)
- Verify both boards have same `session_id`, `vupper64`, and `channel`
- Check antenna orientation

**If you see `status=3-8 (Various Decode Errors)`**:
- ⚠️ Signal is being received but has corruption
- Could indicate RF interference in your environment
- Try changing `rframe` setting (4 vs 3)
- Try using different slot duration or block duration

---

## Advanced Diagnostics

### Checking Configuration Match

Both boards should have **identical**:
- Session ID (42 in our example)
- Vupper64 vector (01:02:03:04:05:06:07:08)
- Slot duration (2400 rstu)
- Block duration (200 ms)
- Round duration (25 slots)
- RFRAME config (4 for SP1)
- Channel and preamble settings

### Command Reference for Quick Testing

```bash
# Simple ranging test (both boards)
initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2

# With longer block duration (more measurements per cycle)
initf 4 2400 500 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
respf 4 2400 500 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2

# With shorter slot duration (faster measurements)
initf 4 1500 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
respf 4 1500 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```

---

## Verifying Full Bidirectional Communication

### Checklist for Successful Bidirectional Setup

- [ ] **Responder Board**
  - Terminal displays boot message
  - RESPF command executes without errors
  - Shows "FiRa session started"
  - Displays "UWB_SIGNAL_MONITOR: Initialized"

- [ ] **Initiator Board**
  - Terminal displays boot message
  - INITF command executes without errors
  - Shows "FiRa session started"
  - Displays "UWB_SIGNAL_MONITOR: Initialized"

- [ ] **RF Communication (Watch Initiator Terminal)**
  - Repeatedly shows: `INIT: [0] 0x0002 status=0(OK)`
  - Distance changes slightly as boards move
  - RSSI value is between 160-240

- [ ] **RF Communication (Watch Responder Terminal)**
  - Repeatedly shows: `RESP: [0] 0x0001 status=0(OK)`
  - Same distance as shown on initiator
  - Same RSSI values as initiator

- [ ] **Bidirectional Confirmation**
  - Every ranging cycle produces results on BOTH terminals
  - Both boards show `status=0(OK)` simultaneously
  - No timeouts or errors in either direction

---

## Signal Monitoring Commands

You can get real-time bidirectional status with special commands (if implemented):

```bash
# Display bidirectional link status
STATUS

# Reset signal statistics
STATS_RST

# Show signal quality history
SIGNAL_QUALITY
```

---

## Distance Interpretation

The distance measurement tells you how far apart the boards are:

```
dist=1234  = 1234 mm = 1.234 meters = ~4 feet
dist=500   = 500 mm = 0.5 meters = ~1.6 feet
dist=5000  = 5000 mm = 5 meters = ~16 feet
```

**Recommended distances for testing**: 1-5 meters (3-16 feet)

---

## Environmental Factors Affecting Results

### Good Signal Conditions
✅ Clear line-of-sight between boards
✅ Boards 1-10 meters apart
✅ No metal obstacles between boards
✅ Away from microwave ovens and Wi-Fi routers

### Poor Signal Conditions
❌ Boards blocked by metal walls or doors
❌ Too close (less than 0.5 meters) - signal saturation
❌ Too far apart (beyond 15 meters in indoor environment)
❌ Multiple reflections (NLOS = Non-Line-of-Sight)

When `los=0` appears (NLOS condition), the ranging works but accuracy decreases.

---

## Expected Output Patterns

### Healthy Communication (Repeating Pattern)

```
Block 0: INIT sends signal → RESP receives it
Block 1: INIT sends signal → RESP receives it
Block 2: INIT sends signal → RESP receives it
...
Block N: INIT sends signal → RESP receives it
```

You should see this pattern continuously with NO gaps or timeouts.

### Unhealthy Communication (Error Pattern)

```
Block 0: INIT sends signal → RESP receives it (OK)
Block 1: INIT sends signal → RESP timeout/error
Block 2: INIT sends signal → RESP timeout/error
Block 3: INIT sends signal → RESP receives it (OK)
```

This indicates intermittent reception - check board placement and environment.

---

## Next Steps After Successful Verification

Once you've confirmed bidirectional communication is working:

1. **Move boards apart gradually** - verify distance measurements update
2. **Move boards into NLOS** - observe los=0 and reduced accuracy
3. **Test button functionality** - press SW1/SW2 to trigger payloads
4. **Monitor RSSI changes** - watch signal strength as distance changes
5. **Test multi-responder** - add a third board for one-to-many ranging

---

## Troubleshooting Summary Table

| Symptom | Likely Cause | Solution |
|---------|-------------|----------|
| No output on either board | Firmware not flashed | Reflash both boards |
| RESPF command works but INITF fails | Responder not configured | Run RESPF first, then INITF |
| Status shows timeout (2) | RF signal not reaching responder | Move boards closer, check line-of-sight |
| Status shows OK but wrong distance | Configuration mismatch | Verify session_id, vupper64 match |
| Intermittent timeouts | RF interference or far apart | Move away from Wi-Fi, reduce distance |
| RSSI < 160 | Boards too far apart | Move closer (1-3 meters) |
| RSSI > 240 | Boards too close | Move apart (3-5 meters) |

---

## Advanced Configuration

### For Indoor Testing (Cluttered Environment)
```
respf 4 2400 500 50 2 42 01:02:03:04:05:06:07:08 1 0 1 2
initf 4 2400 500 50 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```
- Longer blocks (500ms) for more stable measurements
- More slots per round (50) for better averaging

### For Outdoor Testing (Clean Environment)
```
respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```
- Standard configuration is fine outdoors
- Can work at longer distances (up to 40+ meters)

---

## What Was Implemented

### New Features Added to Support Bidirectional Testing

1. **Signal Monitoring Module** (`uwb_signal_monitor.c`)
   - Tracks TX/RX events
   - Logs signal quality metrics
   - Provides bidirectional status summary

2. **Enhanced Logging in FiRa App**
   - Verbose RF configuration output at startup
   - Detailed measurement status for each block
   - Payload detection and logging
   - Error code translation to readable format

3. **Automatic Event Logging**
   - TX_OK / TX_FAIL events
   - RX_OK / RX_ERROR events
   - Payload reception confirmation
   - Distance and signal quality per measurement

This enhancement makes it easy to:
✅ Confirm both directions are working
✅ Monitor signal quality in real-time
✅ Diagnose communication issues
✅ Verify correct board configuration
