# EchoNav - Environment Setup
# Run this ONCE to install everything needed
# How to run:
# 1. Open PowerShell as Administrator
# 2. Navigate to the project folder
# 3. Run: .\scripts\setup_env.ps1

Write-Host ""
Write-Host "=== EchoNav Environment Setup ===" -ForegroundColor Cyan
Write-Host ""

# Check Python is installed
Write-Host "[1/5] Checking Python..." -ForegroundColor Yellow

$pythonVersion = python --version 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Python not found." -ForegroundColor Red
    Write-Host "        Download from https://python.org and re-run this script."
    exit 1
}
Write-Host "       Found: $pythonVersion" -ForegroundColor Green

# Create virtual environment
Write-Host "[2/5] Creating virtual environment..." -ForegroundColor Yellow

if (Test-Path "venv") {
    Write-Host "       venv already exists - skipping creation" -ForegroundColor Gray
} else {
    python -m venv venv
    Write-Host "       Created venv/" -ForegroundColor Green
}

# Activate virtual environment
Write-Host "[3/5] Activating virtual environment..." -ForegroundColor Yellow
& ".\venv\Scripts\Activate.ps1"

# Upgrade pip
Write-Host "[4/5] Upgrading pip..." -ForegroundColor Yellow
python -m pip install --upgrade pip --quiet

# Install all dependencies
Write-Host "[5/5] Installing dependencies..." -ForegroundColor Yellow

$packages = @(
    "opencv-python",
    "numpy",
    "ultralytics",
    "matplotlib",
    "pandas",
    "pyserial",
    "bleak",
    "pyttsx3",
    "openai-whisper"
)

foreach ($pkg in $packages) {
    Write-Host "       Installing $pkg..." -ForegroundColor Gray
    pip install $pkg --quiet
    if ($LASTEXITCODE -ne 0) {
        Write-Host "       [WARN] Failed to install $pkg" -ForegroundColor Red
    }
}

# Done
Write-Host ""
Write-Host "=== Setup complete! ===" -ForegroundColor Green
Write-Host ""
Write-Host "To activate the environment next time, run:" -ForegroundColor Cyan
Write-Host "  .\venv\Scripts\Activate.ps1" -ForegroundColor White
Write-Host ""
Write-Host "To run the ML pipeline:" -ForegroundColor Cyan
Write-Host "  python main.py" -ForegroundColor White
Write-Host ""