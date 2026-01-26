#!/bin/bash
# Test runner script for ECU08 NSIL (Linux/macOS)
# Usage: ./run_tests.sh [unit|sil|all]

set -e

TEST_TYPE="${1:-unit}"  # Default: unit
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║       ECU08 NSIL - Test Build & Execution (CMake)             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "▶ Build type: $TEST_TYPE"
echo "  - Project root: $PROJECT_ROOT"
echo "  - Build dir: $BUILD_DIR"
echo ""

# Configure CMake
echo "▶ Configuring with CMake..."

case "$TEST_TYPE" in
    unit)
        cmake -B . -S "$PROJECT_ROOT" -DBUILD_UNIT_TESTS=ON -DBUILD_SIL_TESTS=OFF
        ;;
    sil)
        cmake -B . -S "$PROJECT_ROOT" -DBUILD_UNIT_TESTS=OFF -DBUILD_SIL_TESTS=ON
        ;;
    all)
        cmake -B . -S "$PROJECT_ROOT" -DBUILD_UNIT_TESTS=ON -DBUILD_SIL_TESTS=ON
        ;;
    *)
        echo "  ✗ Unknown test type: $TEST_TYPE"
        echo "  Valid options: unit, sil, all"
        exit 1
        ;;
esac

echo "  ✓ CMake configuration successful"
echo ""

# Build
echo "▶ Building tests..."
cmake --build .
echo "  ✓ Build successful"
echo ""

# Run tests
echo "▶ Running tests..."
echo ""

ctest --output-on-failure

if [ $? -eq 0 ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                    ALL TESTS PASSED ✓                          ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    exit 0
else
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                  TESTS FAILED ✗                               ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    exit 1
fi

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
