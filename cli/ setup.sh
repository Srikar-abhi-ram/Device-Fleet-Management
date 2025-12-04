#!/bin/bash

# Setup script for Python CLI
# This installs dependencies and generates gRPC code

set -e  # Exit on error

echo "========================================="
echo "Setting up Device Fleet Management CLI"
echo "========================================="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed"
    exit 1
fi

echo "Python version: $(python3 --version)"
echo ""

# Install Python dependencies
echo "Installing Python dependencies..."
pip3 install -r requirements.txt

echo ""

# Generate gRPC code
echo "Generating gRPC code from .proto file..."
./generate_grpc_code.sh

echo ""
echo "========================================="
echo "Setup completed successfully!"
echo "========================================="
echo ""
echo "You can now use the CLI:"
echo "  python3 device_cli.py --help"
echo ""
