# Start both serial monitors side-by-side
# Usage: .\monitor_both.ps1

Write-Host "Starting serial monitors for initiator and responder..." -ForegroundColor Green
Write-Host "Initiator will open in left window (COM12)" -ForegroundColor Cyan
Write-Host "Responder will open in right window (COM13)" -ForegroundColor Cyan
Write-Host ""
Write-Host "Instructions:" -ForegroundColor Yellow
Write-Host "1. Wait for RF Init messages (should see within 2 seconds)"
Write-Host "2. Look for measurement blocks starting with 'INIT:' and 'RESP:'"
Write-Host "3. Check status codes (OK, RX_TIMEOUT, etc.)"
Write-Host "4. Press SW1 on initiator board and watch for BTN MATCH"
Write-Host ""

# Kill any existing miniterm processes
Get-Process python -ErrorAction SilentlyContinue | Where-Object {$_.CommandLine -like "*miniterm*"} | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Start initiator monitor in new window
Write-Host "Opening initiator monitor (COM12)..." -ForegroundColor Green
Start-Process PowerShell -ArgumentList "-NoExit", "-Command", "python -m serial.tools.miniterm COM12 115200" -WindowStyle Normal

Start-Sleep -Seconds 1

# Start responder monitor in new window
Write-Host "Opening responder monitor (COM13)..." -ForegroundColor Green
Start-Process PowerShell -ArgumentList "-NoExit", "-Command", "python -m serial.tools.miniterm COM13 115200" -WindowStyle Normal

Write-Host "Both monitors started! Check the new windows." -ForegroundColor Green
Write-Host "Close them when done." -ForegroundColor Yellow
