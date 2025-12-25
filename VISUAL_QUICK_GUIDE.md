# 📡 BIDIRECTIONAL UWB COMMUNICATION - TESTING FLOWCHART

## Visual Quick Start

```
START
  │
  ├─→ Open 2 Terminal Windows
  │
  ├─→ Terminal A: python -m serial.tools.miniterm COM_PORT_A 115200
  │
  ├─→ Terminal B: python -m serial.tools.miniterm COM_PORT_B 115200
  │
  ├─→ Type on Terminal A (Responder SN: 760216246):
  │   respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
  │   │
  │   └─→ Wait 2 seconds...
  │
  ├─→ Type on Terminal B (Initiator SN: 760216253):
  │   initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
  │
  └─→ WATCH BOTH TERMINALS
     │
     ├─ Both show: "Block 0 measurements=1" ?
     │  │
     │  ├─ YES → Both show "status=0(OK)" ?
     │  │  │
     │  │  ├─ YES → ✅✅✅ SUCCESS! BIDIRECTIONAL COMMUNICATION WORKING!
     │  │  │        Move to testing section
     │  │  │
     │  │  └─ NO → Check troubleshooting guide
     │  │
     │  └─ NO → Responder not configured
     │         Restart and make sure respf command runs first
     │
     └─ See this symbol "→"? That's a good signal!
```

---

## What Each Field Tells You

### The Output Line (What You'll See)
```
RESP: [0] 0x0001 status=0(OK) payload_len=0 dist=1234 slot=0 nlos=0 los=1 rssi=210 fom=20
      │   │      │          │             │          │      │      │      │        │    │
      │   │      │          │             │          │      │      │      │        │    └─ Figure of Merit (quality)
      │   │      │          │             │          │      │      │      │        └─ RSSI (signal strength)
      │   │      │          │             │          │      │      │      └─ Line-of-Sight (1=yes, 0=no)
      │   │      │          │             │          │      │      └─ NLOS indicator (0=good)
      │   │      │          │             │          │      └─ Slot index
      │   │      │          │             │          └─ Block index (measurement cycle)
      │   │      │          │             └─ Payload length (0=ranging, >0=data)
      │   │      │          └─ Status code (0=OK, 2=timeout, 3-8=errors)
      │   │      └─ Status description
      │   └─ Remote board address
      └─ Measurement index

RESPONDER   INITIATOR
Shows:      Shows:
RESP: ...0x0001... | INIT: ...0x0002...
(Reading from initiator) | (Reading from responder)
```

---

## Status Code Decoder

```
status=0(OK)      ✅ PERFECT    → Signal received successfully
status=1(TX_FAIL) ❌ INITIATOR  → Initiator couldn't transmit
status=2(RX_TIMEOUT) ❌ RESPONDER → Responder didn't respond
status=3-8        ⚠️  ERROR     → Signal corrupted or malformed

IF YOU SEE:
✅✅ status=0(OK) on BOTH terminals → You have bidirectional communication!
❌ status=2 on responder → Responder can't hear initiator (too far, blocked, or config mismatch)
⚠️ status=3-8 on either → Signal problem (environment interference or config mismatch)
```

---

## Distance Ruler (Millimeters to Feet)

```
dist=0000   → Too close (boxes touching)
dist=0500   → 0.5m  (1.6 feet)    ├─ Indoor optimal
dist=1000   → 1m    (3.3 feet)    │  range
dist=2000   → 2m    (6.6 feet)    ├─ 1-10 meters
dist=3000   → 3m    (10 feet)     │  (3-33 feet)
dist=5000   → 5m    (16 feet)     │
dist=7500   → 7.5m  (25 feet)     ├─ Maximum
dist=10000  → 10m   (33 feet)     │  indoor
dist=15000  → 15m   (49 feet)     │
dist=20000  → 20m   (66 feet)     └─ Outdoor edge
```

---

## Signal Strength Guide (RSSI)

```
rssi=240   ⚠️ TOO STRONG ━━━━━━ Boards too close (< 0.5m)
rssi=220   ⚠️ STRONG           Boards very close (0.5-1m)
rssi=210   ✅ OPTIMAL ━━━━━━━━ Boards at good distance (1-5m) ← AIM HERE
rssi=190   ✅ GOOD              Boards at medium distance (5-10m)
rssi=170   ⚠️ WEAK              Boards far apart (10-15m)
rssi=160   ❌ TOO WEAK ━━━━━━ Boards too far or blocked
rssi=140   ❌ FAILING            No communication likely
```

---

## Decision Tree for Troubleshooting

```
                        START: No status=0(OK)?
                                  │
                ┌─────────────────┼─────────────────┐
                │                 │                 │
        Both show OK?      Only responder      Neither shows
                │           shows OK?          any output?
                │                 │                 │
               YES               NO                NO
                │                 │                 │
            ✅ SUCCESS!      Check resp config   Check config
                │           on responder terminal order:
                │                 │              1. Responder first
                │           Is responder        2. Wait 2 sec
                │           configured?         3. Then initiator
                │                 │              │
                │               /   \            └──→ Reconfigure
                │              YES  NO               both
                │              │    │
                │              │    └──→ Type: respf 4 2400...
                │              │        Wait 2 sec
                │              │        Type: initf 4 2400...
                │              │
                │         ┌────┴────┐
                │         │Boards at │
                │       1-3 meters?  │
                │         │          │
                │        YES  NO    │
                │         │    │     │
                │         │    └─→ Move closer
                │         │         Try 1-3 meters
                │         │         Check line-of-sight
                │         │
                │         └─→ Check status=2?
                │               │
                │              YES
                │               │
                │         RF signal not
                │         reaching responder:
                │         - Move boards closer
                │         - Remove obstacles
                │         - Move away from Wi-Fi
                │
                ├─→ Check RSSI range?
                │       │
                │     160-240? YES → ✅ Good
                │       │
                │     < 160?  → Boards too far
                │     > 240?  → Boards too close
                │
                ├─→ Check los value?
                │       │
                │     los=1 → ✅ Line-of-sight (good)
                │     los=0 → ⚠️ NLOS (obstructed)
                │
                └─→ ✅ COMMUNICATION VERIFIED!
```

---

## Command Parameter Quick Reference

```
respf/initf RFRAME SLOT BLOCK ROUND RR SESSION VUPPER MULTI HOP INIT RESP
            └─┬──┘ └─┬──┘ └─┬──┘ └──┬──┘ └┬┘ └──┬───┘ └─┬──┘ └┬┘ └──┬──┘
              │      │      │       │     │    │        │    │    │
              │      │      │       │     │    │        │    │    └─ Responder addr (2)
              │      │      │       │     │    │        │    └─ Initiator addr (1)
              │      │      │       │     │    │        └─ Round hopping (0)
              │      │      │       │     │    └─ Multi-mode (1)
              │      │      │       │     └─ Session ID: MUST MATCH!
              │      │      │       └─ RR usage (2=two-way)
              │      │      └─ Block (200=every 200ms)
              │      └─ Slot (2400=2.4ms)
              └─ RFRAME (4=SP1 with payload)

COPY THIS COMMAND (just paste, don't retype):

Responder:
respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2

Initiator:
initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
```

---

## Expected Output Timeline

```
T=0 seconds:  Type RESPF command
              ↓
T=0.5s:       Responder shows: ">>> FiRa process starting <<<"
              ">>> FiRa session 42 started!"
              Responder is LISTENING...
              
T=2 seconds:  Type INITF command (responder ready)
              ↓
T=2.5s:       Initiator shows: ">>> FiRa process starting <<<"
              ">>> FiRa session 42 started!"
              Initiator starts TRANSMITTING...
              
T=3 seconds:  Both terminals start showing measurements
              RESPONDER: "Block 0 measurements=1 ... status=0(OK)"
              INITIATOR: "Block 0 measurements=1 ... status=0(OK)"
              
T=3.2s:       RESPONDER: "Block 1 measurements=1 ... status=0(OK)"
              INITIATOR: "Block 1 measurements=1 ... status=0(OK)"
              
T=3.4s:       (repeating every 200ms from here on)
              Both continuously show measurements
              
✅ BIDIRECTIONAL COMMUNICATION ACTIVE!
```

---

## The 30-Second Success Test

```
1. Two terminals open
2. Type: respf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
3. Wait 2 seconds
4. Type: initf 4 2400 200 25 2 42 01:02:03:04:05:06:07:08 1 0 1 2
5. Wait 1 second
6. Look at BOTH terminals
7. Do you see "status=0(OK)" on BOTH?
   YES → ✅ SUCCESS!
   NO  → See troubleshooting
```

---

## Common Patterns You'll See

### ✅ HEALTHY (Bidirectional Working)
```
RESP: [0] 0x0001 status=0(OK) dist=1234 rssi=210
INIT: [0] 0x0002 status=0(OK) dist=1234 rssi=210
RESP: [0] 0x0001 status=0(OK) dist=1235 rssi=209
INIT: [0] 0x0002 status=0(OK) dist=1235 rssi=209
```
→ Both show OK, same distance, similar RSSI

### ❌ UNHEALTHY (Responder not hearing)
```
RESP: [0] 0x0001 status=2(RX_TIMEOUT)
INIT: [0] 0x0002 status=0(OK) dist=1234 rssi=210
RESP: [0] 0x0001 status=2(RX_TIMEOUT)
INIT: [0] 0x0002 status=0(OK) dist=1234 rssi=210
```
→ Only initiator gets OK, responder times out

### ⚠️ PROBLEMATIC (Occasional failures)
```
RESP: [0] 0x0001 status=0(OK) dist=1234 rssi=210
INIT: [0] 0x0002 status=0(OK) dist=1234 rssi=210
RESP: [0] 0x0001 status=2(RX_TIMEOUT)
INIT: [0] 0x0002 status=2(RX_TIMEOUT)
RESP: [0] 0x0001 status=0(OK) dist=1234 rssi=210
```
→ Intermittent failures, check environment

---

## Moving Forward

Once you see `status=0(OK)` on both boards:

1. **Move the boards** → Watch dist value change
2. **Increase distance** → Watch rssi decrease
3. **Add obstacle** → Watch los change from 1 to 0
4. **Press button** → Watch payload data appear
5. **Multiple responders** → Add more boards!

---

## File Guide

| File | Read This | Time |
|------|-----------|------|
| **QUICK_START_TESTING.md** | First! Quick reference | 5 min |
| **This file** | Visual overview | 10 min |
| **BIDIRECTIONAL_COMMUNICATION_GUIDE.md** | Detailed steps | 30 min |
| **IMPLEMENTATION_SUMMARY.md** | How it works | 20 min |
| **IMPLEMENTATION_COMPLETE.md** | Big picture summary | 10 min |

---

## One-Liner Verification

```bash
# After typing both respf and initf, you should see this pattern:

Watch for this: "status=0(OK)" appearing on BOTH terminal windows simultaneously
Every 200ms: You should see one new line on each terminal
Pattern: RESPONDER shows 0x0001, INITIATOR shows 0x0002
Signal Quality: rssi between 160-240, los=1 or los=0

IF ALL OF ABOVE ✅ → You have working bidirectional UWB communication!
```

---

## That's It!

You're now ready to test bidirectional communication on your two DWM3001CDK boards.

**Next step: Read QUICK_START_TESTING.md**

Good luck! 🚀
