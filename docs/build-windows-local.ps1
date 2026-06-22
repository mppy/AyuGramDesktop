# AyuGram Windows Build Script — local build automation
# Usage: Run in "x64 Native Tools Command Prompt for VS 2022"
#   powershell -ExecutionPolicy Bypass -File build-ayugram.ps1
#
# What it does:
#   1. Clone/update fork
#   2. Prepare dependencies (first time only, ~30 min)
#   3. Configure build with API credentials
#   4. Build Release x64
#   5. Copy binary to output folder

param(
    [string]$BuildPath = "D:\TBuild",
    [string]$Config = "Release",
    [string]$ApiId = "13994409",
    [string]$ApiHash = "9d0860cf26be00e72647bd516e648676",
    [switch]$SkipPrepare  # Skip dependency prep (after first successful build)
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AyuGram Windows Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "BuildPath: $BuildPath"
Write-Host "Config:    $Config"
Write-Host ""

# Step 1: Clone or update fork
Write-Host "[1/5] Clone/update fork..." -ForegroundColor Yellow
$tdesktopPath = Join-Path $BuildPath "tdesktop"

if (-not (Test-Path $tdesktopPath)) {
    New-Item -ItemType Directory -Path $BuildPath -Force | Out-Null
    Set-Location $BuildPath
    git clone --recursive https://github.com/mppy/AyuGramDesktop.git tdesktop
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
} else {
    Write-Host "  Fork exists, pulling latest..."
    Set-Location $tdesktopPath
    git pull origin dev
    if ($LASTEXITCODE -ne 0) { throw "git pull failed" }
    git submodule update --init --recursive
}

Set-Location $tdesktopPath
Write-Host "  OK" -ForegroundColor Green

# Step 2: Prepare dependencies (skip if flag set)
if (-not $SkipPrepare) {
    Write-Host "[2/5] Prepare dependencies (first time ~30 min)..." -ForegroundColor Yellow
    & "Telegram\build\prepare\win.bat"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Prepare failed. If 'IP not allowed' error, enable VPN and retry." -ForegroundColor Red
        throw "prepare.bat failed"
    }
    Write-Host "  OK" -ForegroundColor Green
} else {
    Write-Host "[2/5] Skipped dependency prep (-SkipPrepare)" -ForegroundColor DarkGray
}

# Step 3: Configure build
Write-Host "[3/5] Configure build..." -ForegroundColor Yellow
Set-Location "Telegram"
& .\configure.bat x64 -D TDESKTOP_API_ID=$ApiId -D TDESKTOP_API_HASH=$ApiHash
if ($LASTEXITCODE -ne 0) { throw "configure.bat failed" }
Write-Host "  OK" -ForegroundColor Green

# Step 4: Build
Write-Host "[4/5] Build $Config x64 (20-40 min)..." -ForegroundColor Yellow
$startTime = Get-Date

& msbuild "..\out\Telegram.slnx" /p:Configuration=$Config /p:Platform=x64 /m /v:m
if ($LASTEXITCODE -ne 0) { throw "msbuild failed" }

$duration = (Get-Date) - $startTime
Write-Host "  Build completed in $([math]::Round($duration.TotalMinutes, 1)) minutes" -ForegroundColor Green

# Step 5: Copy binary
Write-Host "[5/5] Copy binary..." -ForegroundColor Yellow
$binaryPath = "..\out\$Config\AyuGram.exe"
if (-not (Test-Path $binaryPath)) {
    throw "AyuGram.exe not found at $binaryPath"
}

$outputDir = Join-Path $BuildPath "output"
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd-HHmm"
$outputFile = Join-Path $outputDir "AyuGram-$Config-$timestamp.exe"
Copy-Item $binaryPath -Destination $outputFile

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  BUILD SUCCESS!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Binary: $outputFile"
Write-Host "Size:   $([math]::Round((Get-Item $outputFile).Length / 1MB, 1)) MB"
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Run AyuGram.exe, login, test features"
Write-Host "  2. If issues, report to https://github.com/mppy/AyuGramDesktop/issues"
