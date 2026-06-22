# AyuGram Windows Build Script — local build automation
# Usage: Run in "x64 Native Tools Command Prompt for VS 2022"
#   powershell -ExecutionPolicy Bypass -File build-ayugram.ps1
#
# What it does:
#   1. Load VS build environment (vcvars64.bat)
#   2. Add Ninja to PATH
#   3. Clone/update fork
#   4. Prepare dependencies (first time only, ~30 min)
#   5. Configure build with API credentials
#   6. Build Release x64
#   7. Copy binary to output folder

param(
    [string]$BuildPath = "D:\TBuild",
    [string]$Config = "Release",
    [string]$ApiId = "13994409",
    [string]$ApiHash = "9d0860cf26be00e72647bd516e648676",
    [string]$VSPath = "C:\Program Files\Microsoft Visual Studio\2022\Community",
    [switch]$SkipPrepare  # Skip dependency prep (after first successful build)
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AyuGram Windows Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "BuildPath: $BuildPath"
Write-Host "Config:    $Config"
Write-Host ""

# Step 0: Load VS build environment (vcvars64.bat)
Write-Host "[0/5] Load VS 2022 build environment..." -ForegroundColor Yellow

if (-not (Test-Path $VSPath)) {
    # Try Enterprise/Professional editions
    $altPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
    )
    foreach ($alt in $altPaths) {
        if (Test-Path $alt) { $VSPath = $alt; break }
    }
}

$vcvarsPath = Join-Path $VSPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvarsPath)) {
    throw "vcvars64.bat not found at $vcvarsPath. Install VS 2022 with C++ workload."
}

Write-Host "  Loading from: $vcvarsPath"

# Run vcvars64.bat and capture environment
$envOutput = & cmd /c "`"$vcvarsPath`" >nul 2>&1 && set"
foreach ($line in $envOutput) {
    if ($line -match "^([^=]+)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

# Verify cl.exe is available
$clVersion = & cl 2>&1 | Select-Object -First 1
if ($clVersion -notmatch "Microsoft.*C/C\+\+") {
    throw "cl.exe not available after vcvars64.bat. VS install may be incomplete."
}
Write-Host "  cl.exe: $clVersion" -ForegroundColor Green

# Step 0b: Add Ninja to PATH (bundled with VS CMake)
$ninjaPaths = @(
    (Join-Path $VSPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"),
    "C:\Program Files\CMake\bin",
    "C:\ninja"
)
foreach ($ninjaPath in $ninjaPaths) {
    if (Test-Path (Join-Path $ninjaPath "ninja.exe")) {
        $currentPath = [System.Environment]::GetEnvironmentVariable("Path", "Process")
        if ($currentPath -notlike "*$ninjaPath*") {
            [System.Environment]::SetEnvironmentVariable("Path", "$ninjaPath;$currentPath", "Process")
        }
        Write-Host "  Ninja found: $ninjaPath" -ForegroundColor Green
        break
    }
}

# Verify ninja
$ninjaVersion = & ninja --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ninja.exe: v$ninjaVersion" -ForegroundColor Green
} else {
    Write-Host "  WARNING: ninja.exe not found. Install Ninja or add to PATH." -ForegroundColor Yellow
    Write-Host "  Download: https://github.com/ninja-build/ninja/releases" -ForegroundColor Yellow
}

# Verify cmake
$cmakeVersion = & cmake --version 2>&1 | Select-Object -First 1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  $cmakeVersion" -ForegroundColor Green
} else {
    throw "cmake not found. Add CMake to PATH."
}

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
