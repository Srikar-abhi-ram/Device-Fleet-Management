# Device Fleet Management Service

A production-ready, enterprise-level device fleet management system with a C++ gRPC backend and Python CLI client. This service provides comprehensive device registration, status tracking, and asynchronous action management capabilities.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [Building and Running](#building-and-running)
  - [Backend Service (C++)](#backend-service-c)
  - [Python CLI Client](#python-cli-client)
- [API Documentation](#api-documentation)
- [Usage Examples](#usage-examples)
- [Protocol Buffers Definition](#protocol-buffers-definition)
- [Understanding the Code](#understanding-the-code)
- [Assumptions and Simplifications](#assumptions-and-simplifications)
- [Future Improvements](#future-improvements)
- [Troubleshooting](#troubleshooting)

## Overview

This project implements a **Device Fleet Management Service** that allows you to:

- **Register devices** in a fleet with unique identifiers
- **Track device status** (IDLE, BUSY, OFFLINE, MAINTENANCE, UPDATING, RECOVERING, ERROR)
- **Initiate long-running actions** on devices (e.g., software updates)
- **Monitor action progress** asynchronously
- **Query device and action information** in real-time

The system is designed with enterprise-level considerations:
- **Thread-safe** state management
- **Asynchronous action simulation** using background threads
- **Comprehensive error handling** and validation
- **Clean separation of concerns** (business logic, gRPC layer, CLI)
- **Production-ready build system** (CMake)
- **Extensive documentation** and code comments

## Architecture

### High-Level Architecture

```
┌─────────────────┐
│  Python CLI     │  (Frontend)
│  (device_cli.py)│
└────────┬────────┘
         │ gRPC
         │ (Protocol Buffers)
         ▼
┌─────────────────────────────────────┐
│  C++ Backend Service                │
│  ┌───────────────────────────────┐  │
│  │ DeviceManagementServiceImpl   │  │  (gRPC Service Layer)
│  └───────────┬───────────────────┘  │
│              │                       │
│  ┌───────────▼───────────────────┐  │
│  │ DeviceManager                 │  │  (State Management)
│  │ - Thread-safe device storage  │  │
│  │ - Device lifecycle management │  │
│  └───────────┬───────────────────┘  │
│              │                       │
│  ┌───────────▼───────────────────┐  │
│  │ ActionSimulator               │  │  (Async Action Processing)
│  │ - Background thread execution │  │
│  │ - Action status tracking      │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

### Component Breakdown

#### 1. **Protocol Buffers (.proto)**
- Defines the API contract between client and server
- Specifies all message types and service methods
- Language-agnostic interface definition

#### 2. **C++ Backend Service**

**DeviceManager** (`device_manager.h/cpp`):
- In-memory, thread-safe storage for device state
- Uses `std::unordered_map` for O(1) device lookups
- Protected by mutex locks for concurrent access
- Manages device lifecycle (registration, status updates, action associations)

**ActionSimulator** (`device_manager.h/cpp`):
- Simulates long-running device actions asynchronously
- Uses background threads to simulate 10-30 second operations
- Tracks action status (PENDING → RUNNING → COMPLETED/FAILED)
- Updates device status during action execution
- 90% success rate simulation with random failures

**DeviceManagementServiceImpl** (`device_management_service_impl.h/cpp`):
- Implements the gRPC service interface
- Handles request validation and error responses
- Delegates business logic to DeviceManager and ActionSimulator
- Converts between Protocol Buffers messages and internal data structures

**Server** (`server.cpp`):
- Main entry point for the gRPC server
- Configures and starts the server
- Handles graceful shutdown on SIGINT/SIGTERM
- Listens on configurable port (default: 50051)

#### 3. **Python CLI Client**

**DeviceManagementClient** (`device_cli.py`):
- High-level client wrapper for gRPC communication
- Provides convenient methods for each operation
- Handles connection management and error reporting
- Formats output for human readability

**Command-Line Interface**:
- Uses `argparse` for command parsing
- Supports all API operations via subcommands
- Includes polling functionality for action monitoring

### Data Flow Example: Software Update

1. **CLI sends request**: `initiate-action device-001 SOFTWARE_UPDATE --params version=2.1.0`
2. **gRPC layer**: Request validated, converted to internal format
3. **Service layer**: Checks device exists, not busy
4. **ActionSimulator**: Creates action, starts background thread
5. **DeviceManager**: Updates device status to UPDATING, associates action ID
6. **Background thread**: Simulates 10-30 second operation
7. **Completion**: Updates action status, device status (IDLE or ERROR)
8. **CLI polling**: Periodically queries action status until completion

## Prerequisites

### For Backend (C++)

- **C++17 compatible compiler** (GCC 7+, Clang 5+, or MSVC 2017+)
- **CMake 3.15+**
- **Protocol Buffers compiler (protoc) 3.0+**
- **gRPC C++ library** (grpc++)
- **pthread** (usually included with compiler)

**Installation on Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config
sudo apt-get install -y libprotobuf-dev protobuf-compiler
sudo apt-get install -y libgrpc++-dev protobuf-compiler-grpc
```

**Installation on macOS:**
```bash
brew install cmake protobuf grpc
```

### For CLI (Python)

- **Python 3.7+**
- **pip** (Python package manager)

**Installation:**
```bash
# Python 3 is usually pre-installed on Linux/macOS
python3 --version  # Verify installation
```

## Project Structure

```
Fleet Management/
├── proto/
│   └── device_management.proto          # Protocol Buffers API definition
├── backend/
│   ├── src/
│   │   ├── device_manager.h              # Device state management header
│   │   ├── device_manager.cpp            # Device state management implementation
│   │   ├── device_management_service_impl.h  # gRPC service header
│   │   ├── device_management_service_impl.cpp # gRPC service implementation
│   │   └── server.cpp                    # Main server entry point
│   ├── CMakeLists.txt                    # CMake build configuration
│   └── build.sh                          # Build script
├── cli/
│   ├── device_cli.py                     # Python CLI client
│   ├── requirements.txt                  # Python dependencies
│   ├── setup.sh                          # CLI setup script
│   ├── generate_grpc_code.sh            # Script to generate Python gRPC code
│   └── generated/                        # Generated Python gRPC code (created by setup)
│       ├── device_management_pb2.py
│       └── device_management_pb2_grpc.py
└── README.md                             # This file
```

## Building and Running

### Backend Service (C++)

#### Step 1: Build the Project

```bash
cd backend
./build.sh
```

This script will:
1. Create a `build/` directory
2. Configure the project with CMake
3. Compile all source files
4. Generate Protocol Buffers and gRPC code
5. Link everything into an executable

**Manual build (alternative):**
```bash
cd backend
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

#### Step 2: Run the Server

```bash
cd backend/build/bin
./device_management_server
```

**With custom port:**
```bash
./device_management_server --port 50052
```

The server will start and display:
```
========================================
Device Fleet Management Service
========================================
Server listening on 0.0.0.0:50051
Press Ctrl+C to shutdown
========================================
```

**Note**: Keep the server running in one terminal. Use another terminal for the CLI.

### Python CLI Client

#### Step 1: Setup

```bash
cd cli
./setup.sh
```

This script will:
1. Install Python dependencies (grpcio, grpcio-tools, protobuf)
2. Generate Python gRPC code from the .proto file

**Manual setup (alternative):**
```bash
cd cli
pip3 install -r requirements.txt
./generate_grpc_code.sh
```

#### Step 2: Verify Installation

```bash
python3 device_cli.py --help
```

You should see the command help menu.

## API Documentation

### Service Methods

#### 1. RegisterDevice
Registers a new device in the fleet.

**Request:**
- `device_id` (string, required): Unique device identifier
- `device_name` (string, optional): Human-readable name
- `device_type` (string, optional): Device type (e.g., "router", "sensor")
- `initial_status` (enum, optional): Initial status (defaults to IDLE)

**Response:**
- `success` (bool): Whether registration succeeded
- `message` (string): Human-readable message
- `device_id` (string): The registered device ID

#### 2. SetDeviceStatus
Manually updates a device's status.

**Request:**
- `device_id` (string, required): Device identifier
- `status` (enum, required): New status (IDLE, BUSY, OFFLINE, MAINTENANCE, etc.)

**Response:**
- `success` (bool): Whether update succeeded
- `message` (string): Human-readable message
- `previous_status` (enum): Status before update
- `current_status` (enum): Status after update

#### 3. GetDeviceInfo
Retrieves complete information about a device.

**Request:**
- `device_id` (string, required): Device identifier

**Response:**
- `success` (bool): Whether query succeeded
- `message` (string): Human-readable message
- `device_info` (DeviceInfo): Complete device information including:
  - Device ID, name, type
  - Current status
  - Registration and last update timestamps
  - Currently running action ID (if any)

#### 4. InitiateDeviceAction
Starts a long-running action on a device.

**Request:**
- `device_id` (string, required): Device identifier
- `action_type` (enum, required): Type of action (SOFTWARE_UPDATE, etc.)
- `action_params` (map<string, string>, optional): Action-specific parameters

**Response:**
- `success` (bool): Whether initiation succeeded
- `message` (string): Human-readable message
- `action_id` (string): Unique action identifier for tracking
- `action_status` (enum): Initial status (typically RUNNING)

#### 5. GetDeviceActionStatus
Checks the status of a previously initiated action.

**Request:**
- `action_id` (string, required): Action identifier

**Response:**
- `success` (bool): Whether query succeeded
- `message` (string): Human-readable message
- `action_info` (ActionInfo): Complete action information including:
  - Action ID, device ID, action type
  - Current status (PENDING, RUNNING, COMPLETED, FAILED)
  - Initiation and completion timestamps
  - Action parameters
  - Error message (if failed)

#### 6. ListDevices
Lists all registered devices and their statuses.

**Request:**
- (empty)

**Response:**
- `success` (bool): Always true
- `message` (string): Summary message
- `devices` (repeated DeviceInfo): List of all devices

### Device Statuses

- **IDLE**: Device is idle and ready for operations
- **BUSY**: Device is busy with an operation
- **OFFLINE**: Device is offline/unreachable
- **MAINTENANCE**: Device is in maintenance mode
- **UPDATING**: Device is updating software/firmware
- **RECOVERING**: Device is recovering from an error
- **ERROR**: Device is in an error state

### Action Types

- **SOFTWARE_UPDATE**: Software update action
- **FIRMWARE_UPDATE**: Firmware update action
- **SYSTEM_REBOOT**: System reboot action
- **CONFIGURATION_CHANGE**: Configuration change action

### Action Statuses

- **PENDING**: Action is queued but not started
- **RUNNING**: Action is currently executing
- **COMPLETED**: Action completed successfully
- **FAILED**: Action failed

## Usage Examples

### Example 1: Register and List Devices

```bash
# Register a device
python3 device_cli.py register router-001 --name "Main Router" --type "router" --status IDLE

# Register another device
python3 device_cli.py register sensor-001 --name "Temperature Sensor" --type "sensor"

# List all devices
python3 device_cli.py list
```

**Output:**
```
✓ Device 'router-001' registered successfully
✓ Device 'sensor-001' registered successfully

Registered Devices (2 total):
--------------------------------------------------------------------------------
Device ID            Name            Type            Status          Action
--------------------------------------------------------------------------------
router-001          Main Router     router          IDLE            None
sensor-001          Temperature Sensor sensor          IDLE            None
```

### Example 2: Get Device Information

```bash
python3 device_cli.py get-info router-001
```

**Output:**
```
Device Information:
  ID:            router-001
  Name:          Main Router
  Type:          router
  Status:        IDLE
  Registered:    2024-01-15 10:30:00
  Last Updated:  2024-01-15 10:30:00
  Current Action: None
```

### Example 3: Update Device Status

```bash
python3 device_cli.py set-status router-001 MAINTENANCE
```

**Output:**
```
✓ Device 'router-001' status updated:
  Previous: IDLE
  Current:  MAINTENANCE
```

### Example 4: Initiate Software Update

```bash
# Start a software update
python3 device_cli.py initiate-action router-001 SOFTWARE_UPDATE --params version=2.1.0

# Check action status
python3 device_cli.py action-status <action_id>
```

**Output:**
```
✓ Action initiated successfully
  Action ID: action_1705312200000_1
  Status:    RUNNING
```

### Example 5: Poll Action Status

```bash
# Poll until action completes (updates every 2 seconds)
python3 device_cli.py poll-action action_1705312200000_1 --interval 2
```

**Output:**
```
Polling action 'action_1705312200000_1' (interval: 2s)...
Press Ctrl+C to stop polling

[10:30:15] Status: RUNNING
[10:30:17] Status: RUNNING
[10:30:19] Status: RUNNING
...
[10:30:35] Status: COMPLETED
✓ Action completed successfully!
```

### Example 6: Complete Workflow

```bash
# 1. Register a device
python3 device_cli.py register device-001 --name "Test Device" --type "gateway"

# 2. Check device status
python3 device_cli.py get-info device-001

# 3. Initiate software update
ACTION_ID=$(python3 device_cli.py initiate-action device-001 SOFTWARE_UPDATE --params version=2.1.0 | grep "Action ID" | awk '{print $3}')

# 4. Poll action status
python3 device_cli.py poll-action $ACTION_ID

# 5. Verify device status after update
python3 device_cli.py get-info device-001

# 6. List all devices
python3 device_cli.py list
```

## Protocol Buffers Definition

The API is defined in `proto/device_management.proto`. This file specifies:

- **Service interface**: All RPC methods
- **Message types**: Request/response structures
- **Enumerations**: Device statuses, action types, action statuses

**Key Design Decisions:**
- Used `proto3` syntax for modern Protocol Buffers features
- All fields are optional by default (proto3 behavior)
- Used enums for type-safe status values
- Included comprehensive field documentation

**Generating Code:**
- **C++**: Automatically generated during CMake build
- **Python**: Generated by `cli/generate_grpc_code.sh`

## Understanding the Code

### Why gRPC?

**gRPC** (gRPC Remote Procedure Calls) is a modern, high-performance RPC framework:
- **Language-agnostic**: Works with C++, Python, Java, Go, etc.
- **Efficient**: Uses Protocol Buffers for binary serialization (faster than JSON)
- **Type-safe**: Strong typing through .proto definitions
- **Streaming support**: Can handle bidirectional streaming (not used in this project)
- **Industry standard**: Used by Google, Netflix, and many others

### Why Protocol Buffers?

**Protocol Buffers** (protobuf) is a language-neutral data serialization format:
- **Compact**: Binary format is smaller than JSON/XML
- **Fast**: Faster serialization/deserialization
- **Schema evolution**: Can add fields without breaking compatibility
- **Code generation**: Automatically generates client/server code

### Thread Safety in DeviceManager

The `DeviceManager` class uses **mutex locks** to ensure thread safety:

```cpp
std::lock_guard<std::mutex> lock(devices_mutex_);
// Critical section: access devices_ map
```

**Why needed?**
- Multiple gRPC requests can arrive concurrently
- Each request runs in a separate thread
- Without locks, concurrent map access could cause data corruption or crashes

**`std::lock_guard`** automatically locks on construction and unlocks on destruction (RAII pattern).

### Asynchronous Action Simulation

Actions are simulated in **background threads**:

```cpp
raw_ptr->simulation_thread = std::thread([this, ...]() {
    this->SimulateAction(...);
});
```

**Why threads?**
- Actions take 10-30 seconds to complete
- If synchronous, the gRPC call would block for that long
- With threads, the RPC returns immediately with an action ID
- Client can poll for status while action runs in background

**Thread lifecycle:**
1. Thread created when action initiated
2. Thread sleeps for random duration (10-30 seconds)
3. Thread updates action status to COMPLETED/FAILED
4. Thread updates device status
5. Thread exits (joins on shutdown)

### Error Handling

The code uses multiple error handling strategies:

1. **gRPC Status Codes**: Standard HTTP-like status codes (NOT_FOUND, INVALID_ARGUMENT, etc.)
2. **Response Messages**: Human-readable error messages in response
3. **Validation**: Input validation before processing (empty strings, invalid enums, etc.)
4. **Exception Handling**: Try-catch blocks in CLI for network errors

## Assumptions and Simplifications

### In-Memory Storage
- **Assumption**: Device state is stored in memory (not persisted to disk)
- **Implication**: All data is lost when server restarts
- **Production**: Would use a database (PostgreSQL, MongoDB, etc.)

### Single Server Instance
- **Assumption**: One server instance handles all requests
- **Implication**: No distributed system concerns (load balancing, replication)
- **Production**: Would use multiple server instances behind a load balancer

### No Authentication/Authorization
- **Assumption**: No security layer (anyone can access the API)
- **Implication**: Not suitable for production without additional security
- **Production**: Would add TLS, authentication tokens, role-based access control

### Simulated Actions
- **Assumption**: Actions are simulated with random delays
- **Implication**: No actual device communication
- **Production**: Would integrate with real device APIs (REST, MQTT, etc.)

### No Action Cancellation
- **Assumption**: Once started, actions cannot be cancelled
- **Implication**: Must wait for completion or failure
- **Production**: Would add cancellation mechanism

### Fixed Action Duration
- **Assumption**: Actions take 10-30 seconds (random)
- **Implication**: Not configurable per action type
- **Production**: Would make duration configurable or based on action type

### No Action Queue
- **Assumption**: Only one action per device at a time
- **Implication**: Subsequent actions are rejected if device is busy
- **Production**: Would implement action queuing system

### No Device Heartbeat
- **Assumption**: Devices don't report their own status
- **Implication**: Status must be manually set or updated by actions
- **Production**: Would implement heartbeat mechanism for automatic status updates

## Future Improvements

If more time were available, the following enhancements would be valuable:

### 1. Persistence Layer
- **Database integration**: Store device state in PostgreSQL or MongoDB
- **Action history**: Persist completed actions for auditing
- **Data migration**: Tools for schema updates

### 2. Advanced Features
- **Action cancellation**: Ability to cancel running actions
- **Action queuing**: Queue multiple actions per device
- **Bulk operations**: Register/update multiple devices at once
- **Device groups**: Organize devices into groups for batch operations

### 3. Monitoring and Observability
- **Metrics**: Prometheus metrics for device counts, action success rates
- **Logging**: Structured logging (e.g., using spdlog)
- **Tracing**: Distributed tracing for request flow
- **Health checks**: gRPC health check service (partially implemented)

### 4. Security
- **TLS/SSL**: Encrypted communication
- **Authentication**: API keys or OAuth2 tokens
- **Authorization**: Role-based access control (RBAC)
- **Rate limiting**: Prevent abuse

### 5. Scalability
- **Load balancing**: Multiple server instances
- **Caching**: Redis for frequently accessed data
- **Message queue**: RabbitMQ/Kafka for async action processing
- **Service mesh**: Istio for service-to-service communication

### 6. Testing
- **Unit tests**: Google Test for C++ backend
- **Integration tests**: Test gRPC client-server interaction
- **Load testing**: gRPC benchmark tools
- **CI/CD**: Automated testing and deployment

### 7. Documentation
- **API documentation**: OpenAPI/Swagger for REST-like documentation
- **Architecture diagrams**: More detailed system diagrams
- **Deployment guides**: Docker, Kubernetes deployment instructions

### 8. Device Integration
- **Real device APIs**: Integrate with actual device management protocols
- **MQTT support**: For IoT device communication
- **Webhook notifications**: Notify external systems on status changes

## Troubleshooting

### Backend Issues

**Problem**: CMake can't find gRPC or Protocol Buffers
```
Solution: Install dependencies (see Prerequisites section)
```

**Problem**: Build fails with "undefined reference" errors
```
Solution: Ensure all libraries are properly linked in CMakeLists.txt
```

**Problem**: Server crashes on startup
```
Solution: Check if port 50051 is already in use:
  lsof -i :50051
  # Kill the process or use a different port
```

**Problem**: Generated code not found
```
Solution: Ensure build completed successfully. Check build/generated/ directory
```

### CLI Issues

**Problem**: "gRPC code not found" error
```
Solution: Run ./setup.sh or ./generate_grpc_code.sh
```

**Problem**: "Could not connect to server"
```
Solution: 
  1. Ensure backend server is running
  2. Check server address: python3 device_cli.py --server localhost:50051 list
  3. Verify firewall settings
```

**Problem**: Import errors in Python
```
Solution: 
  1. Ensure dependencies installed: pip3 install -r requirements.txt
  2. Verify Python version: python3 --version (should be 3.7+)
```

**Problem**: Permission denied on scripts
```
Solution: chmod +x setup.sh generate_grpc_code.sh build.sh
```
