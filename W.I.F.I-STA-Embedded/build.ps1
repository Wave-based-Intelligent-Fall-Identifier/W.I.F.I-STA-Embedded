# STA project build helper
# Usage:   .\build.ps1
# Flash:   .\build.ps1 -Flash -Port COM5
# Clean:   .\build.ps1 -Clean
param(
    [switch]$Flash,
    [string]$Port = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$IDF  = "C:\Espressif\frameworks\esp-idf-v5.5.4"
$VENV = "C:\Espressif\python_env\idf5.5_py3.13_env"

Write-Host "[1/2] Activating ESP-IDF environment..." -ForegroundColor Cyan
$env:IDF_PATH       = $IDF
$env:IDF_TOOLS_PATH = "C:\Espressif"
$env:PATH           = "$VENV\Scripts;$env:PATH"
. "$IDF\export.ps1"

Set-Location $PSScriptRoot

if ($Clean) {
    Write-Host "Running fullclean..." -ForegroundColor Yellow
    idf.py fullclean
}

Write-Host "[2/2] Building..." -ForegroundColor Cyan
idf.py build

if ($Flash) {
    if ($Port -eq "") {
        idf.py flash monitor
    } else {
        idf.py -p $Port flash monitor
    }
}
