# PURRGRAM BUILD ENVIRONMENT RESTORE v2 - VS 2026
# Chay SAU khi: (1) cai lai Windows, (2) cai VS 2026 Community + v145 toolset + Win SDK
# Cu phap: powershell -ExecutionPolicy Bypass -File build-env-restore.ps1
#
# Script tu:
#   1. Detect VS 2026 + v145 (hoac v143 fallback) + Win SDK
#   2. Verify CMake 4.x co san (tich hop VS)
#   3. Add ThirdParty tools (perl, git, python, jom) vao PATH
#   4. Regenerate CMakeCache (VS 2026 generator, x64, v145)
#   5. Verify: cmake configure thanh cong
#   6. Huong dan build tiep theo

$ErrorActionPreference = "Stop"

$BuildPath = "D:\TBuild"
$Tdesktop = "$BuildPath\tdesktop"
$ThirdParty = "$BuildPath\ThirdParty"
$OutDir = "$Tdesktop\out"

Write-Host "=== PurrGram Build Env Restore v2 (VS 2026) ===" -ForegroundColor Cyan
Write-Host "BuildPath: $BuildPath"
Write-Host ""

# ========== 1. KIEM TRA VS 2026 ==========
Write-Host "[1/6] Kiem tra Visual Studio 2026..." -ForegroundColor Yellow

$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Host "  LOI: Khong tim thay VS Installer (vswhere.exe)" -ForegroundColor Red
    Write-Host "  Cai VS 2026 Community tu: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
    Write-Host "  Components can chon:" -ForegroundColor Yellow
    Write-Host "    - Desktop development with C++" -ForegroundColor White
    Write-Host "    - MSVC v145 - VS 2026 C++ x64/x86 build tools (default)" -ForegroundColor White
    Write-Host "    - Windows 10 SDK 10.0.26100.0 (hoac SDK moi hon)" -ForegroundColor White
    Write-Host "    - C++ CMake tools for Windows" -ForegroundColor White
    Write-Host "    - [OPTIONAL] MSVC v143 - VS 2022 C++ build tools (fallback)" -ForegroundColor White
    exit 1
}

# Tim VS 2026 (version 18.x)
$vsPath = & $vsWhere -latest -version "[18.0,19.0)" -property installationPath 2>$null
if (-not $vsPath) {
    # Fallback: thu VS 2022
    $vsPath = & $vsWhere -latest -version "[17.0,18.0)" -property installationPath 2>$null
    if ($vsPath) {
        Write-Host "  [WARN] Khong tim thay VS 2026, co VS 2022 tai $vsPath" -ForegroundColor Yellow
        Write-Host "         Script se chay voi VS 2022 + v143 toolset" -ForegroundColor Yellow
        $vsMajor = "2022"
        $toolset = "v143"
    } else {
        Write-Host "  LOI: Khong tim thay VS 2026 hoac VS 2022" -ForegroundColor Red
        Write-Host "  Cai VS 2026 Community tu: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "  [OK] VS 2026: $vsPath" -ForegroundColor Green
    $vsMajor = "2026"
    $toolset = "v145"
}
Write-Host "  Toolset mac dinh: $toolset" -ForegroundColor Green

# Kiem tra toolset
$vcToolsDir = Join-Path $vsPath "VC\Tools\MSVC"
$toolsetPattern = if ($toolset -eq "v145") { "14.5*" } else { "14.3*" }
$toolsetDirs = Get-ChildItem $vcToolsDir -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like $toolsetPattern }
if (-not $toolsetDirs) {
    Write-Host "  LOI: $toolset toolset khong co." -ForegroundColor Red
    if ($toolset -eq "v145") {
        Write-Host "  Chon 'MSVC v145 - VS 2026 C++ x64/x86 build tools' trong VS Installer." -ForegroundColor Yellow
    } else {
        Write-Host "  Chon 'MSVC v143 - VS 2022 C++ build tools' trong VS Installer." -ForegroundColor Yellow
    }
    exit 1
}
Write-Host "  [OK] $toolset toolset: $($toolsetDirs[0].Name)" -ForegroundColor Green

# Kiem tra v143 fallback (optional)
$v143dirs = Get-ChildItem $vcToolsDir -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "14.3*" }
if ($v143dirs) {
    Write-Host "  [OK] v143 fallback: $($v143dirs[0].Name) (co)" -ForegroundColor DarkGray
}

# ========== 2. KIEM TRA WINDOWS SDK ==========
Write-Host ""
Write-Host "[2/6] Kiem tra Windows SDK..." -ForegroundColor Yellow

$winSdkRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\Include"
$sdkVersions = @()
if (Test-Path $winSdkRoot) {
    $sdkVersions = Get-ChildItem $winSdkRoot -Directory -ErrorAction SilentlyContinue |
                   Where-Object { $_.Name -like "10.0.26100*" } |
                   ForEach-Object { $_.Name }
}

if ($sdkVersions.Count -eq 0) {
    $anySdk = Get-ChildItem $winSdkRoot -Directory -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -like "10.0.*" } |
              Sort-Object Name -Descending | Select-Object -First 1
    if ($anySdk) {
        Write-Host "  [WARN] Khong co SDK 10.0.26100, co SDK $($anySdk.Name)" -ForegroundColor Yellow
        Write-Host "         PurrGram co the build duoc voi SDK moi hon" -ForegroundColor Yellow
        $sdkVersion = $anySdk.Name
    } else {
        Write-Host "  LOI: Khong co Windows 10 SDK nao." -ForegroundColor Red
        Write-Host "  Cai qua VS Installer: 'Windows 10 SDK 10.0.26100.0'" -ForegroundColor Yellow
        exit 1
    }
} else {
    $sdkVersion = $sdkVersions[0]
    Write-Host "  [OK] Windows SDK: $sdkVersion" -ForegroundColor Green
}

# ========== 3. KIEM TRA CMAKE ==========
Write-Host ""
Write-Host "[3/6] Kiem tra CMake..." -ForegroundColor Yellow

$cmakePaths = @(
    "$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "${env:ProgramFiles}\CMake\bin\cmake.exe",
    "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe"
)
$cmakeExe = $null
foreach ($p in $cmakePaths) {
    if (Test-Path $p) {
        $cmakeExe = $p
        break
    }
}

if (-not $cmakeExe) {
    Write-Host "  LOI: Khong tim thay CMake. Cai qua VS Installer: 'C++ CMake tools for Windows'" -ForegroundColor Red
    exit 1
}
$cmakeVersion = & $cmakeExe --version 2>$null | Select-Object -First 1
Write-Host "  [OK] CMake: $cmakeVersion" -ForegroundColor Green
Write-Host "  Path: $cmakeExe" -ForegroundColor DarkGray

# ========== 4. ADD THIRD PARTY TOOLS VAO PATH ==========
Write-Host ""
Write-Host "[4/6] Add ThirdParty tools vao PATH..." -ForegroundColor Yellow

$toolsToAdd = @(
    "$ThirdParty\python",
    "$ThirdParty\jom",
    "$ThirdParty\msys64\usr\bin",
    "$ThirdParty\gyp",
    "$ThirdParty\NuGet"
)

$gitDir = Get-ChildItem "$ThirdParty" -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "git*" } | Select-Object -First 1
if ($gitDir) {
    $toolsToAdd += "$($gitDir.FullName)\cmd"
    $toolsToAdd += "$($gitDir.FullName)\mingw64\bin"
}

foreach ($toolPath in $toolsToAdd) {
    if (Test-Path $toolPath) {
        $env:PATH = "$toolPath;$env:PATH"
        Write-Host "  [OK] + $toolPath" -ForegroundColor Green
    } else {
        Write-Host "  [SKIP] $toolPath khong ton tai" -ForegroundColor DarkGray
    }
}

# Verify tools
$tools = @("python", "perl", "git", "jom")
foreach ($t in $tools) {
    $found = Get-Command $t -ErrorAction SilentlyContinue
    if ($found) {
        Write-Host "  [OK] $t : $($found.Source)" -ForegroundColor Green
    } else {
        Write-Host "  [WARN] $t khong tim thay trong PATH" -ForegroundColor Yellow
    }
}

# ========== 5. REGENERATE CMAKECACHE ==========
Write-Host ""
Write-Host "[5/6] Regenerate CMakeCache (VS $vsMajor, x64, $toolset)..." -ForegroundColor Yellow

if (-not (Test-Path $Tdesktop)) {
    Write-Host "  LOI: $Tdesktop khong ton tai. Source code PurrGram mat!" -ForegroundColor Red
    exit 1
}

# Backup CMakeCache cu
if (Test-Path "$OutDir\CMakeCache.txt") {
    $oldGen = (Get-Content "$OutDir\CMakeCache.txt" | Where-Object { $_ -match "CMAKE_GENERATOR:INTERNAL" } -ErrorAction SilentlyContinue) -replace "CMAKE_GENERATOR:INTERNAL=", ""
    Copy-Item "$OutDir\CMakeCache.txt" "$OutDir\CMakeCache.backup.txt" -Force
    Write-Host "  [OK] Backup CMakeCache cu (generator: $oldGen) -> CMakeCache.backup.txt" -ForegroundColor Green
}

# Xoa CMakeCache + CMakeFiles de regenerate sach
$cleanItems = @("CMakeCache.txt", "CMakeFiles", "cmake_install.cmake")
foreach ($item in $cleanItems) {
    $p = Join-Path $OutDir $item
    if (Test-Path $p) {
        Remove-Item $p -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "  Dang configure..." -ForegroundColor Yellow

$vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Host "  LOI: Khong tim thay vcvars64.bat tai $vcvars" -ForegroundColor Red
    exit 1
}

$generator = if ($vsMajor -eq "2026") { "Visual Studio 18 2026" } else { "Visual Studio 17 2022" }

$batScript = @"
@echo OFF
call "$vcvars" >nul 2>&1
cd /d "$OutDir"
"$cmakeExe" "$Tdesktop" -G "$generator" -A x64 -T $toolset -D TDESKTOP_API_ID=13994409 -D TDESKTOP_API_HASH=9d0860cf26be00e72647bd516e648676
exit /b %errorlevel%
"@

$batPath = "$env:TEMP\purr-cmake-configure.bat"
Set-Content -Path $batPath -Value $batScript -Encoding ASCII

$configResult = & cmd /c $batPath 2>&1
$configLog = "$BuildPath\purr-cmake-configure.log"
Set-Content -Path $configLog -Value $configResult -Encoding UTF8

if ($LASTEXITCODE -ne 0) {
    Write-Host "  LOI: CMake configure fail! Exit $LASTEXITCODE" -ForegroundColor Red
    Write-Host "  Xem log: $configLog" -ForegroundColor Yellow
    Write-Host "  Tail 20 dong:" -ForegroundColor Yellow
    Get-Content $configLog -Tail 20 | ForEach-Object { Write-Host "    $_" }
    Remove-Item $batPath -Force -ErrorAction SilentlyContinue
    exit $LASTEXITCODE
}

Remove-Item $batPath -Force -ErrorAction SilentlyContinue
Write-Host "  [OK] CMake configure thanh cong!" -ForegroundColor Green

$newGen = Get-Content "$OutDir\CMakeCache.txt" | Where-Object { $_ -match "CMAKE_GENERATOR:INTERNAL" }
$newToolset = Get-Content "$OutDir\CMakeCache.txt" | Where-Object { $_ -match "CMAKE_GENERATOR_TOOLSET:INTERNAL" }
Write-Host "  $newGen" -ForegroundColor DarkGray
Write-Host "  $newToolset" -ForegroundColor DarkGray

# ========== 6. HUONG DAN BUILD ==========
Write-Host ""
Write-Host "[6/6] Build Env Restore HOAN THANH!" -ForegroundColor Green
Write-Host ""
Write-Host "Cach build tiep theo:" -ForegroundColor Cyan
Write-Host "  1. Mo 'x64 Native Tools Command Prompt for VS $vsMajor.bat'" -ForegroundColor White
Write-Host "     (Start Menu > Visual Studio $vsMajor)" -ForegroundColor DarkGray
Write-Host "  2. cd /d D:\TBuild\tdesktop\out" -ForegroundColor White
Write-Host "  3. cmake --build . --config Release --target Telegram" -ForegroundColor White
Write-Host "     (incremental ~12 min, full rebuild ~60 min)" -ForegroundColor DarkGray
Write-Host ""
Write-Host "Luu y:" -ForegroundColor Cyan
Write-Host "  - Toolset $toolset -> full rebuild lan dau (vi doi tu $oldGen)" -ForegroundColor Yellow
Write-Host "  - PurrGram.exe dang chay phai TAT truoc khi build" -ForegroundColor Yellow
Write-Host "  - Neu link.exe CPU flat -> kill + rebuild" -ForegroundColor Yellow
Write-Host ""
Write-Host "Log: $configLog" -ForegroundColor DarkGray
