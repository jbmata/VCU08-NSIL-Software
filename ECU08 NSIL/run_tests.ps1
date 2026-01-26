# Test runner script for ECU08 NSIL (Windows PowerShell)
# Usage: powershell -ExecutionPolicy Bypass -File run_tests.ps1

param(
    [string]$TestType = "unit",  # "unit", "sil", or "all"
    [switch]$Verbose = $false
)

$ErrorActionPreference = "Stop"

$PROJECT_ROOT = Split-Path -Parent $MyInvocation.MyCommand.Path
$BUILD_DIR = Join-Path $PROJECT_ROOT "build"

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║       ECU08 NSIL - Test Build & Execution (CMake)              ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Create build directory
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
}

Write-Host "▶ Build type: $TestType" -ForegroundColor Yellow
Write-Host "  - Project root: $PROJECT_ROOT"
Write-Host "  - Build dir: $BUILD_DIR"
Write-Host ""

# Configure CMake
Write-Host "▶ Configuring with CMake..." -ForegroundColor Yellow

$CmakeArgs = @()
if ($TestType -eq "unit") {
    $CmakeArgs = @("-DBUILD_UNIT_TESTS=ON", "-DBUILD_SIL_TESTS=OFF")
} elseif ($TestType -eq "sil") {
    $CmakeArgs = @("-DBUILD_UNIT_TESTS=OFF", "-DBUILD_SIL_TESTS=ON")
} else {
    $CmakeArgs = @("-DBUILD_UNIT_TESTS=ON", "-DBUILD_SIL_TESTS=ON")
}

cd $BUILD_DIR
cmake -B . -S "$PROJECT_ROOT" @CmakeArgs 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ CMake configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host "  ✓ CMake configuration successful" -ForegroundColor Green
Write-Host ""

# Build
Write-Host "▶ Building tests..." -ForegroundColor Yellow
cmake --build . 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "  ✓ Build successful" -ForegroundColor Green
Write-Host ""

# Run tests
Write-Host "▶ Running tests..." -ForegroundColor Yellow
Write-Host ""

ctest --output-on-failure 2>&1
$TestResult = $LASTEXITCODE

Write-Host ""

if ($TestResult -eq 0) {
    Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
    Write-Host "║                    ALL TESTS PASSED ✓                          ║" -ForegroundColor Green
    Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Green
    exit 0
} else {
    Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Red
    Write-Host "║                  TESTS FAILED ✗                               ║" -ForegroundColor Red
    Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Red
    exit 1
}
