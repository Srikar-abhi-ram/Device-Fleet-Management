#!/bin/bash

# Build script for Device Fleet Management Service
# This script sets up the build directory and compiles the C++ backend

set -e  # Exit on error

echo "========================================="
echo "Building Device Fleet Management Service"
echo "========================================="

# Create build directory
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    echo "Created build directory: $BUILD_DIR"
fi

# Change to build directory
cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo ""
echo "Building the project..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "========================================="
echo "Build completed successfully!"
echo "========================================="
echo "Executable location: $BUILD_DIR/bin/device_management_server"
echo ""
echo "To run the server:"
echo "  cd $BUILD_DIR/bin"
echo "  ./device_management_server [--port PORT]"
echo ""

