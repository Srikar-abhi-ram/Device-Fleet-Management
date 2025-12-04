#!/bin/bash

# Script to generate Python gRPC code from Protocol Buffers definition
# This must be run before using the CLI

set -e  # Exit on error

echo "========================================="
echo "Generating Python gRPC Code"
echo "========================================="

# Check if grpcio-tools is installed
if ! python3 -c "import grpc_tools" 2>/dev/null; then
    echo "Error: grpcio-tools is not installed."
    echo "Please run: pip install -r requirements.txt"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
PROTO_DIR="$PROJECT_ROOT/proto"
CLI_DIR="$SCRIPT_DIR"

# Check if proto file exists
if [ ! -f "$PROTO_DIR/device_management.proto" ]; then
    echo "Error: Proto file not found at $PROTO_DIR/device_management.proto"
    exit 1
fi

# Create output directory if it doesn't exist
OUTPUT_DIR="$CLI_DIR/generated"
mkdir -p "$OUTPUT_DIR"

echo "Proto file: $PROTO_DIR/device_management.proto"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Generate Python code from .proto file
python3 -m grpc_tools.protoc \
    --proto_path="$PROTO_DIR" \
    --python_out="$OUTPUT_DIR" \
    --grpc_python_out="$OUTPUT_DIR" \
    "$PROTO_DIR/device_management.proto"

# Rename generated files to match import statements in device_cli.py
if [ -f "$OUTPUT_DIR/device_management_pb2.py" ] && [ -f "$OUTPUT_DIR/device_management_pb2_grpc.py" ]; then
    echo "✓ Generated files:"
    echo "  - $OUTPUT_DIR/device_management_pb2.py"
    echo "  - $OUTPUT_DIR/device_management_pb2_grpc.py"
    echo ""
    echo "========================================="
    echo "Generation completed successfully!"
    echo "========================================="
else
    echo "Error: Generated files not found"
    exit 1
fi
