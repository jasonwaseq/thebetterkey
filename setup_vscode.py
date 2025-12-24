#!/usr/bin/env python3
"""
VS Code Serial Monitor Setup Helper
Helps identify boards and set up serial monitoring
"""

import subprocess
import json
import sys

def get_connected_boards():
    """Get list of connected nRF boards via nrfjprog"""
    print("\n" + "="*60)
    print("CHECKING CONNECTED J-LINK BOARDS")
    print("="*60)
    try:
        result = subprocess.run(['nrfjprog', '-i'], capture_output=True, text=True)
        print(result.stdout)
        if "Connected probes:" in result.stdout:
            # Extract serial numbers
            lines = result.stdout.split('\n')
            serials = []
            for line in lines:
                if 'J-Link' in line or 'DK' in line:
                    # Try to extract serial number
                    parts = line.split()
                    for part in parts:
                        if part.isdigit() and len(part) == 9:
                            serials.append(part)
            return serials
    except FileNotFoundError:
        print("❌ nrfjprog not found. Install it: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools")
    except Exception as e:
        print(f"❌ Error: {e}")
    return []

def get_serial_ports():
    """Get list of available COM ports"""
    print("\n" + "="*60)
    print("CHECKING AVAILABLE SERIAL PORTS")
    print("="*60)
    try:
        result = subprocess.run([sys.executable, '-m', 'serial.tools.list_ports'], 
                              capture_output=True, text=True)
        print(result.stdout)
        
        # Parse COM ports
        ports = []
        for line in result.stdout.split('\n'):
            if 'COM' in line or '/dev/tty' in line:
                port = line.split()[0]
                if port:
                    ports.append(port)
        return ports
    except Exception as e:
        print(f"❌ Error: {e}")
        print("Install pyserial: pip install pyserial")
    return []

def setup_tasks_inputs(boards, ports):
    """Generate recommended inputs for tasks.json"""
    print("\n" + "="*60)
    print("RECOMMENDED VS CODE SETUP")
    print("="*60)
    
    if len(boards) >= 2:
        print(f"\n✓ Found {len(boards)} boards:")
        for i, sn in enumerate(boards, 1):
            board_type = "Initiator" if i == 1 else "Responder"
            print(f"  {i}. {board_type}: SN = {sn}")
        print(f"\nAdd to tasks.json inputs:")
        print(f'  "initiatorSN": "{boards[0]}"')
        print(f'  "responderSN": "{boards[1]}"')
    elif len(boards) == 1:
        print(f"\n⚠ Found only 1 board: SN = {boards[0]}")
        print("  Connect second board via USB")
    else:
        print("\n❌ No boards detected!")
        print("  1. Check USB connections")
        print("  2. Check J-Link drivers installed")
        print("  3. Run: nrfjprog -i (directly)")
    
    if len(ports) >= 2:
        print(f"\n✓ Found {len(ports)} serial ports:")
        for i, port in enumerate(ports, 1):
            board_type = "Initiator" if i == 1 else "Responder"
            print(f"  {i}. {board_type}: {port}")
        print(f"\nAdd to tasks.json inputs:")
        print(f'  "initiatorCOM": "{ports[0]}"')
        print(f'  "responderCOM": "{ports[1]}"')
    elif len(ports) == 1:
        print(f"\n⚠ Found only 1 serial port: {ports[0]}")
        print("  Connect second board via USB")
    else:
        print("\n⚠ No serial ports detected!")
        print("  Boards may not be connected via USB UART")

def main():
    print("\n" + "="*60)
    print("VSCODE SERIAL MONITOR SETUP HELPER")
    print("="*60)
    
    boards = get_connected_boards()
    ports = get_serial_ports()
    
    setup_tasks_inputs(boards, ports)
    
    print("\n" + "="*60)
    print("NEXT STEPS")
    print("="*60)
    print("""
1. Update .vscode/tasks.json with your board SNs and COM ports
2. In VS Code, press Ctrl+Shift+B to build
3. Press Ctrl+Shift+P and search for:
   - "Flash Initiator Board" (or Responder)
   - "Monitor Initiator Serial" (or Responder)
4. Follow the prompts to enter board info
5. Enjoy automated builds and monitoring!
    """)

if __name__ == '__main__':
    main()
