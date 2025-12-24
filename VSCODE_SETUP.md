# VS Code Build, Flash & Monitor Setup

## Quick Start (30 seconds)

### 1. Get Board Information
```powershell
# In VS Code terminal (Ctrl+`)
python setup_vscode.py
```

This will show:
- Connected J-Link boards (serial numbers)
- Available COM ports (for serial monitoring)

### 2. Update Configuration
Edit `.vscode/tasks.json` and update these lines with your actual values:
```json
"inputs": [
  {
    "id": "initiatorSN",
    "default": "YOUR_BOARD_1_SN"  // Replace with first board SN
  },
  {
    "id": "responderSN", 
    "default": "YOUR_BOARD_2_SN"  // Replace with second board SN
  },
  {
    "id": "initiatorCOM",
    "default": "COM5"  // Replace with first COM port
  },
  {
    "id": "responderCOM",
    "default": "COM6"  // Replace with second COM port
  }
]
```

### 3. Build & Flash
```
Ctrl+Shift+B          Build project
Ctrl+Shift+P          Command palette
"Flash Initiator"     Flash first board
"Flash Responder"     Flash second board
```

### 4. Monitor
```
Ctrl+Shift+P
"Monitor Initiator"   Watch first board output
"Monitor Responder"   Watch second board output
```

---

## Installation Requirements

### Windows Prerequisites

#### 1. **Nordic nRF Command Line Tools**
```powershell
# Download from: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools
# Install with default settings

# Verify installation:
nrfjprog -i
```

#### 2. **Python Serial Tools**
```powershell
pip install pyserial
```

#### 3. **J-Link Drivers** (if not already installed)
- Download from: https://www.segger.com/downloads/jlink/
- Install J-Link software package

### Linux Prerequisites

```bash
# Install nRF tools
sudo apt-get install nrf5-command-line-tools

# Install Python serial tools
pip install pyserial

# Add USB permissions (may not need if sudo)
sudo usermod -a -G dialout $USER
```

### macOS Prerequisites

```bash
# Using Homebrew
brew install nrf-command-line-tools
pip install pyserial

# J-Link drivers from: https://www.segger.com/downloads/jlink/
```

---

## Finding Board Information

### Get Connected Boards (J-Link SNs)

**Option 1: Using setup script (easiest)**
```powershell
python setup_vscode.py
```

**Option 2: Using nrfjprog directly**
```powershell
nrfjprog -i
```

Output example:
```
Connected probes:
0. DK 680000000 J-Link nrf52833_dk
1. DK 680000001 J-Link nrf52833_dk
```

The numbers at the end (680000000, 680000001) are your SNs.

**Option 3: From Device Manager (Windows)**
1. Plug in board via USB
2. Open Device Manager
3. Look for "SEGGER J-Link"
4. Right-click → Properties → Details
5. Serial Number field shows the SN

### Get Serial Ports (COM ports)

**Option 1: Using setup script**
```powershell
python setup_vscode.py
```

**Option 2: Using pyserial**
```powershell
python -m serial.tools.list_ports
```

Output example:
```
COM5 - USB Serial Device (CP210x USB UART Bridge)
COM6 - USB Serial Device (CP210x USB UART Bridge)
```

**Option 3: From Device Manager (Windows)**
1. Open Device Manager
2. Expand "Ports (COM & LPT)"
3. Look for "USB-to-Serial COM Adapter" or "CP210x"
4. Note the COM port number

**Option 4: Manual via Device Manager**
1. Plug in one board
2. Check Device Manager, note port (e.g., COM5)
3. Plug in second board
4. Check Device Manager, note new port (e.g., COM6)

---

## Detailed Workflow

### Complete Build & Flash Cycle

**Step 1: Connect Boards**
```
- Plug Board 1 (Initiator) via USB
- Plug Board 2 (Responder) via USB
- Wait 2 seconds for drivers to initialize
```

**Step 2: Identify Boards**
```powershell
# In VS Code terminal:
python setup_vscode.py

# Note the output:
# Board SNs: 680000000, 680000001
# COM Ports: COM5, COM6
```

**Step 3: Update .vscode/tasks.json**
```json
Edit line in inputs section:
"initiatorSN": "680000000",
"responderSN": "680000001",
"initiatorCOM": "COM5",
"responderCOM": "COM6"
```

**Step 4: Build Project**
```
Press: Ctrl+Shift+B

Or press Ctrl+Shift+P and select "Build Project"

Output in terminal should show:
[✓] Build successful
[✓] Linking complete
[✓] Output: Output/.../DWM3001CDK...hex
```

**Step 5: Flash Initiator**
```
Press: Ctrl+Shift+P
Type: Flash Initiator
Press: Enter

When prompted:
"Enter Initiator Board Serial Number"
Type: 680000000
Press: Enter

Wait for:
[✓] Flashing completed successfully
```

**Step 6: Flash Responder**
```
Press: Ctrl+Shift+P
Type: Flash Responder
Press: Enter

When prompted:
"Enter Responder Board Serial Number"
Type: 680000001
Press: Enter

Wait for:
[✓] Flashing completed successfully
```

**Step 7: Open Serial Monitors**

Terminal 1 (Initiator):
```
Ctrl+Shift+P
Type: Monitor Initiator
Press: Enter

When prompted:
"Enter Initiator Board COM Port"
Type: COM5
Press: Enter

Should show: "--- Miniterm on COM5 115200,8,N,1 ---"
```

Terminal 2 (Responder):
```
Ctrl+Shift+P
Type: Monitor Responder
Press: Enter

When prompted:
"Enter Responder Board COM Port"
Type: COM6
Press: Enter

Should show: "--- Miniterm on COM6 115200,8,N,1 ---"
```

**Step 8: Run Tests**
```
Initiator Terminal:
  Press SW1 button on board

Expected:
  [Initiator] Button 0 pressed - starting UWB ranging
  [Responder] [RX] Signal received - Servo moving
  [Responder] Servo position: 2000us (180 degrees)
  [Wait 2 seconds]
  [Responder] Servo position: 1500us (90 degrees - auto-return)
  [Initiator] Button 0 released - stopping UWB ranging
```

---

## VS Code Task Reference

### Available Tasks

| Task | Shortcut | Purpose |
|------|----------|---------|
| **Build Project** | Ctrl+Shift+B | Compile code |
| **Flash Initiator** | Ctrl+Shift+P → "Flash Initiator" | Program board 1 |
| **Flash Responder** | Ctrl+Shift+P → "Flash Responder" | Program board 2 |
| **Monitor Initiator** | Ctrl+Shift+P → "Monitor Initiator" | Watch board 1 output |
| **Monitor Responder** | Ctrl+Shift+P → "Monitor Responder" | Watch board 2 output |
| **List Boards** | Ctrl+Shift+P → "List Connected Boards" | Show J-Link boards |
| **List Ports** | Ctrl+Shift+P → "List Serial Ports" | Show COM ports |

### Opening Task Terminal

**Method 1: Keyboard (fastest)**
```
Ctrl+Shift+P         Open command palette
Type task name      (e.g., "Flash Initiator")
Press Enter
```

**Method 2: Terminal Menu**
```
View → Terminal → Run Task
Select task from dropdown
```

**Method 3: Using Keyboard Shortcut**
```
Ctrl+Shift+B         Always runs "Build Project"
```

---

## Troubleshooting

### "nrfjprog not found"
```
Solution:
1. Install nRF Command Line Tools
2. Add to PATH: C:\Program Files\Nordic Semiconductor\nrf-command-line-tools\bin
3. Restart VS Code
4. Test: Open PowerShell → nrfjprog -i
```

### "No boards detected"
```
Check:
1. USB cable connected securely
2. Board powers on (LED blinks)
3. Try different USB port
4. Check Device Manager for "SEGGER J-Link"
5. Update J-Link firmware: https://www.segger.com/downloads/jlink/

Run diagnostics:
nrfjprog -i (should show boards)
```

### "Serial port in use" error
```
Solution:
1. Close other serial monitors (Arduino IDE, PuTTY, etc.)
2. Click "X" on miniterm tab in VS Code
3. Close and reopen VS Code
4. Try different COM port number
```

### "Flash failed" error
```
Check:
1. Board is powered on
2. Board shows in: nrfjprog -i
3. Try: nrfjprog -r (reset board)
4. Check J-Link is not locked: nrfjprog --unlock
5. Update firmware if older than 2 years old
```

### Serial output shows garbled text
```
Solution:
1. Check COM port is correct: python -m serial.tools.list_ports
2. Baud rate is 115200 (set in tasks.json)
3. Close and reopen serial monitor
4. Try different USB port
5. Check USB cable (should be high quality)
```

### "Cannot find miniterm" error
```
Solution:
1. Install pyserial: pip install pyserial
2. Verify: python -m serial.tools.miniterm
3. Try: python -c "import serial; print(serial.__file__)"
```

---

## Advanced Usage

### Custom Build Flags

Edit tasks.json to modify build command:
```json
{
  "label": "Build with Debug",
  "type": "shell",
  "command": "make",
  "args": ["clean", "build", "DEBUG=1"],
}
```

### Faster Flash Without Rebuild

Create new task:
```json
{
  "label": "Flash Initiator (No Build)",
  "type": "shell",
  "command": "nrfjprog --program ${workspaceFolder}/Output/.../DWM3001CDK.hex ...",
  "dependsOn": []
}
```

### Auto-Monitor After Flash

Modify tasks to chain operations:
```json
{
  "label": "Flash & Monitor Initiator",
  "dependsOn": ["Flash Initiator", "Monitor Initiator"]
}
```

### Custom Serial Monitor Settings

Edit tasks.json miniterm args:
```json
"args": [
  "-m", "serial.tools.miniterm",
  "${input:initiatorCOM}",
  "115200",
  "--echo",           // Echo input
  "--filter=colorize" // Colorize output
]
```

---

## Tips & Tricks

### Save Time with Keyboard Shortcuts

```
Ctrl+Shift+B              Build (quickest)
Ctrl+K Ctrl+B             Rebuild (clean build)
Ctrl+Shift+P F Flash Ini  Flash initiator fast
Ctrl+Shift+P M Initiator  Monitor initiator fast
```

### Multiple Serial Monitors

Open 2-3 split terminals:
```
Ctrl+Shift+P → "Monitor Initiator" (creates new terminal)
Ctrl+\ (split terminal)
Ctrl+Shift+P → "Monitor Responder" (use other terminal)
Now see both outputs side-by-side!
```

### Quick Board Reset

```powershell
nrfjprog -r
```

### Erase All Flash

```powershell
nrfjprog -e
```

### Check Board Firmware Version

```powershell
nrfjprog --readuii
```

---

## One-Command Setup (Experts Only)

Save as `flash.bat` (Windows):
```batch
@echo off
python setup_vscode.py
echo.
echo.
pause
```

Then double-click `flash.bat` to auto-detect all settings!

---

## Support

If tasks aren't working:
1. Check .vscode/tasks.json has correct syntax (VS Code will show errors)
2. Verify all paths are correct for your system
3. Test commands manually in PowerShell first
4. Check VS Code Output panel for detailed error messages

---

**Everything configured? You're ready to test!** 🚀

See `TESTING_PROCEDURE.md` for the complete test plan.
