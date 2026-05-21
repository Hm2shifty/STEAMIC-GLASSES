# ─────────────────────────────────────────
#  EchoNav — scripts/flash_esp32.ps1
#  Flashes the compiled firmware to both
#  ESP32 boards over USB using esptool.
#
#  How to run:
#  1. Plug in the LEFT ESP32 via USB
#  2. Open PowerShell in the project folder
#  3. Run: .\scripts\flash_esp32.ps1 -Board left -Port COM3
#  4. Unplug, plug in RIGHT ESP32
#  5. Run: .\scripts\flash_esp32.ps1 -Board right -Port COM4
#
#  Find your COM port in Device Manager
#  under "Ports (COM & LPT)"
#
#  Requires: esptool.py
#  Install: pip install esptool
# ─────────────────────────────────────────

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("left", "right")]
    [string]$Board,

    [Parameter(Mandatory=$true)]
    [string]$Port    # e.g. COM3 or COM4
)

Write-Host ""
Write-Host "=== EchoNav ESP32 Flasher ===" -ForegroundColor Cyan
Write-Host "    Board : $Board" -ForegroundColor White
Write-Host "    Port  : $Port"  -ForegroundColor White
Write-Host ""


# ── Check esptool is installed ────────────
Write-Host "[1/4] Checking esptool..." -ForegroundColor Yellow

esptool.py version 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] esptool not found." -ForegroundColor Red
    Write-Host "        Run: pip install esptool"
    exit 1
}
Write-Host "       esptool found" -ForegroundColor Green


# ── Set firmware path based on board ─────
Write-Host "[2/4] Locating firmware binary..." -ForegroundColor Yellow

if ($Board -eq "left") {
    $FirmwarePath = "firmware\esp32_cam_left\.pio\build\esp32cam\firmware.bin"
    $BoardLabel   = "LEFT camera board"
} else {
    $FirmwarePath = "firmware\esp32_cam_right\.pio\build\esp32cam\firmware.bin"
    $BoardLabel   = "RIGHT camera board"
}

if (-not (Test-Path $FirmwarePath)) {
    Write-Host "[ERROR] Firmware binary not found at:" -ForegroundColor Red
    Write-Host "        $FirmwarePath"
    Write-Host ""
    Write-Host "        Build the firmware first in PlatformIO:"
    Write-Host "        PlatformIO sidebar → Build"
    exit 1
}
Write-Host "       Found: $FirmwarePath" -ForegroundColor Green


# ── Erase flash first ─────────────────────
Write-Host "[3/4] Erasing ESP32 flash on $Port..." -ForegroundColor Yellow

esptool.py --port $Port --baud 921600 erase_flash
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Erase failed. Check that:" -ForegroundColor Red
    Write-Host "        - The ESP32 is plugged in"
    Write-Host "        - $Port is the correct COM port"
    Write-Host "        - No other program is using $Port"
    exit 1
}
Write-Host "       Flash erased" -ForegroundColor Green


# ── Flash firmware ────────────────────────
Write-Host "[4/4] Flashing $BoardLabel..." -ForegroundColor Yellow

esptool.py `
    --port $Port `
    --baud 921600 `
    --before default_reset `
    --after hard_reset `
    write_flash `
    --flash_mode dio `
    --flash_freq 80m `
    --flash_size detect `
    0x0 $FirmwarePath

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Flash failed." -ForegroundColor Red
    exit 1
}


# ── Done ──────────────────────────────────
Write-Host ""
Write-Host "=== Flash complete! ===" -ForegroundColor Green
Write-Host "    $BoardLabel flashed successfully on $Port" -ForegroundColor White
Write-Host ""
Write-Host "Open Serial Monitor at 115200 baud to see boot log." -ForegroundColor Cyan
Write-Host ""