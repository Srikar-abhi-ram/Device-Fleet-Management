# Quick Start Guide

This guide will help you get the Device Fleet Management Service up and running quickly.

## Prerequisites Check

### Backend (C++)
```bash
# Check compiler
g++ --version  # or clang++ --version

# Check CMake
cmake --version  # Should be 3.15+

# Check protoc
protoc --version  # Should be 3.0+

# Check gRPC (may need to install)
pkg-config --modversion grpc++  # Should show version
```

If any of these are missing, install them as follows.

#### On macOS (Homebrew)

```bash
# Xcode Command Line Tools (compiler, headers, etc.)
xcode-select --install

# CMake
brew install cmake

# Protocol Buffers compiler (protoc)
brew install protobuf

# gRPC and grpc++
brew install grpc

# pkg-config (used by some build setups)
brew install pkg-config
```

### CLI (Python)
```bash
# Check Python
python3 --version  # Should be 3.7+

# Check pip
pip3 --version
```

## 5-Minute Setup

### Step 1: Build Backend (2 minutes)

```bash
cd backend
./build.sh
```
If Permissions are denied run the below command:
```bash
cd backend
chmod +x build.sh
```
If build succeeds, you'll see:
```
Build completed successfully!
Executable location: build/bin/device_management_server
```

### Step 2: Start Server

```bash
cd backend/build/bin
./device_management_server
```

You should see:
```
========================================
Device Fleet Management Service
========================================
Server listening on 0.0.0.0:50051
Press Ctrl+C to shutdown
========================================
```

**Keep this terminal open!** Open a new terminal for the CLI.

### Step 3: Setup CLI (1 minute)

```bash
cd cli
./setup.sh
```

If facing issue regarding permissions run : 
```bash
cd cli
chmod +x setup.sh 
chmod +x generate_grpc_code.sh
```
This installs dependencies and generates gRPC code.

### Step 4: Test the System

```bash
# Start the interactive CLI
python3 device_cli.py
```

## Common Commands

```bash
# Inside the CLI, type commands like:

# Register device
register <device_id> --name "My Device" --type "router" --status IDLE

# List all devices
list

# Get device information
get-info <device_id>

# Set device status
set-status <device_id> <STATUS>

# Initiate action
initiate-action <device_id> SOFTWARE_UPDATE --params version=2.1.0

# Check action status
action-status <action_id>

# Poll action until completion
poll-action <action_id> --interval 2
```

## Next Steps

- Read the full [README.md](README.md) for detailed documentation
- Explore the code structure
- Try all API operations
- Review the architecture section
