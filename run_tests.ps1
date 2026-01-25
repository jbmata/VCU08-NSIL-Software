# Test runner script for ECU08 NSIL (Windows PowerShell)
# Usage: powershell -ExecutionPolicy Bypass -File run_tests.ps1

param(
    [string]$Compiler = "auto",  # "auto", "gcc", or "arm-none-eabi-gcc"
    [switch]$Verbose = $false
)

$ErrorActionPreference = "Stop"

$PROJECT_ROOT = Split-Path -Parent $MyInvocation.MyCommand.Path
$TEST_DIR = Join-Path $PROJECT_ROOT "Core\Src"
$INC_DIR = Join-Path $PROJECT_ROOT "Core\Inc"
$BUILD_DIR = Join-Path $PROJECT_ROOT "build_test"

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║          ECU08 NSIL - Unit Test Build & Execution             ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Create build directory
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
}

Write-Host "▶ Compiling tests..." -ForegroundColor Yellow
Write-Host "  - Project root: $PROJECT_ROOT"
Write-Host "  - Source dir: $TEST_DIR"
Write-Host "  - Include dir: $INC_DIR"
Write-Host ""

# Determine compiler
$CompilerPath = $null
$CompilerName = ""

if ($Compiler -eq "auto" -or $Compiler -eq "") {
    # Try ARM compiler first
    $CompilerPath = (Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue).Path
    if ($CompilerPath) {
        $CompilerName = "arm-none-eabi-gcc"
    } else {
        # Fall back to native gcc
        $CompilerPath = (Get-Command gcc -ErrorAction SilentlyContinue).Path
        if ($CompilerPath) {
            $CompilerName = "gcc"
        }
    }
} else {
    $CompilerPath = (Get-Command $Compiler -ErrorAction SilentlyContinue).Path
    if ($CompilerPath) {
        $CompilerName = $Compiler
    }
}

if (-not $CompilerPath) {
    Write-Host "  ✗ ERROR: No C compiler found!" -ForegroundColor Red
    Write-Host "    - Install gcc (MinGW) or ARM cross-compiler"
    Write-Host "    - Or use STM32CubeIDE's built-in tools"
    exit 1
}

Write-Host "  Using compiler: $CompilerName" -ForegroundColor Green
Write-Host ""

# Compile
$TestExe = Join-Path $BUILD_DIR "test_control.exe"
$CompileArgs = @(
    "-I`"$INC_DIR`"",
    "-std=c99",
    "-Wall", "-Wextra",
    "-g", "-O2",
    "`"$TEST_DIR\test_control.c`"",
    "`"$TEST_DIR\control.c`"",
    "`"$TEST_DIR\app_state.c`"",
    "`"$TEST_DIR\can.c`"",
    "-o", "`"$TestExe`"",
    "-lm"
)

if ($Verbose) {
    Write-Host "  Command: $CompilerName $CompileArgs" -ForegroundColor DarkGray
}

try {
    & $CompilerPath @CompileArgs 2>&1
    Write-Host "  ✓ Compilation successful" -ForegroundColor Green
} catch {
    Write-Host "  ✗ Compilation failed!" -ForegroundColor Red
    Write-Host "  Error: $_"
    exit 1
}

Write-Host ""
Write-Host "▶ Running tests..." -ForegroundColor Yellow
Write-Host ""

# Run tests
& $TestExe
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
