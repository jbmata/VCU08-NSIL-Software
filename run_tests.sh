#!/bin/bash
# Test runner script for ECU08 NSIL
# Usage: ./run_tests.sh

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$PROJECT_ROOT/Core/Src"
INC_DIR="$PROJECT_ROOT/Core/Inc"
BUILD_DIR="$PROJECT_ROOT/build_test"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║          ECU08 NSIL - Unit Test Build & Execution             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "▶ Compiling tests..."
echo "  - Source: $TEST_DIR"
echo "  - Include: $INC_DIR"
echo ""

# Check if arm-none-eabi-gcc is available
if command -v arm-none-eabi-gcc &> /dev/null; then
    echo "  Using ARM cross-compiler (arm-none-eabi-gcc)"
    COMPILER="arm-none-eabi-gcc"
else
    echo "  Using native GCC (gcc)"
    COMPILER="gcc"
fi

# Compile with common math library
$COMPILER \
    -I"$INC_DIR" \
    -std=c99 \
    -Wall -Wextra \
    -g -O2 \
    "$TEST_DIR/test_control.c" \
    "$TEST_DIR/control.c" \
    "$TEST_DIR/app_state.c" \
    "$TEST_DIR/can.c" \
    -o test_control \
    -lm

echo "  ✓ Compilation successful"
echo ""

echo "▶ Running tests..."
echo ""

./test_control

TEST_RESULT=$?

echo ""
if [ $TEST_RESULT -eq 0 ]; then
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                    ALL TESTS PASSED ✓                          ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    exit 0
else
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                  TESTS FAILED ✗                               ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    exit 1
fi
