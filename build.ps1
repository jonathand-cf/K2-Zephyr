# build.ps1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Write-Host "Setting up Zephyr environment..."
Set-Location "$HOME\zephyrproject"

# Activate venv (PowerShell)
if (Test-Path ~/zephyrproject/.venv/bin/Activate.ps1) {
  ~/zephyrproject/.venv/bin/Activate.ps1
} else {
  Write-Host "Virtualenv activation script not found. "
  exit 1
}

Write-Host "Building K2-Zephyr project..."
Set-Location "$HOME\zephyrproject\K2-Zephyr"
west build -p -b nucleo_f767zi

Write-Host "Build complete! Flash with: west flash"