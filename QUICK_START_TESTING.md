# Quick Reference: Bidirectional UWB Communication Testing

## Hardware Serial Numbers
- **Initiator**: SN 760216253 → Sends signals
- **Responder**: SN 760216246 → Receives & responds

---

## Step 1: Find Your COM Ports
```bash
python -m serial.tools.list_ports
```

---

## Step 2: Open Two Terminal Windows

### Terminal A (Responder - SN: 760216246)
```bash
python -m serial.tools.miniterm COM_PORT_A 115200 --filter=colorize
```

### Terminal B (Initiator - SN: 760216253)
```bash
python -m serial.tools.miniterm COM_PORT_B 115200 --filter=colorize
```

---

## Step 3: Configure Both Boards

### On Terminal A Type:
```
respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```

### On Terminal B Type:
```
initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```

---

## Step 4: Watch for Success Signs

### Look for this on BOTH terminals:
```
status=0(OK)
```

### Example Good Output:
```
RESP: [0] 0x0001 status=0(OK) payload_len=0 dist=1234 slot=0 nlos=0 los=1 rssi=210
INIT: [0] 0x0002 status=0(OK) payload_len=0 dist=1234 slot=0 nlos=0 los=1 rssi=210
```

✅ **If you see `status=0(OK)` on BOTH boards repeatedly** → You have bidirectional communication!

---

## Understanding the Output

```
[0]          → Measurement index
0x0001/0002  → Remote board address (responder/initiator)
status=0(OK) → Signal received successfully (0=OK, 2=timeout, others=error)
dist=1234    → Distance in millimeters (1234mm = 1.23 meters)
rssi=210     → Signal strength (160-240 is good range)
los=1        → Line-of-sight (1=yes, 0=NLOS obstructed)
```

---

## If You See Errors

### `status=2(RX_TIMEOUT)`
❌ Responder not receiving initiator signals
- Move boards closer (try 1-3 meters)
- Check line-of-sight (no obstacles)
- Verify both use `session_id=42`

### `status=3-8` (Decode Errors)
❌ Signal corrupted or configuration mismatch
- Verify addresses match:
  - Initiator: `short_addr=1` `dest_addr=2`
  - Responder: `short_addr=2` `dest_addr=1`
- Try moving boards further apart
- Change environment (away from Wi-Fi)

### No output on Responder
❌ Responder not configured
- Make sure you typed `respf` command first
- Wait 2-3 seconds after respf before starting initiator

---

## Distance Interpretation

```
dist=500     → 0.5 meters (1.6 feet)
dist=1500    → 1.5 meters (5 feet)
dist=5000    → 5 meters (16 feet)
dist=10000   → 10 meters (33 feet)
```

Boards work best at **1-10 meters** distance.

---

## RSSI (Signal Strength) Guide

```
RSSI < 160   → ⚠️  Weak (boards too far)
RSSI 160-220 → ✅ Good (optimal)
RSSI > 240   → ⚠️  Strong (boards too close)
```

---

## Testing Checklist

- [ ] Boards powered on
- [ ] USB cables connected (J20 for serial, J9 for flashing)
- [ ] Firmware built and flashed
- [ ] Responder terminal open and ready
- [ ] Initiator terminal open and ready
- [ ] Responder configured with `respf` command
- [ ] Initiator configured with `initf` command
- [ ] Both showing `status=0(OK)` messages
- [ ] Distance values changing slightly (boards respond to movement)
- [ ] RSSI values in 160-240 range

---

## Advanced Configuration Options

### For Cluttered Indoor Environment (More stable measurements):
```
respf 4 2400 500 50 2 42 01:02:03:04:05:06:07:08 1 0 1 2
initf 4 2400 500 50 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```
(Longer block duration = more measurements per cycle)

### For Outdoor/Long Distance:
```
respf 4 3000 300 40 2 42 01:02:03:04:05:06:07:08 1 0 1 2
initf 4 3000 300 40 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```
(Longer slots = better range, slower updates)

### For Fastest Updates:
```
respf 4 1500 100 15 2 42 01:02:03:04:05:06:07:08 1 0 1 2
initf 4 1500 100 15 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```
(Shorter slots = faster but needs closer boards)

---

## Button Test (Optional)

### If RFRAME=4 (SP1 payload):
1. Press SW1 or SW2 on **Initiator** board
2. Watch **Responder** terminal for:
   ```
   RESP: SP1 data: [0x42 0x54 0x4e XX]
   RESP: *** BTN MATCH *** id=XX SERVO TRIGGER
   ```

If servo is connected, it will move!

---

## Detailed Documentation

For more information, see:
- `BIDIRECTIONAL_COMMUNICATION_GUIDE.md` - Step-by-step with expected outputs
- `IMPLEMENTATION_SUMMARY.md` - Technical details on how it works
- `README.md` - General project information

---

## Troubleshooting Flow

```
Do both boards show status=0(OK)?
│
├─ YES → ✅ Bidirectional communication working!
│        Move on to testing different distances
│
└─ NO → status=2 (RX_TIMEOUT)?
         │
         ├─ YES → ❌ Responder not hearing initiator
         │        │
         │        ├─ Move boards 1-3 meters closer
         │        ├─ Check line-of-sight (remove obstacles)
         │        └─ Move away from Wi-Fi/microwave
         │
         └─ NO → status=3-8 (Decode error)?
                 │
                 ├─ YES → ❌ Signal corrupted
                 │        │
                 │        ├─ Check address configuration
                 │        └─ Try different slot duration
                 │
                 └─ NO → No output at all?
                         │
                         └─ Responder not configured
                            Type: respf 4 2400 200 25 2 42...
```

---

## Key Addresses in Commands

```
Parameter meanings:
initf [rframe] [slot] [block] [round] [rr] [session] [vupper] [multi] [hop] [init_addr] [resp_addr]
              ^^^^^^ ^^^^^^  ^^^^^^^  ^^^^^^                                     ^^^^^^^   ^^^^^^^^
              |     |       |        |                                          |         └─ Responder (0x0002)
              |     |       |        └─ Ranging Round usage (2=SS-TWR)          └─ Initiator (0x0001)
              |     |       └─ Round duration (25 slots)
              |     └─ Block duration (200ms = measurements every 200ms)
              └─ Slot duration (2400 rstu ~2.4ms per slot)

               For your 2 board setup, always use:
               ├─ initf ... 1 0 1 2  (Initiator addr=1, Responder addr=2)
               └─ respf ... 1 0 1 2  (same for responder)
```

---

## What Each Board Does

### Initiator (760216253)
```
┌─────────────────────────────────┐
│ Every 200ms:                    │
├─ Transmit ranging poll to 0x0002
├─ Wait for response              │
├─ Measure response time          │
├─ Calculate: distance = delay × c
├─ Report: dist, rssi, los, status
└─ Repeat                         │
└─────────────────────────────────┘
```

### Responder (760216246)
```
┌─────────────────────────────────┐
│ Continuous:                     │
├─ Listen for initiator signals   │
├─ On receive: auto-transmit      │
│             response            │
├─ Receive initiator's measurement
├─ Report same: dist, rssi, los   │
└─ Wait for next poll            │
└─────────────────────────────────┘
```

---

## Expected Behavior by Phase

### Phase 1: Right After Configuration (First 2 seconds)
```
[Boards synchronizing, setting up radio, calibrating]
May see: "FiRa session starting" messages
May see: "uwbmac_start OK"
May see: "FiRa session started"
```

### Phase 2: Ranging Begins (2-5 seconds in)
```
[First ranging cycles happening]
Initiator shows: "Block 0 measurements=1"
Responder shows: "Block 0 measurements=1"
Watch for status values
```

### Phase 3: Continuous Operation (5+ seconds)
```
[Steady state ranging]
Every 200ms: Both boards print one line
Both show same distance value (±10mm)
Both show status=0(OK)
```

---

## Still Having Issues?

See the full troubleshooting guide: `BIDIRECTIONAL_COMMUNICATION_GUIDE.md`

Key sections:
- "Troubleshooting Summary Table" - Common issues and solutions
- "Understanding Status Codes" - What each error means
- "Environmental Factors" - How your surroundings affect signals
